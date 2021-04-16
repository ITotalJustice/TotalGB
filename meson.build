project('TotalGB', ['c', 'cpp'],
    version: '0.0.1',
    default_options: [
        'buildtype=release',
    ],
)

dependencies = [
    # this probably works fine with older sdl2 versions
    # but this is untested.
    # 2.0.5 is the oldest version that i tested...
    dependency('sdl2', version : '>=2.0.5'),
    # this is needed for linux
    dependency('gtk+-3.0'),
    # dyn link the rest
    dependency('zlib'),
    # dependency('minizip'),
    # dependency('liblzma'),
]

source = files([
    # core
    'src/core/gb.c',
    'src/core/cpu.c',
    'src/core/bus.c',
    'src/core/joypad.c',
    'src/core/ppu/ppu.c',
    'src/core/ppu/dmg_renderer.c',
    'src/core/ppu/gbc_renderer.c',
    'src/core/ppu/sgb_renderer.c',
    'src/core/apu/apu.c',
    'src/core/apu/apu_io.c',
    'src/core/apu/square1.c',
    'src/core/apu/square2.c',
    'src/core/apu/wave.c',
    'src/core/apu/noise.c',

    'src/core/mbc/mbc.c',
    'src/core/mbc/mbc_0.c',
    'src/core/mbc/mbc_1.c',
    'src/core/mbc/mbc_2.c',
    'src/core/mbc/mbc_3.c',
    'src/core/mbc/mbc_5.c',

    'src/core/timers.c',
    'src/core/serial.c',
    'src/core/sgb.c',
    'src/core/accessories/printer.c',
    'src/core/tables/palette_table.c',

    # core extras
    'src/core/extra/bmp.c',
    'src/core/extra/patch/bps.c',
    'src/core/extra/patch/common.c',
    'src/core/extra/patch/crc32.c',
    'src/core/extra/patch/ips.c',
    'src/core/extra/patch/ups.c',

    # frontend mgb++
    'src/frontend/main.cpp',
    'src/frontend/mgb.cpp',
    'src/frontend/events.cpp',
    'src/frontend/util/mem.cpp',
    'src/frontend/util/util.cpp',
    'src/frontend/io/ifile_7z.cpp',
    'src/frontend/io/ifile_cfile.cpp',
    'src/frontend/io/ifile_gzip.cpp',
    'src/frontend/io/ifile_mem.cpp',
    'src/frontend/io/ifile_rar.cpp',
    'src/frontend/io/ifile_zip.cpp',
    'src/frontend/io/ifile_zstd.cpp',
    'src/frontend/io/romloader.cpp',

    # MINIZIP
    'src/libs/minizip/unzip.c',
    'src/libs/minizip/ioapi.c',

    # LZMA
    'src/libs/lzma/Alloc.c',
    'src/libs/lzma/LzFind.c',
    'src/libs/lzma/LzmaDec.c',
    'src/libs/lzma/LzmaEnc.c',
    'src/libs/lzma/LzmaLib.c',
])

includes = include_directories([
    'src',
    'src/libs/',
])

# https://github.com/mesonbuild/meson/blob/master/docs/markdown/Reference-manual.md#build_machine-object
# https://github.com/mesonbuild/meson/blob/master/docs/markdown/Reference-tables.md#operating-system-names

# this is probably not the sane way to do it, but eh it works :)
if host_machine.system() == 'windows' or host_machine.system() == 'cygwin'
    # nativefiledialog
    source += 'src/libs/nativefiledialog/nfd_win.cpp'
elif host_machine.system() == 'linux'
    # # nativefiledialog
    source += 'src/libs/nativefiledialog/nfd_gtk.cpp'
elif host_machine.system() == 'darwin'
    # # nativefiledialog
    source += 'src/libs/nativefiledialog/nfd_cocoa.m'
endif

cflags = [
    # optimise
    # '-Ofast',
    # lto seems to break clang
    # '-flto',
    # '-DNDEBUG',

    # warns
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wswitch-enum',
    '-Wshadow',
    '-Wfatal-errors',
    '-Wdouble-promotion',
    #'-Wframe-larger-than=300KB',

    '-DGB_DEBUG',

    # 7zip
    '-D_7ZIP_ST',

    # minizip
    '-DNOUNCRYPT',
    '-DNOCRYPT',

    '-DMGB_NO_LZ4',
    '-DMGB_NO_ZSTD',

    # sdl_thread audio (race condition!)
    # '-DGB_SDL_AUDIO_CALLBACK=1',

    # '-DGB_SDL_AUDIO_CALLBACK_STREAM=1',

    # '-DDUMP_AUDIO=1',

    '-DSCALE=3',
]

cppflags = [
    '-std=c++17',
]

linkflags = [
    # '-Wl,-subsystem,windows',
    '-s',
]

#subdir('res')

executable(
    'TotalGB',
 #   res, # set the icon
    [ source ],
    dependencies : [ dependencies ] ,
    install: false,
    include_directories: [ includes ],
    c_args: [ cflags ],
    cpp_args: [ cppflags, cflags ],
    link_args: [ linkflags ]
)
