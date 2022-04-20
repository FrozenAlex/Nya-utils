#define RAPIDJSON_HAS_STDSTRING 1
// OurClass.cpp
#include "ImageView.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "HMUI/ImageView.hpp"
#include "WebUtils.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"


// Necessary
DEFINE_TYPE(NyaUtils, ImageView);

using namespace UnityEngine;

// Start
void NyaUtils::ImageView::ctor()
{
  floatVar = 1.0f;
  frameTime = 0.0f;

  currentFrame = 0;
  width = 0.0f;
  height = 0.0f;
  ArrayW<float> animationTimings;
  ArrayW<UnityEngine::Texture2D *> animationFrames;
  play = false;

  imageView = this->get_gameObject()->GetComponent<HMUI::ImageView *>();
  // Constructor!
}

// Update
void NyaUtils::ImageView::Update()
{
  if (play)
  {
    int length = animationFrames.Length();
    if (length > 0)
    {
      float deltaTime = Time::get_deltaTime();

      bool isFrameNeeded = false;

      // Count frame length
      float frameLength = animationTimings[currentFrame] / 100;
      if (frameLength > 0.0f)
      {
        // Basic delta time based frame switching
        while (frameTime > frameLength)
        {
          currentFrame = (currentFrame + 1) % length;
          isFrameNeeded = true;
          frameTime = frameTime - frameLength;
          frameLength = animationTimings[currentFrame] / 100;
        }
      }
      else
      {
        // Skip the frame with 0 ms
        currentFrame = (currentFrame + 1) % length;
        isFrameNeeded = true;
        frameLength = animationTimings[currentFrame] / 100;
      }

      if (isFrameNeeded)
      {
        if (imageView == nullptr)
        {
          il2cpp_utils::getLogger().warning("imageView is null");
          imageView = this->get_gameObject()->GetComponent<HMUI::ImageView *>();
        }
        if (animationFrames.Length() > currentFrame)
        {
          auto frame = animationFrames.get(currentFrame);

          if (frame != nullptr)
          {
            if (this->canvasRenderer == nullptr)
            {
              il2cpp_utils::getLogger().warning("sprite renderer is null");
            }
            else
            {
              canvasRenderer->SetTexture(frame);
            }
          }
        }
      }
      frameTime += deltaTime;
    }
  }
}

// Update
void NyaUtils::ImageView::UpdateImage(ArrayW<UnityEngine::Texture2D *> frames, ArrayW<float> timings, float ImageWidth, float ImageHeight)
{
  // Check for nulls
  if (!frames)
  {
    il2cpp_utils::getLogger().warning("Frames are null, skipping");
    return;
  }
  if (!timings)
  {
    il2cpp_utils::getLogger().warning("Timings are null, skipping");
    return;
  }
  if (frames.Length() < 1)
  {
    il2cpp_utils::getLogger().warning("Zero frames, skipping");
    return;
  }

  // CHECK if the gif has zero timings to prevent infinite loop
  float total_length = 0.0f;
  for (int i = 0; i < timings.Length(); i++)
  {
    total_length += timings[i];
  }
  if (total_length == 0)
  {
    il2cpp_utils::getLogger().warning("Gif has zero timings for some reason, skipping...");
    return;
  }

  // Validate width and height
  if (!(ImageWidth > 0 && ImageHeight > 0))
  {
    il2cpp_utils::getLogger().warning("Timings are null, skipping");
    return;
  }

  // Clean things
  cleanupTextures();

  // stop gif playback
  play = false;
  currentFrame = 0;

  // Update variables
  animationFrames = frames;
  animationTimings = timings;
  currentFrame = 0;

  width = ImageWidth;
  height = ImageHeight;

  // Create a sprite with new dimensions with first frame
  auto sprite = Sprite::Create(animationFrames[0],
                               Rect(0.0f, 0.0f, (float)width, (float)height),
                               Vector2(0.5f, 0.5f), 1024.0f, 1u, SpriteMeshType::FullRect, Vector4(0.0f, 0.0f, 0.0f, 0.0f), false);

  imageView->set_sprite(sprite);

  // Get canvas renderer reference
  this->canvasRenderer = this->imageView->get_canvasRenderer();
  // Start playback
  play = true;
}

// Set normal image
void NyaUtils::ImageView::UpdateStaticImage(UnityEngine::Sprite *image)
{
  play = false;
  cleanupTextures();
  currentFrame = 0;
  imageView->set_sprite(image);
  this->canvasRenderer = this->imageView->get_canvasRenderer();
}

void NyaUtils::ImageView::cleanupTextures()
{
  auto oldFrames = animationFrames;
  UnityEngine::Sprite *oldSprite = imageView->get_sprite();
  UnityEngine::Texture2D *oldTexture = nullptr;
  if (oldSprite != nullptr)
  {
    oldTexture = oldSprite->get_texture();
  }

  // Cleanup
  if (oldTexture != nullptr)
  {
    oldTexture = nullptr;
  }
  if (oldSprite != nullptr)
  {
    UnityEngine::Object::Destroy(oldSprite);
  }
  // Clear all previous textures
  if (oldFrames)
  {
    int length = oldFrames.Length();
    for (int i = 0; i < length; i++)
    {
      auto frame = oldFrames[i];
      if (frame != nullptr)
      {
        UnityEngine::Object::Destroy(frame);
      }
    }
  }
}

void NyaUtils::ImageView::dtor()
{
  cleanupTextures();
}

void NyaUtils::ImageView::DownloadImage(
  StringW url, 
  float timeoutInSeconds,
  std::function<void()> finished
)
{
  WebUtils::GetAsync(
      url,
      timeoutInSeconds,
      [&](long code, std::string result)
      {
          switch (code)
          {
              case 200:
                  rapidjson::Document document;
                  document.Parse(result);
                  if(document.HasParseError() || !document.IsObject())
                      return;
                  std::string url = "";
                  if(document.HasMember("url"))
                  {
                      url = document.FindMember("url")->value.GetString();
                  }
                  il2cpp_utils::getLogger().debug("%s", url.c_str());
                  
                  WebUtils::GetAsync(url, 10.0, [this, url,finished](long code, std::string result){
                      std::vector<uint8_t> bytes(result.begin(), result.end());
                      
                      il2cpp_utils::getLogger().debug("Downloaded Image!");
                      il2cpp_utils::getLogger().debug("%lu", bytes.size());
                      switch (code)
                      {
                          case 200:
                              if(url.find(".gif") != std::string::npos) {
                                  il2cpp_utils::getLogger().debug("Found a gif");
                                      QuestUI::MainThreadScheduler::Schedule([this, result, finished]
                                      {
                                          std::string resCopy = result;

                                          // Decode the gif
                                          Gif gif(resCopy);
                                          int parseResult = gif.Parse();
                                          int slurpResult = gif.Slurp();
                                          int width = gif.get_width();
                                          int height = gif.get_height();
                                          int length = gif.get_length();
                                          AllFramesResult result = gif.get_all_frames();
                                          
                          
                                          
                                          this->UpdateImage(result.frames, result.timings,  (float)width, (float)height);
                                          if (finished != nullptr) {
                                            finished();
                                          }
                                      });
                              } else {
                                  il2cpp_utils::getLogger().debug("Static image");
                                  QuestUI::MainThreadScheduler::Schedule([this, bytes, finished]
                                  {
                                      UnityEngine::Sprite* sprite = QuestUI::BeatSaberUI::VectorToSprite(bytes);
                                      this->UpdateStaticImage(sprite);
                                      if (finished != nullptr) {
                                         finished();
                                      }
                                     
                                  });
                              }
                      }
                  });
                  break;
          } });
}