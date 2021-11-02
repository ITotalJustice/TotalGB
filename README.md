# TotalGB

PR's, issues and any help is very much welcome!

## Building SDL2

- sdl2
- sdl2-image
- zlib

for linux, use your package manager. on windows, use vcpkg.

```cmake
cmake -B build -PLATFORM_SDL2=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Credits

- <https://gbdev.github.io/pandocs/>
