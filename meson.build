project('gb-emu', ['c', 'cpp', 'objc'], # objc for osx
    version: '0.0.1',
    default_options: [
        'buildtype=debug',
    ],
)

dependencies = [
    # this probably works fine with older sdl2 versions
    # but this is untested.
    # 2.0.5 is the oldest version that i tested...
    dependency('sdl2', version : '>=2.0.5'),
]

source = files([
    # core
    'src/core/gb.c',
    'src/core/cpu.c',
    'src/core/ppu.c',
    'src/core/bus.c',
    'src/core/apu.c',
    'src/core/joypad.c',
    'src/core/mbc.c',
    'src/core/timers.c',
    'src/core/serial.c',
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
    'src/frontend/util/rewind.cpp',
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

    # LZ4
    'src/libs/lz4/lz4.c',

    # ZSTD
    'src/libs/zstd/common/debug.c',
    'src/libs/zstd/common/entropy_common.c',
    'src/libs/zstd/common/error_private.c',
    'src/libs/zstd/common/fse_decompress.c',
    'src/libs/zstd/common/pool.c',
    'src/libs/zstd/common/threading.c',
    'src/libs/zstd/common/xxhash.c',
    'src/libs/zstd/common/zstd_common.c',
    'src/libs/zstd/compress/fse_compress.c',
    'src/libs/zstd/compress/hist.c',
    'src/libs/zstd/compress/huf_compress.c',
    'src/libs/zstd/compress/zstd_compress.c',
    'src/libs/zstd/compress/zstd_compress_literals.c',
    'src/libs/zstd/compress/zstd_compress_sequences.c',
    'src/libs/zstd/compress/zstd_compress_superblock.c',
    'src/libs/zstd/compress/zstd_double_fast.c',
    'src/libs/zstd/compress/zstd_fast.c',
    'src/libs/zstd/compress/zstd_lazy.c',
    'src/libs/zstd/compress/zstd_ldm.c',
    'src/libs/zstd/compress/zstd_opt.c',
    'src/libs/zstd/compress/zstdmt_compress.c',
    'src/libs/zstd/decompress/huf_decompress.c',
    'src/libs/zstd/decompress/zstd_ddict.c',
    'src/libs/zstd/decompress/zstd_decompress.c',
    'src/libs/zstd/decompress/zstd_decompress_block.c',

    # LZMA
    'src/libs/lzma/Alloc.c',
    'src/libs/lzma/LzFind.c',
    'src/libs/lzma/LzmaDec.c',
    'src/libs/lzma/LzmaEnc.c',
    'src/libs/lzma/LzmaLib.c',

    # discord-rpc
    'src/libs/discord_rpc/discord_rpc.cpp',
    'src/libs/discord_rpc/dllmain.cpp',
    'src/libs/discord_rpc/rpc_connection.cpp',
    'src/libs/discord_rpc/serialization.cpp',
])

includes = include_directories([
    'src/libs/'
])

# https://github.com/mesonbuild/meson/blob/master/docs/markdown/Reference-manual.md#build_machine-object
# https://github.com/mesonbuild/meson/blob/master/docs/markdown/Reference-tables.md#operating-system-names

# this is probably not the sane way to do it, but eh it works :)
if host_machine.system() == 'windows' or host_machine.system() == 'cygwin'
    # discord-rpc
    source += 'src/libs/discord_rpc/connection_win.cpp'
    source += 'src/libs/discord_rpc/discord_register_win.cpp'
    # nativefiledialog
    source += 'src/libs/nativefiledialog/nfd_win.cpp'
elif host_machine.system() == 'linux'
    # discord-rpc
    source += 'src/libs/discord_rpc/connection_linux.cpp'
    source += 'src/libs/discord_rpc/discord_register_linux.cpp'
    # # nativefiledialog
    source += 'src/libs/nativefiledialog/nfd_gtk.cpp'
elif host_machine.system() == 'darwin'
    # discord-rpc
    source += 'src/libs/discord_rpc/discord_register_osx.m'
    # # nativefiledialog
    source += 'src/libs/nativefiledialog/nfd_cocoa.m'
endif

cflags = [
    # optimise
    # '-Ofast',
    # '-flto',
    # '-DNDEBUG',

    # warns
    '-Wall',
    '-Wextra',
    # '-Wswitch-enum',
    '-Wduplicated-branches',
    '-Wduplicated-cond',
    '-Wshadow',
    '-Wfatal-errors',
    '-Wdouble-promotion',
    '-Wframe-larger-than=300KB',

    # '-DGB_DEBUG',

    # 7zip
    '-D_7ZIP_ST',

    # zstd
    '-DZSTD_LIB_MINIFY=1',

    # lz4
    '-DLZ4_FAST_DEC_LOOP=1',

    # minizip
    '-DNOUNCRYPT',
    '-DNOCRYPT',

    # discord-rpc
    # '-DUSE_DISCORD=1',
    # '-DDISCORD_WINDOWS',

    # speedhacks
    '-DGB_RTC_SPEEDHACK=1'
]

cppflags = [
    '-std=gnu++20',
    # '-Wuseless-cast',
    '-fdevirtualize-at-ltrans',
    '-Wsuggest-final-types',
]

linkflags = [
    # '-Wl,-subsystem,windows',
    # '-s',
]

subdir('res')

executable(
    'gb-emu',
    res, # set the icon
    [ source ],
    dependencies : [ dependencies ] ,
    install: false,
    include_directories: [ includes ],
    c_args: [ cflags ],
    cpp_args: [ cppflags, cflags ],
    link_args: [ linkflags ]
)
