# Build and Run

CPU-only build; CUDA and a GPU are not required. Supported here: Linux and Windows.

## Linux

Install a C++17 compiler, `make`, `pkg-config`, SDL2, and SDL2_ttf development packages. On Ubuntu/Debian:

```sh
sudo apt install build-essential make pkg-config libsdl2-dev libsdl2-ttf-dev
make
./build/simulation
```

## Windows

Install CMake, a C++17 compiler (for example Visual Studio Build Tools), and SDL2/SDL2_ttf. With vcpkg, install the project dependencies from `vcpkg.json` and set `VCPKG_ROOT` to the vcpkg folder. From the project directory:

```bat
build.bat
build\Release\simulation.exe
```