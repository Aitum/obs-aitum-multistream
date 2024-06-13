# Aitum Multistream for OBS Studio

Plugin for [OBS Studio](https://github.com/obsproject/obs-studio) to add [![Aitum logo](media/aitum.png) Aitum](https://aitum.tv)

# Build
- In-tree build
    - Build OBS Studio: https://obsproject.com/wiki/Install-Instructions
    - Check out this repository to UI/frontend-plugins/aitum-multistream
    - Add `add_subdirectory(aitum-multistream)` to UI/frontend-plugins/CMakeLists.txt
    - Rebuild OBS Studio
- Stand-alone build
    - Verify that you have development files for OBS
    - Check out this repository and run `cmake -S . -B build -DBUILD_OUT_OF_TREE=On && cmake --build build`

# Translations
Please read [Translations](TRANSLATIONS.md)
