project('TotalGB', 'c',
  version : '0.0.1',
  license : 'MIT',
  default_options : [
    'warning_level=3',
    'optimization=3',
    'strip=true',
    'c_std=c99',
  ]
)

c_flags = [
]

c_warnings = [
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
]

source = files([
    'src/gb.c',
    'src/cpu.c',
    'src/bus.c',
    'src/joypad.c',
    'src/ppu/ppu.c',
    'src/ppu/dmg_renderer.c',
    'src/ppu/gbc_renderer.c',
    'src/ppu/sgb_renderer.c',
    'src/apu/apu.c',
    'src/apu/io.c',
    'src/apu/square1.c',
    'src/apu/square2.c',
    'src/apu/wave.c',
    'src/apu/noise.c',
    'src/mbc/mbc.c',
    'src/mbc/mbc_0.c',
    'src/mbc/mbc_1.c',
    'src/mbc/mbc_2.c',
    'src/mbc/mbc_3.c',
    'src/mbc/mbc_5.c',
    'src/timers.c',
    'src/serial.c',
    'src/sgb.c',
    'src/accessories/printer.c',
    'src/tables/palette_table.c',
])

# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = [
  '-DBUILDING_LIB',
]

if get_option('pixel-stride') == '8'
  c_flags += '-DGB_PIXEL_STRIDE=8'
elif get_option('pixel-stride') == '16'
  c_flags += '-DGB_PIXEL_STRIDE=16'
elif get_option('pixel-stride') == '32'
  c_flags += '-DGB_PIXEL_STRIDE=32'
endif

# todo: setup the endianness
if host_machine.endian() == 'little'
elif
endif

if get_option('gb-debug') == true
    c_flags += '-DGB_DEBUG=1'
endif

if get_option('gb-builtin-palette') == false
    c_flags += '-DGB_NO_BUILTIN_PALETTE=1'
endif

if get_option('gb-sram-size') == 0
    c_flags += '-DGB_MAX_SRAM_SIZE=0'
elif get_option('gb-sram-size') == 1
    c_flags += '-DGB_MAX_SRAM_SIZE=1'
elif get_option('gb-sram-size') == 2
    c_flags += '-DGB_MAX_SRAM_SIZE=2'
elif get_option('gb-sram-size') == 3
    c_flags += '-DGB_MAX_SRAM_SIZE=3'
elif get_option('gb-sram-size') == 4
    c_flags += '-DGB_MAX_SRAM_SIZE=4'
elif get_option('gb-sram-size') == 5
    c_flags += '-DGB_MAX_SRAM_SIZE=5'
endif

shlib = shared_library(
  'TotalGB', source,
  install : true,
  c_args : [lib_args, c_warnings, c_flags],
  gnu_symbol_visibility : 'hidden',
)

test_sanity_exe = executable(
  'args_test', 'tests/sanity.c',
  link_with : shlib
)

test('sanity', test_sanity_exe)

# Make this library usable as a Meson subproject.
test_dep = declare_dependency(
  include_directories: include_directories('src'),
  link_with : shlib
)

# Make this library usable from the system's
# package manager.
install_headers('src/gb.h', subdir : 'TotalGB')
install_headers('src/types.h', subdir : 'TotalGB')
install_headers('src/tables/palette_table.h', subdir : 'TotalGB/tables')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'TotalGB',
  filebase : 'TotalGB',
  description : '[GB, GBC] emulator core!',
  subdirs : 'TotalGB',
  libraries : shlib,
  version : '0.0.1',
)

# scripts
run_target('cppcheck',
  command : 'scripts/cppcheck.sh')
