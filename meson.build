project('gb-emu', ['c', 'cpp'],
    version: '0.0.1',
    default_options: [
        'buildtype=debug',
    ],
)

dependencies = [
    dependency('sdl2', version : '>=2.0.0'),
]

source = files([
    'src/main.cpp',

    'src/core/gb.c',
    'src/core/cpu.c',
    'src/core/ppu.c',
    'src/core/bus.c',
    'src/core/apu.c',
    'src/core/joypad.c',
    'src/core/mbc.c',
    'src/core/timers.c',
    'src/core/tables/palette_table.c',

    'src/core/extra/bmp.c',
    'src/core/extra/patch/bps.c',
    'src/core/extra/patch/common.c',
    'src/core/extra/patch/crc32.c',
    'src/core/extra/patch/ips.c',
    'src/core/extra/patch/ups.c',
])

includes = include_directories([
])

cflags = [
    # optimise
    # '-Ofast',
    # '-flto',
    # '-DNDEBUG',

    # warns
    # '-Wno-unused',
    '-Wall',
    '-Wextra',
    '-Wswitch-enum',
    '-Wduplicated-branches',
    '-Wduplicated-cond',
    '-Wshadow',
    '-Wfatal-errors',
    '-Wdouble-promotion',
    '-Wframe-larger-than=100KB',

    # '-DLOG_3',
    '-DGB_DEBUG',
    # '-Wno-format',
]

cppflags = [
    '-std=gnu++20',
    # gcc-11
    # '-std=gnu++2b',
    '-Wuseless-cast',
    '-fdevirtualize-at-ltrans',
    '-Wsuggest-final-types',
]

linkflags = [
    # this is needed for <execution> algorithms
    # '-ltbb',

    # '-Wl,-subsystem,windows',
    # '-s',
]

executable(
    'gb-emu',
    [ source ],
    dependencies : [ dependencies ] ,
    install: false,
    include_directories: [ includes ],
    c_args: [ cflags ],
    cpp_args: [ cppflags, cflags ],
    link_args: [ linkflags ]
)
