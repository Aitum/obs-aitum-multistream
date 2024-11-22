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


# Upgrade Requests - Issues
- The configuration should save between profiles.
- Make other Stream outputs into "video encoder" stream possibilities so as to allow multiple streams for the one type of encoder rather than having to multiply the encoders out per stream.  This allows for only 2 encoders when there are 3 or more streams.  
