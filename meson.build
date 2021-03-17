project('gb-emu', ['c', 'cpp'],
    version: '0.0.1',
    default_options: [
        'buildtype=debug',
    ],
)

dependencies = [
]

source = files([
    'src/main.cpp',
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
