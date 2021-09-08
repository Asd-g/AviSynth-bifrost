## Description

Bifrost is a temporal derainbowing filter created by Fredrik Mellbin.

The original Avisynth plugin (version 1.1) worked on the whole frame or not at all. This version works on blocks, meaning that static parts of the image can be processed even if something moves on screen.

### Requirements:

- AviSynth 2.60 / AviSynth+ 3.4 or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases))

### Usage:

```
Bifrost (clip input, clip "altclip", float "luma_thresh", int "variation", bool "conservative_mask", bool "interlaced", int "blockx", int "blocky")
```

### Parameters:

- input\
    A clip to process. It must be in YUV 8..16-bit planar format and must have at least three planes.
    
- altclip\
    Bifrost will copy from this clip the chroma of the blocks it can't process. This allows moving blocks to be processed with some other filter.\
    altclip must have the same format, dimensions and length as clip.\
    Default: input.
    
- luma_thresh\
    Blocks where the luma changes less than this are considered static and may be processed.\
    Must be between 0.0 and 255.0\
    Default: 10.0.
    
- variation\
    Controls how big a chroma change must be in order to be considered a rainbow.\
    Increasing this can reduce false positives.\
    Must be between 0 and 255.\
    Default: 5.
    
- conservative_mask\
    If true, only pixels detected as rainbows will be processed.\
    Otherwise, pixels that have rainbows above and below them will also be processed.\
    Default: False.
    
- interlaced\
    If true, SeparateFields and Weave will be invoked in order to process the top and bottom fields separatedly.\
    Default: True.
    
- blockx, blocky\
    The dimensions of the blocks.\
    Smaller is probably better.\
    Default: 4.

### Building:

- Windows\
    Use solution files.

- Linux
    ```
    Requirements:
        - Git
        - C++11 compiler
        - CMake >= 3.16
    ```
    ```
    git clone https://github.com/Asd-g/AviSynth-bifrost && \
    cd AviSynth-bifrost && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j$(nproc) && \
    sudo make install
    ```
