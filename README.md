# TotalGB Prehistorik-man

This branch features a hacky way of correctly rendering prehistorik man by keeping track of when palette changes occur during the scanline.

While this worked, it broke pretty much every other GB game, namely zelda and a few ppu tests.

so this was removed from master branch and is kept here so i can refrence it if needed, without having to go through commit history

## Building SDL2

you will need to install the following packages.

- sdl2

for linux, use your package manager. on windows, use vcpkg.

```cmake
cmake -B build -PLATFORM_SDL2=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Credits

- <https://gbdev.github.io/pandocs/>
