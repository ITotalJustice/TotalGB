#ifndef _GB_IO_READ_TABLE_H_
#define _GB_IO_READ_TABLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


static const uint8_t IO_READ_TABLE[0x80] =
{
    [0x00] = 0xC0, // JOYP
    [0x01] = 0x00, // SB
    [0x02] = 0x7C, // SC
    [0x03] = 0xFF, // DIV LOWER
    [0x04] = 0x00, // DIV UPPER
    [0x05] = 0x00, // TIMA
    [0x06] = 0x00, // TMA
    [0x07] = 0xF8, // TAC
    [0x08] = 0xFF,
    [0x09] = 0xFF,
    [0x0A] = 0xFF,
    [0x0B] = 0xFF,
    [0x0C] = 0xFF,
    [0x0D] = 0xFF,
    [0x0E] = 0xFF,
    [0x0F] = 0xC0, // IF

    [0x10] = 0x80, // NR10
    [0x11] = 0x3F, // NR11
    [0x12] = 0x00, // NR12
    [0x13] = 0xFF, // NR13
    [0x14] = 0xBF, // NR14
    [0x15] = 0xFF,
    [0x16] = 0x3F, // NR21
    [0x17] = 0x00, // NR22
    [0x18] = 0xFF, // NR23
    [0x19] = 0xBF, // NR24
    [0x1A] = 0x7F, // NR30
    [0x1B] = 0xFF, // NR31
    [0x1C] = 0x95, // NR32
    [0x1D] = 0xFF, // NR33
    [0x1E] = 0xBF, // NR34
    [0x1F] = 0xFF,

    [0x20] = 0xFF, // NR41
    [0x21] = 0x00, // NR42
    [0x22] = 0x00, // NR43
    [0x23] = 0xBF, // NR44
    [0x24] = 0x00, // NR50
    [0x25] = 0x00, // NR51
    [0x26] = 0x70, // NR52
    [0x27] = 0xFF,
    [0x28] = 0xFF,
    [0x29] = 0xFF,
    [0x2A] = 0xFF,
    [0x2B] = 0xFF,
    [0x2C] = 0xFF,
    [0x2D] = 0xFF,
    [0x2E] = 0xFF,
    [0x2F] = 0xFF,

    // todo: this can be read back as 0xFF or the actual value...
    [0x30] = 0xFF, // WAVE RAM
    [0x31] = 0xFF, // WAVE RAM
    [0x32] = 0xFF, // WAVE RAM
    [0x33] = 0xFF, // WAVE RAM
    [0x34] = 0xFF, // WAVE RAM
    [0x35] = 0xFF, // WAVE RAM
    [0x36] = 0xFF, // WAVE RAM
    [0x37] = 0xFF, // WAVE RAM
    [0x38] = 0xFF, // WAVE RAM
    [0x39] = 0xFF, // WAVE RAM
    [0x3A] = 0xFF, // WAVE RAM
    [0x3B] = 0xFF, // WAVE RAM
    [0x3C] = 0xFF, // WAVE RAM
    [0x3D] = 0xFF, // WAVE RAM
    [0x3E] = 0xFF, // WAVE RAM
    [0x3F] = 0xFF, // WAVE RAM

    [0x40] = 0x00, // LCDC
    [0x41] = 0x80, // STAT
    [0x42] = 0x00, // SCY
    [0x43] = 0x00, // SCX
    [0x44] = 0x00, // LY
    [0x45] = 0x00, // LYC
    [0x46] = 0xFF, // DMA
    [0x47] = 0x00, // BGP
    [0x48] = 0x00, // OPB0
    [0x49] = 0x00, // OPB1
    [0x4A] = 0x00, // WY
    [0x4B] = 0x00, // WX
    [0x4C] = 0xFF,
    [0x4D] = 0x7E, // KEY1 [GBC]
    [0x4E] = 0xFF,
    [0x4F] = 0xFE,

    [0x50] = 0xFF,
    [0x51] = 0x00, // HDMA1 [GBC]
    [0x52] = 0x00, // HDMA2 [GBC]
    [0x53] = 0x00, // HDMA3 [GBC]
    [0x54] = 0x00, // HDMA4 [GBC]
    [0x55] = 0x00, // HDMA5 [GBC]
    [0x56] = 0x3E, // RP [GBC]
    [0x57] = 0xFF,
    [0x58] = 0xFF,
    [0x59] = 0xFF,
    [0x5A] = 0xFF,
    [0x5B] = 0xFF,
    [0x5C] = 0xFF,
    [0x5D] = 0xFF,
    [0x5E] = 0xFF,
    [0x5F] = 0xFF,

    [0x60] = 0xFF,
    [0x61] = 0xFF,
    [0x62] = 0xFF,
    [0x63] = 0xFF,
    [0x64] = 0xFF,
    [0x65] = 0xFF,
    [0x66] = 0xFF,
    [0x67] = 0xFF,
    [0x68] = 0x40, // BCPS [GBC]
    [0x69] = 0x00, // BCPD [GBC]
    [0x6A] = 0x40, // OCPS [GBC]
    [0x6B] = 0x00, // OCPD [GBC]
    [0x6C] = 0xFE, // OPRI [GBC-MODE] 
    [0x6D] = 0xFF,
    [0x6E] = 0xFF,
    [0x6F] = 0xFF,

    [0x70] = 0xF8, // SVBK [GBC]
    [0x71] = 0xFF,
    [0x72] = 0x00, // UNK [GBC]
    [0x73] = 0x00, // UNK [GBC]
    [0x74] = 0x00, // UNK [GBC-MODE]
    [0x75] = 0x8F, // UNK [GBC]
    [0x76] = 0x00, // UNK [GBC]
    [0x77] = 0x00, // UNK [GBC]
    [0x78] = 0xFF,
    [0x79] = 0xFF,
    [0x7A] = 0xFF,
    [0x7B] = 0xFF,
    [0x7C] = 0xFF,
    [0x7D] = 0xFF,
    [0x7E] = 0xFF,
    [0x7F] = 0xFF,
};

#ifdef __cplusplus
extern "C" {
#endif

#endif //_GB_IO_READ_TABLE_H_
