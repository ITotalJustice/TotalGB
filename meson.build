project('TotalGB', ['c', 'cpp'],
    version: '0.0.1',
    default_options: [
        # core options
        'buildtype=release',
        'debug=false',
        'warning_level=3',
        'optimization=3',
        'strip=true',

        # base options
        # 'b_lto=true',
        'b_ndebug=if-release',

        'c_std=c11',
        'cpp_std=c++17',
        'cpp_rtti=false',
    ],
)

dependencies = []

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
    
    'src/frontend/util/mem.cpp',
    'src/frontend/util/util.cpp',
    
    'src/frontend/util/io/ifile_7z.cpp',
    'src/frontend/util/io/ifile_cfile.cpp',
    'src/frontend/util/io/ifile_gzip.cpp',
    'src/frontend/util/io/ifile_mem.cpp',
    'src/frontend/util/io/ifile_rar.cpp',
    'src/frontend/util/io/ifile_zip.cpp',
    'src/frontend/util/io/ifile_zstd.cpp',
    'src/frontend/util/io/romloader.cpp',
    
    # [Video Backends]
    'src/frontend/platforms/video/interface.cpp',
    'src/frontend/platforms/video/sdl2/sdl2_base.cpp',
    'src/frontend/platforms/video/sdl2/sdl2_ttf.cpp',
    'src/frontend/platforms/video/sdl2/sdl2_video.cpp',
    # 'src/frontend/platforms/video/sdl2-opengl/sdl2_opengl.cpp',
    # 'src/frontend/platforms/video/allegro5/allegro5_video.cpp',

    # [Audio Backends]
    # 'src/frontend/platforms/audio/sdl1/sdl1_audio.cpp',
    'src/frontend/platforms/audio/sdl2/sdl2_audio.cpp',
    # 'src/frontend/platforms/audio/allegro5/allegro5_audio.cpp',

    # MINIZIP
    'src/libs/minizip/unzip.c',
    'src/libs/minizip/ioapi.c',

    # ZLIB
    'src/libs/zlib/adler32.c',
    'src/libs/zlib/compress.c',
    'src/libs/zlib/crc32.c',
    'src/libs/zlib/deflate.c',
    'src/libs/zlib/gzclose.c',
    'src/libs/zlib/gzlib.c',
    'src/libs/zlib/gzread.c',
    'src/libs/zlib/gzwrite.c',
    'src/libs/zlib/infback.c',
    'src/libs/zlib/inffast.c',
    'src/libs/zlib/inflate.c',
    'src/libs/zlib/inftrees.c',
    'src/libs/zlib/trees.c',
    'src/libs/zlib/uncompr.c',
    'src/libs/zlib/zutil.c',

    # LZMA
    'src/libs/lzma/Alloc.c',
    'src/libs/lzma/LzFind.c',
    'src/libs/lzma/LzmaDec.c',
    'src/libs/lzma/LzmaEnc.c',
    'src/libs/lzma/LzmaLib.c',

    # stb
    'src/libs/stb/stb_rect_pack.c',
    'src/libs/stb/stb_truetype.c',
])

includes = include_directories([
    'src',
    'src/libs/',
])

cflags = [
    # warns
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wswitch-enum',
    '-Wshadow',
    '-Wfatal-errors',
    '-Wdouble-promotion',

    # '-DGB_DEBUG',

    # 7zip
    '-D_7ZIP_ST',

    # minizip
    '-DNOUNCRYPT',
    '-DNOCRYPT',

    '-DMGB_NO_LZ4',
    '-DMGB_NO_ZSTD',
]

cppflags = [
    '-DMGB_SDL2_AUDIO',
    '-DMGB_SDL2_VIDEO',
]

linkflags = []


# https://github.com/mesonbuild/meson/blob/master/docs/markdown/Reference-manual.md#build_machine-object
# https://github.com/mesonbuild/meson/blob/master/docs/markdown/Reference-tables.md#operating-system-names
# this is probably not the sane way to do it, but eh it works :)

if host_machine.system() == 'windows' or host_machine.system() == 'cygwin'
    source += 'src/libs/nativefiledialog/nfd_win.cpp'

    dependencies += dependency('sdl2', version : '>=2.0.5')

elif host_machine.system() == 'linux'
    source += 'src/libs/nativefiledialog/nfd_gtk.cpp'

    dependencies += dependency('sdl2', version : '>=2.0.5')
    dependencies += dependency('gtk+-3.0')

elif host_machine.system() == 'darwin'
    source += 'src/libs/nativefiledialog/nfd_cocoa.m'

    dependencies += dependency('sdl2', version : '>=2.0.5')

elif host_machine.system() == 'emscripten'
    cflags += ['-s', 'USE_SDL=2']
    cppflags += ['-s', 'USE_SDL=2']
    linkflags += ['-s', 'USE_SDL=2']

    # only enable if using async stuff else it'll bloat the code
    # cflags += ['-s', 'ASYNCIFY']
    # cppflags += ['-s', 'ASYNCIFY']
    # linkflags += ['-s', 'ASYNCIFY']
endif


exe = executable(
    'TotalGB',
    [ source ],
    dependencies : [ dependencies ],
    install: true,
    include_directories: [ includes ],
    
    c_args: [ cflags ],
    cpp_args: [ cppflags, cflags ],
    link_args: [ linkflags ],
)
