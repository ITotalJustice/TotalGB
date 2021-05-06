project('TotalGB', ['c'],
    version: '0.0.1',
    license : 'MIT',
    default_options: [
        # core options
        'buildtype=debug',
        'warning_level=3',
        'optimization=0',
        'strip=true',
        'werror=true',

        # base options
        'b_ndebug=if-release',
        'b_sanitize=address,undefined',

        'c_std=c99',
    ],
)

dependencies = []
dependencies_str = []

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
    'src/core/apu/io.c',
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

    # frontend
    'src/frontend/main.c',
    'src/frontend/mgb.c',
    'src/frontend/util.c',
    'src/frontend/romloader.c',

    # gui stuff
    'src/frontend/gui/gui.c',
    'src/frontend/gui/menus/common.c',
    'src/frontend/gui/menus/menu_core_settings.c',
    'src/frontend/gui/menus/menu_gui_settings.c',
    'src/frontend/gui/menus/menu_help.c',
    'src/frontend/gui/menus/menu_loadrom.c',
    'src/frontend/gui/menus/menu_main.c',

    'src/frontend/ifile/ifile.c',
    'src/frontend/ifile/cfile/cfile.c',
    'src/frontend/ifile/mem/mem.c',
    'src/frontend/ifile/zip/zip.c',
    'src/frontend/ifile/gzip/gzip.c',

    'src/frontend/video/interface.c',
    'src/frontend/audio/interface.c',

    # MINIZIP
    'src/libs/minizip/unzip.c',
    'src/libs/minizip/zip.c',
    'src/libs/minizip/ioapi.c',
    'src/libs/minizip/ioapi_mem.c',

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

    # nuklear
    'src/libs/nuklear/nuklear.c',
    'src/frontend/gui/nk/interface.c',
    'src/frontend/gui/nk/gl2/gl2.c',
    #'src/frontend/gui/nk/gl3/gl3.c',
    #'src/frontend/gui/nk/gl4/gl4.c',
])


sdl1_video_backend_source = [
    'src/frontend/video/sdl1/base/base.c',
    'src/frontend/video/sdl1/sdl1.c',
]

sdl1_opengl_video_backend_source = [
    'src/frontend/video/sdl1/base/base.c',
    'src/frontend/video/sdl1/sdl1_opengl.c',
]

sdl2_opengl_video_backend_source = [
    'src/frontend/video/sdl2/base/base.c',
    'src/frontend/video/sdl2/sdl2_opengl.c',
]

sdl2_vulkan_video_backend_source = [
    'src/frontend/video/sdl2/base/base.c',
    'src/frontend/video/sdl2/sdl2_vulkan.c',
]

Allegro5_video_backend_source = [
    'src/frontend/video/allegro5/base/base.c',
    'src/frontend/video/allegro5/allegro5.c',
    'src/frontend/video/allegro5/allegro5_opengl.c',
]

sdl1_video_backend_dependencies = [
    'sdl',
]

sdl1_opengl_video_backend_dependencies = [
    'sdl',
    'gl',
]

sdl2_video_backend_dependencies = [
    'sdl2',
]

sdl2_opengl_video_backend_dependencies = [
    'sdl2',
    'gl',
]

sdl2_vulkan_video_backend_dependencies = [
    'sdl2',
    'vulkan',
]

allegro4_video_backend_dependencies = [
    'allegro-4',
    'gl',
]

allegro5_video_backend_dependencies = [
    'allegro-5',
    'gl',
]

includes = include_directories([
    'src',
    'src/libs/',
])

cflags = [
    # warns
    '-Wall',
    '-Wextra',
    '-Werror',
    '-Wfatal-errors',
    '-Wpedantic',
    '-Wshadow',
    '-Wdouble-promotion',
    '-Wformat=2',
    '-Wformat-overflow=2',
    '-Wformat-truncation=1',
    '-Wundef',
    '-Wmissing-include-dirs',
    '-Wstrict-aliasing=2',
    '-Walloca',
    '-Wduplicated-branches',
    '-Wduplicated-cond',
    '-Wwrite-strings',
    '-Wdate-time',
    '-Wsizeof-pointer-memaccess',
    '-Wlogical-op',
    '-Wpacked',
    '-Wredundant-decls',
    '-Wnested-externs',
    '-Winline',

    '-ftrapv',
    '-freg-struct-return',
    '-fno-common',

    '-Wno-unused-function',

    # these also need to be added to the linker!
    '-fsanitize=address',
    '-fsanitize=leak',
    '-fsanitize=undefined',
    '-fsanitize=pointer-compare',
    '-fsanitize=pointer-subtract',
    '-fstack-protector-all',

    '-DGB_DEBUG',

    # speeeeeeeed
    #'-O3',
    #'-flto',
    #'-DNDEBUG',

    # minizip
    '-DNOUNCRYPT',
    '-DNOCRYPT',

    '-DGB_AUDIO_FREQUENCY=44100',
]

linkflags = [
    '-lm',
    # '-flto',
    #'-fsanitize=address',
    '-fsanitize=leak',
    #'-fsanitize=undefined',
    '-fsanitize=pointer-compare',
    '-fsanitize=pointer-subtract',
    '-fstack-protector-all',
]

# todo: setup the endianness
if host_machine.endian() == 'little'
elif
endif


if get_option('video') == 'sdl1'
    source += sdl1_video_backend_source
    dependencies_str +=  sdl1_video_backend_dependencies
    cflags += '-DMGB_VIDEO_BACKEND_SDL1'

elif get_option('video') == 'sdl1_opengl'
    source += sdl1_opengl_video_backend_source
    dependencies_str +=  sdl1_opengl_video_backend_dependencies
    cflags += '-DMGB_VIDEO_BACKEND_SDL1_OPENGL'

elif get_option('video') == 'sdl2'
    source += sdl2_video_backend_source
    dependencies_str +=  sdl2_video_backend_dependencies
    cflags += '-DMGB_VIDEO_BACKEND_SDL2'

elif get_option('video') == 'sdl2_opengl'
    source += sdl2_opengl_video_backend_source
    dependencies_str +=  sdl2_opengl_video_backend_dependencies
    cflags += '-DMGB_VIDEO_BACKEND_SDL2_OPENGL'

elif get_option('video') == 'allegro4'
    source += allegro4_video_backend_source
    dependencies_str +=  allegro4_video_backend_dependencies
    cflags += '-DMGB_VIDEO_BACKEND_ALLEGRO4'

elif get_option('video') == 'allegro5'
    source += allegro5_video_backend_source
    dependencies_str +=  allegro5_video_backend_dependencies
    cflags += '-DMGB_VIDEO_BACKEND_ALLEGRO5'

elif get_option('video') == 'glfw'
    source += glfw_video_backend_source
    dependencies_str +=  glfw_video_backend_dependencies
    cflags += '-DMGB_VIDEO_BACKEND_GLFW'
endif


# detect the compiler (will be used for compiler flags)
if meson.get_compiler('c').get_id() == 'arm'
elif meson.get_compiler('c').get_id() == 'armclang'
elif meson.get_compiler('c').get_id() == 'c2000'
elif meson.get_compiler('c').get_id() == 'ccomp'
elif meson.get_compiler('c').get_id() == 'ccrx'
elif meson.get_compiler('c').get_id() == 'clang'
elif meson.get_compiler('c').get_id() == 'clang-cl'
elif meson.get_compiler('c').get_id() == 'emscripten'
elif meson.get_compiler('c').get_id() == 'gcc'
elif meson.get_compiler('c').get_id() == 'intel'
elif meson.get_compiler('c').get_id() == 'intel-cl'
elif meson.get_compiler('c').get_id() == 'lcc'
elif meson.get_compiler('c').get_id() == 'llvm'
elif meson.get_compiler('c').get_id() == 'msvc'
elif meson.get_compiler('c').get_id() == 'pgi'
elif meson.get_compiler('c').get_id() == 'xc16'
endif

# detect the OS
if host_machine.system() == 'android'
elif host_machine.system() == 'windows' or host_machine.system() == 'cygwin'
elif host_machine.system() == 'darwin'
elif host_machine.system() == 'dragonfly'
elif host_machine.system() == 'emscripten'
elif host_machine.system() == 'freebsd'
elif host_machine.system() == 'gnu'
elif host_machine.system() == 'haiku'
elif host_machine.system() == 'linux'
elif host_machine.system() == 'netbsd'
elif host_machine.system() == 'openbsd'
elif host_machine.system() == 'sunos'
endif


# add all dependencies
foreach d : dependencies_str
    dependencies += dependency(d)
endforeach


exe = executable(
    'TotalGB',
    [ source ],
    dependencies : [ dependencies ],
    install: true,
    include_directories: [ includes ],
    c_args: [ cflags ],
    link_args: [ linkflags ],
)


# scripts
run_target('cppcheck',
  command : 'scripts/cppcheck.sh')