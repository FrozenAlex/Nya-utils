# Nya Utils

Image display utils to help with displaying images and gifs. 

This library was originally developed for the Nya beatsaber mod located [here](https://github.com/FrozenAlex/Nya-quest), the repository has some examples of usage of the library

## Credits

* [yttsh](https://github.com/yttsh) and [mellgrief](https://github.com/mellgrief) for helping in fixing gif decoding
* [zoller27osu](https://github.com/zoller27osu), [Sc2ad](https://github.com/Sc2ad) and [jakibaki](https://github.com/jakibaki) - [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook)
* [raftario](https://github.com/raftario)
* [Lauriethefish](https://github.com/Lauriethefish), [danrouse](https://github.com/danrouse) and [Bobby Shmurner](https://github.com/BobbyShmurner) for [this template](https://github.com/Lauriethefish/quest-mod-template)

## Build instructions

1. Install ninja, cmake and android ndk (version 24 or higher)
2. install [qpm-rust](https://github.com/Lauriethefish/quest-mod-template)
3. Create a file called `ndkpath.txt` in the project root and paste the path to ndk folder
4. Run build.ps1 to build the project
5. To run the project use copy.ps1
6. To make a qmod file run `./buildQMOD.ps1 Nya`

## Development build

To test out the library for my own project I use this command to build and copy it to my project directory (it is written in bash).

../nya is the project path in this instance

```
pwsh  build.ps1 && cp build/libnya-utils.so ../nya/extern/libs/ && cp shared/* ../nya/extern/includes/nya-utils/shared/
```


## Installation

To install the library run 
```bash
qpm-rust dependency add nya-utils
```

## How to use

### Examples in my code
[Initializing of the component](https://github.com/FrozenAlex/Nya-quest/blob/112c83a9203792ab28158bb40d589cb456e1ee03/src/NyaFloatingUI.cpp#L67)

[Usage of the DownloadImage function](https://github.com/FrozenAlex/Nya-quest/blob/112c83a9203792ab28158bb40d589cb456e1ee03/src/NyaFloatingUI.cpp#L91)

[Usage of the LoadFile function](https://github.com/FrozenAlex/Nya-quest/blob/112c83a9203792ab28158bb40d589cb456e1ee03/src/NyaFloatingUI.cpp#L85)


### How the ImageView works

ImageView component requires you to have ```HMUI::ImageView *``` component on the game object you attach it to.
To import ImageView into your current file, add
```cpp
#include "nya-utils/shared/ImageView.hpp"
```

To use it you need to have a HMUI::ImageView * in your game object, "small" example of adding it

```cpp
// Create UI layout
auto* vert = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(UIScreen->get_transform());

NYA = QuestUI::BeatSaberUI::CreateImage(vert->get_transform(), nullptr, Vector2::get_zero(), Vector2(50, 50));


// Make the image preserve aspect ratio
NYA->set_preserveAspect(true);

auto ele = NYA->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
auto view = NYA->get_gameObject()->AddComponent<NyaUtils::ImageView*>();

ele->set_preferredHeight(50);
ele->set_preferredWidth(50);
```

"Small" example of using it
```cpp
// Example of usage of the download function
view->DownloadImage(url, 10.0f, [this](bool success, long code) {
    // Callback after the function execution, success is set to true only if the image is set succesfully
    this->nyaButton->set_interactable(true);
});
```


```cpp
// Example of usage of the download function
// filePath is the full file path to the image
view->LoadFile(filePath, [this](bool success) {
    // Callback after the function execution, success is set to true only if the image is set succesfully
    this->nyaButton->set_interactable(true);
});
```