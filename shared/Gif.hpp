#include "gif-lib/shared/gif_lib.h"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/Color32.hpp"
#include "UnityEngine/TextureWrapMode.hpp"
#include "UnityEngine/FilterMode.hpp"
#include <thread>
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

struct AllFramesResult
{
    ArrayW<UnityEngine::Texture2D *> frames;
    ArrayW<float> timings;
};

// raw PVRTC bytes for Unity
struct TextureColor
{
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
    uint8_t Transparency;
};

// wrapper type to wrap any datatype for use within a stream
template <typename CharT, typename TraitsT = std::char_traits<CharT>>
class vectorwrapbuf : public std::basic_streambuf<CharT, TraitsT>
{
public:
    vectorwrapbuf(std::string &text)
    {
        this->std::basic_streambuf<CharT, TraitsT>::setg(text.data(), text.data(), text.data() + text.size());
    }

    vectorwrapbuf(std::vector<CharT> &vec)
    {
        this->std::basic_streambuf<CharT, TraitsT>::setg(vec.data(), vec.data(), vec.data() + vec.size());
    }

    vectorwrapbuf(Array<CharT> *&arr)
    {
        this->std::basic_streambuf<CharT, TraitsT>::setg(arr->values, arr->values, arr->values + arr->Length());
    }
};

class GifDecoder
{
private:
    std::istream *datastream = nullptr;
    vectorwrapbuf<char> *data = nullptr;

    /**
     * 1. Go to a different thread to parse stuff
     * 2. Decode gif into raw texture things.
     * 3. Make textures and cleanup the source material
     */

public:
    GifFileType *gif = nullptr;
    TextureColor *pixelData = nullptr;
    ArrayW<float> timings;
    int width = 0;
    int height = 0;
    int length = 0;

    //
    GifDecoder()
    {
    }

    GifDecoder(std::string &text)
    {
        this->data = new vectorwrapbuf<char>(text);
        this->datastream = new std::istream(this->data);
        this->Parse();
    };

    // Destruct the gif
    ~GifDecoder()
    {
        int error = 0;
        DGifCloseFile(gif, &error);
        delete datastream;
        delete data;
    }

    int Parse()
    {
        int error = 0;
        /// open Gif file, first arg is anything you wanna pass (for us it's the this instance), second arg is a static function pointer that takes the args that Gif::read takes
        gif = DGifOpen(this, &GifDecoder::read, &error);

        /// 0 means success! keep that in mind
        return error;
    }

    /// @brief parse given data, pretty much always ran directly after construction
    /// @return error code
    int ParseFile(std::string filePath)
    {
        int error = 0;
        auto filePathc = filePath.c_str();
        /// open Gif file, first arg is anything you wanna pass (for us it's the this instance), second arg is a static function pointer that takes the args that Gif::read takes
        this->gif = DGifOpenFileName(filePathc, &error);

        /// 0 means success! keep that in mind
        return error;
    }

    int Slurp()
    {
        return DGifSlurp(gif);
    }

    /// @brief static function that reads the data from the datastream
    /// @param pGifHandle the gif file
    /// @param dest the buffer to push to
    /// @param toRead the amount of bytes expected to be read into the buffer
    static int read(GifFileType *pGifHandle, GifByteType *dest, int toRead)
    {
        /// gifhandle->UserData is the first argument passed to DGifOpen where we passed the this pointer
        GifDecoder &dataWrapper = *(GifDecoder *)pGifHandle->UserData;
        return dataWrapper.datastream->readsome(reinterpret_cast<char *>(dest), toRead);
    }

    void DecodeIntoFrames(std::function<void(bool success)> finished)
    {
        std::thread t(
            [this, finished]
            {
                std::thread::id this_id = std::this_thread::get_id();
                il2cpp_utils::getLogger().warning("Runs in a thread %d", this_id);
                
                int slurpResult = this->Slurp();

                const char *GifVersion = DGifGetGifVersion(gif);
                int length = this->get_length();

                if (!gif || length == 0)
                    return;

                // Get gif params
                this->length = length;
                int width = get_width();
                int height = get_height();
                this->width = width;
                this->height = height;
                il2cpp_utils::getLogger().warning("Got gif length");

                // FrameBuffer with continuous pixel data
                this->pixelData = new TextureColor[width * height * length];
                this->timings = ArrayW<float>(length);

                // Persist data from the previous frame
                GifColorType *color;
                SavedImage *frame;
                ExtensionBlock *ext = nullptr;
                GifImageDesc *frameInfo;
                ColorMapObject *colorMap;
                // Graphic control ext block
                GraphicsControlBlock GCB;
                int GCBResult = GIF_ERROR;

                for (int idx = 0; idx < length; idx++)
                {
                    int x, y, j, loc;

                    TextureColor *frameData = pixelData + (width * height * idx);
                    

                    // Copy memory to keep prev frame
                    if (idx > 0)
                    {
                        TextureColor *prevFrameData = pixelData + (width * height * (idx - 1));
                        memcpy(frameData, prevFrameData,  4 * width * height);
                    }

                    frame = &(gif->SavedImages[idx]);
                    frameInfo = &(frame->ImageDesc);
                    if (frameInfo->ColorMap)
                    {
                        colorMap = frameInfo->ColorMap;
                    }
                    else
                    {
                        colorMap = gif->SColorMap;
                    }
                    for (j = 0; j < frame->ExtensionBlockCount; ++j)
                    {
                        if (frame->ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE)
                        {
                            ext = &(frame->ExtensionBlocks[j]);
                            break;
                        }
                    }

                    // Get additional info about the frame
                    if (ext != nullptr)
                    {
                        GCBResult = DGifExtensionToGCB(ext->ByteCount, (const GifByteType *)ext->Bytes, &GCB);
                    }
                    // gif->SWidth is not neccesarily the same as FrameInfo->Width due to a frame possibly describing a smaller block of pixels than the entire gif size

                    // UnityEngine::Texture2D *texture = UnityEngine::Texture2D::New_ctor(width, height, UnityEngine::TextureFormat::RGBA32, false);
                    // This is the same size as the entire size of the gif :)
                    // offset into the entire image, might need to also have it's y value flipped? need to test
                    long flippedFrameTop = height - frameInfo->Top - frameInfo->Height;
                    long pixelDataOffset = flippedFrameTop * width + frameInfo->Left;
                    // it's easier to understand iteration from 0 -> value, than it is to understand value -> value

                    for (y = 0; y < frameInfo->Height; ++y)
                    {
                        for (x = 0; x < frameInfo->Width; ++x)
                        {
                            // Weirdness here is to flip Y coordinate
                            loc = (frameInfo->Height - y - 1) * frameInfo->Width + x;
                            // Checks if the pixel is transparent
                            if (GCB.TransparentColor >= 0 && frame->RasterBits[loc] == ext->Bytes[3] && ext->Bytes[0])
                            {
                                continue;
                            }

                            color = &colorMap->Colors[frame->RasterBits[loc]];
                            // for now we just use this method to determine where to draw on the image, we will probably come across a better way though
                            long locWithinFrame = x + pixelDataOffset;
                            frameData[locWithinFrame] = {
                                color->Red, color->Green, color->Blue, 0xff};
                        }

                        // Goes to a new row (saves compute power)
                        pixelDataOffset = pixelDataOffset + width;
                    }

                    this->timings[idx] = static_cast<float>(GCB.DelayTime);
                };
                finished(true);
            });
        t.detach();
      
    }


    void ConvertToTextures(std::function<void(AllFramesResult result)> finished)
    {
        int length = this->get_length();
        
        
        if (!gif || length == 0)
            return;
        
        int width = get_width();
        int height = get_height();

        // Create the frames array
        auto frames = ArrayW<UnityEngine::Texture2D*>(length);

        for (int idx = 0; idx < length; idx++) {
            // 
            UnityEngine::Texture2D* texture = UnityEngine::Texture2D::New_ctor(width, height, UnityEngine::TextureFormat::RGBA32, false);

            // Get the frame
            TextureColor *frameData = this->pixelData + (width * height * idx);
            // Copy raw pixel data to texture
            texture->LoadRawTextureData(frameData,  width * height * 4);
            // texture->set_filterMode(UnityEngine::FilterMode::Trilinear);
            // Compress texture
            texture->Compress(false);
            // Upload to GPU
            texture->Apply();

            frames[idx] = texture;
        }

        // Free the pixel data
        delete this->pixelData;
        
        finished({
            frames, this->timings
        });
    }
    int get_width() { return gif ? gif->SWidth : 0; };
    int get_height() { return gif ? gif->SHeight : 0; };
    int get_length() { return gif ? gif->ImageCount : 0; };
};
