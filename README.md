# LED-Strip DDP Simulator

*LED-Strip DDP Simulator* can be used as a visual debugging tool for the [DDP protocol](http://www.3waylabs.com/ddp/).
It really shines✨ fullscreen on e.g. a Raspberry Pi to turn any monitor into a low-fi part of your lighting setup,
e.g. using [LedFx](https://www.ledfx.app/) to drive animations.

- 🎆 Fullscreen or window (options for undecorated/borderless and stay-on-top)
- 🕺 Low latency
- 💫 Configurable high framerate
- 🔋 Minimal resource usage
- 🚀 Hardware accelerated
- 🪄 Framebuffer or Window manager (X / Wayland / MacOS)

## Releases

On the [Releases page](https://github.com/zuckschwerdt/ledstrip-ddp-simulator/releases) the following variants are available:
- `Desktop-amd64`: Runs with a window manager on Linux x86_64
- `Desktop-arm64`: Runs with a window manager on Linux aarch64 (Raspberry Pi 64-bit)
- `Desktop-armhf`: Runs with a window manager on Linux armhf (Raspberry Pi 32-bit)
- `DRM-amd64`: Runs in a Framebuffer (on the Console) on Linux x86_64
- `DRM-arm64`: Runs in a Framebuffer (on the Console) on Linux aarch64 (Raspberry Pi 64-bit)
- `DRM-armhf`: Runs in a Framebuffer (on the Console) on Linux armhf (Raspberry Pi 32-bit)
- `MacOS-x86_64`: Runs on MacOS x86_64 (Intel)
- `MacOS-arm64`: Runs on MacOS arm64 (M1)

## Building

This project uses [Raylib]( https://www.raylib.com/ ) with
the [CMake](https://cmake.org) [template](https://github.com/raysan5/raylib/tree/master/projects/CMake).
To compile, use one of the following dependending on your build target.

### Desktop

Use the following commands to build for desktop:

``` bash
cmake -DPLATFORM=Desktop -B build
cmake --build build
```

### DRM (Framebuffer)

Use the following commands to build for DRM (framebuffer):

``` bash
cmake -DPLATFORM=DRM -B build
cmake --build build
```

## Future Ideas

Support for e.g. Art-Net and E1.31 could be added, if there is sufficient interest.

## Usage

### General options
- `[-V]` Output the version string and exit
- `[-v]` Increase verbosity (can be used multiple times).
         -v : verbose, -vv : debug, -vvv : trace.
- `[-h]` Output this usage help and exit
### Geometry parameters
- `[-s NxM]` Screen/window width to use (default 800x600).
- `[-p N | NxM]` horizontal pixel count (default 20x10).
- `[-n N]` total pixel count, automatic if `pixelY` ist set.
- `[-g N | NxM]` horizontal gap between pixels (default 15x15).
### Geometry options
- `[-S]` Enable snake layout, alternates direction of rows.
- `[-M]` Enable mirror layout, mirros horizontally.
- `[-F]` Enable flip layout, flips vertically.
- `[-T]` Enable tilt layout, transforms diagonally.
- `[-R]` Rotate layout right.
- `[-L]` Rotate layout left.
- `[-C]` Enable circle drawing.
- `[-O]` Enable text overlay.
### Statistics options
- `[-f N]` Target FPS (default 60).
- `[-H N]` Hold N seconds before blanking (0 is forever, default 0).
- `[-E N]` Exit after being idle N seconds (0 is never, default 0).
- `[-r N]` Console report rate.
- `[-d N]` Dump every n'th packet.

### Hotkeys

The windowed builds (Desktop) support the following hotkeys:

- <kbd>Esc</kbd>: Quit
- <kbd>F</kbd>: Toggles fullscreen
- <kbd>R</kbd>: Toggles resizable window
- <kbd>D</kbd>: Toggles undecorated window
- <kbd>M</kbd>: Toggles maximized window
- <kbd>T</kbd>: Toggles topmost window
- <kbd>V</kbd>: Toggles vsync hint
