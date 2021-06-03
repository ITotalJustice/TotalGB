#include "gb.h"
#include "internal.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>


// SOURCE: https://gbdev.io/pandocs/#sgb-functions


enum SGB_Commands {
    CMD_PAL01       = 0x00, // Set SGB Palette 0 & 1
    CMD_PAL23       = 0x01, // Set SGB Palette 2 & 3
    CMD_PAL03       = 0x02, // Set SGB Palette 0 & 3
    CMD_PAL12       = 0x03, // Set SGB Palette 1 & 2

    CMD_ATTR_BLK    = 0x04, // "Block" Area Designation Mode
    CMD_ATTR_LIN    = 0x05, // "Line" Area Designation Mode
    CMD_ATTR_DIV    = 0x06, // "Divide" Area Designation Mode
    CMD_ATTR_CHR    = 0x07, // "1CHR" Area Designation Mode

    CMD_SOUND       = 0x08, // Sound On/Off
    CMD_SOU_TRN     = 0x09, // Transfer Sound PRG/DATA

    CMD_PAL_SET     = 0x0A, // Set SGB Palette Indirect
    CMD_PAL_TRN     = 0x0B, // Set System Color Palette Data

    CMD_ATRC_EN     = 0x0C, // Enable/disable Attraction Mode
    CMD_TEST_EN     = 0x0D, // Speed Function
    CMD_ICON_EN     = 0x0E, // SGB Function

    CMD_DATA_SND    = 0x0F, // SUPER NES WRAM Transfer 1
    CMD_DATA_TRN    = 0x10, // SUPER NES WRAM Transfer 2

    CMD_MLT_REG     = 0x11, // Controller 2 Request
    CMD_JUMP        = 0x12, // Set SNES Program Counter

    CMD_CHR_TRN     = 0x13, // Transfer Character Font Data
    CMD_PCT_TRN     = 0x14, // Set Screen Data Color Data
    CMD_ATTR_TRN    = 0x15, // Set Attribute from ATF
    CMD_ATTR_SET    = 0x16, // Set Data to ATF

    CMD_MASK_EN     = 0x17, // Game Boy Window Mask
    CMD_OBJ_TRN     = 0x18, // Super NES OBJ Mode
};

enum SGB_ScreenMask {
    // cancle screen mask!
    SCREEN_MASK_CANCEL          = 0,
    // the screen is fronzen (todo)
    SCREEN_MASK_FREEZE          = 1,
    // the screen is set to all black
    SCREEN_MASK_BLACK_SCREEN    = 2,
    // the screen is set to colour 0
    SCREEN_MASK_BLANK_SCREEN    = 3,
};

enum SGB_TransferState {
    // this happens when both p15 and p14 go lo
    WAITING_FOR_RESET,

    // transfer is in progress
    DATA_TRANSFER,

    // 1-bit (has to be zero) NULL terminated bit
    // sent after the data transfer is done.
    ZERO_BIT
};

struct SGB_HeaderByte {
    uint8_t len : 3;
    uint8_t cmd : 5;
};

// 1-bit is transfered at a time
// because of this, we need to keep track at which bit index we are in!
struct SGB_Packet {
    uint8_t data[16];

    // max value of 128 (for 16-bytes)
    uint8_t bit_index;
};

struct SGB_Tranfer {
    // which state we are in
    enum SGB_TransferState state;

    // the data (and index)
    struct SGB_Packet packets[7];
    uint8_t packet_index;

    // set in the first transfer.
    struct SGB_HeaderByte header_byte;
};

enum MLT_PlayerCount {
    MLT_1_PLAYERS = 0,
    MLT_2_PLAYERS = 1,
    MLT_4_PLAYERS = 3
    // 2-F is not used...
};

// going to dump all sgb stuff here for now until i've
// fully impl the needed SGB commands.
// then i'll restructure this and add it to the main Core struct.
struct SGB_Stub {
    struct {
        // which controller num are we checking
        uint8_t index : 2;
        // how many controllers are we checking
        uint8_t count;
    } mlt;

    struct {
        enum SGB_ScreenMask mask;
    } mask_en;

    // 0-3 are used for the game
    // 4-7 are used for the border only.
    uint16_t palettes[8][16];

    // 4kb
    uint16_t palette_ram[512][4];
};

// for testing.
// will be part of the core struct soon.
static struct SGB_Tranfer transfer = {0};
static struct SGB_Stub sgb_stub = {0};

static void cmd_mlt_reg(struct GB_Core* gb) {
    GB_UNUSED(gb);
    sgb_stub.mlt.count = transfer.packets[0].data[1] & 0x3;

    // i think we always reset the current index?
    switch (sgb_stub.mlt.count) {
        case MLT_1_PLAYERS:
            printf("[SGB] MLT_1_PLAYERS\n");
            break;

        case MLT_2_PLAYERS:
            printf("[SGB] MLT_2_PLAYERS\n");
            break;

        case MLT_4_PLAYERS:
            printf("[SGB] MLT_4_PLAYERS\n");
            break;

        default:
            printf("[SGB] inavlid amount! %u\n", sgb_stub.mlt.count);
            break;
    }

    sgb_stub.mlt.index = 0;
}

static void cmd_mask_en(struct GB_Core* gb) {
    GB_UNUSED(gb);
    sgb_stub.mask_en.mask = transfer.packets[0].data[0] & 0x7;

    switch (sgb_stub.mask_en.mask) {
        case SCREEN_MASK_CANCEL:
            printf("[SGB] SCREEN_MASK_CANCEL\n");
            break;

        case SCREEN_MASK_FREEZE:
            printf("[SGB] SCREEN_MASK_FREEZE\n");
            break;

        case SCREEN_MASK_BLACK_SCREEN:
            printf("[SGB] SCREEN_MASK_BLACK_SCREEN\n");
            break;

        case SCREEN_MASK_BLANK_SCREEN:
            printf("[SGB] SCREEN_MASK_BLANK_SCREEN\n");
            break;

    }
}

static void _cmd_palxx(struct GB_Core* gb, uint8_t p0, uint8_t p1) {
    const uint16_t col1 = *(uint16_t*)(&(transfer.packets[0].data[0x1]));
    const uint16_t col2 = *(uint16_t*)(&(transfer.packets[0].data[0x3]));
    const uint16_t col3 = *(uint16_t*)(&(transfer.packets[0].data[0x5]));
    const uint16_t col4 = *(uint16_t*)(&(transfer.packets[0].data[0x7]));
    const uint16_t col5 = *(uint16_t*)(&(transfer.packets[0].data[0x9]));
    const uint16_t col6 = *(uint16_t*)(&(transfer.packets[0].data[0xB]));
    const uint16_t col7 = *(uint16_t*)(&(transfer.packets[0].data[0xD]));

    const uint8_t array[2] = { p0, p1 };

    GB_UNUSED(col1); GB_UNUSED(col2); GB_UNUSED(col3); GB_UNUSED(col4);
    GB_UNUSED(col5); GB_UNUSED(col6); GB_UNUSED(col7); GB_UNUSED(array);

    // for (size_t i = 0; i < 2; ++i) {
    //     switch (array[i]) {
    //         case 0:
    //             memcpy(gb->palette.BG, pal_array, sizeof(pal_array));
    //             break;

    //         case 1:
    //             memcpy(gb->palette.OBJ0, pal_array, sizeof(pal_array));
    //             break;

    //         case 2:
    //             memcpy(gb->palette.OBJ1, pal_array, sizeof(pal_array));
    //             break;

    //         case 3:
    //             memcpy(gb->palette.OBJ0, pal_array, sizeof(pal_array));
    //             break;
    //     }
    // }

    // refresh the palette cache.
    GB_update_all_colours_gb(gb);
}

static void cmd_pal01(struct GB_Core* gb) {
    _cmd_palxx(gb, 0, 1);
}

static void cmd_pal23(struct GB_Core* gb) {
    _cmd_palxx(gb, 2, 3);
}

static void cmd_pal03(struct GB_Core* gb) {
    _cmd_palxx(gb, 0, 3);
}

static void cmd_pal12(struct GB_Core* gb) {
    _cmd_palxx(gb, 1, 2);
}

static void cmd_pal_set(struct GB_Core* gb) {
    GB_UNUSED(gb);
}

static void execute_cmd(struct GB_Core* gb) {
    switch (transfer.header_byte.cmd) {
        case CMD_PAL01:   // Set SGB Palette 0 & 1
            printf("[SGB] %s\n", "CMD_PAL01");
            cmd_pal01(gb);
            break;

        case CMD_PAL23:   // Set SGB Palette 2 & 3
            printf("[SGB] %s\n", "CMD_PAL23");
            cmd_pal23(gb);
            break;

        case CMD_PAL03:   // Set SGB Palette 0 & 3
            printf("[SGB] %s\n", "CMD_PAL03");
            cmd_pal03(gb);
            break;

        case CMD_PAL12:   // Set SGB Palette 1 & 2
            printf("[SGB] %s\n", "CMD_PAL12");
            cmd_pal12(gb);
            break;

        case CMD_ATTR_BLK:// "Block" Area Designation Mode
            printf("[SGB] %s\n", "CMD_ATTR_BLK");
            // assert(0 && "CMD_ATTR_BLK");
            break;

        case CMD_ATTR_LIN:// "Line" Area Designation Mode
            printf("[SGB] %s\n", "CMD_ATTR_LIN");
            // assert(0 && "CMD_ATTR_LIN");
            break;

        case CMD_ATTR_DIV:// "Divide" Area Designation Mode
            printf("[SGB] %s\n", "CMD_ATTR_DIV");
            // assert(0 && "CMD_ATTR_DIV");
            break;

        case CMD_ATTR_CHR:// "1CHR" Area Designation Mode
            printf("[SGB] %s\n", "CMD_ATTR_CHR");
            // assert(0 && "CMD_ATTR_CHR");
            break;

        case CMD_SOUND:   // Sound On/Off
            printf("[SGB] %s\n", "CMD_SOUND");
            // assert(0 && "CMD_SOUND");
            break;

        case CMD_SOU_TRN: // Transfer Sound PRG/DATA
            printf("[SGB] %s\n", "CMD_SOU_TRN");
            // assert(0 && "CMD_SOU_TRN");
            break;

        case CMD_PAL_SET: // Set SGB Palette Indirect
            printf("[SGB] %s\n", "CMD_PAL_SET");
            cmd_pal_set(gb);
            break;

        case CMD_PAL_TRN: // Set System Color Palette Data
            printf("[SGB] %s\n", "CMD_PAL_TRN");
            // assert(0 && "CMD_PAL_TRN");
            break;

        case CMD_ATRC_EN: // Enable/disable Attraction Mode
            printf("[SGB] %s\n", "CMD_ATRC_EN");
            // assert(0 && "CMD_ATRC_EN");
            break;

        case CMD_TEST_EN: // Speed Function
            printf("[SGB] %s\n", "CMD_TEST_EN");
            // assert(0 && "CMD_TEST_EN");
            break;

        case CMD_ICON_EN: // SGB Function
            printf("[SGB] %s\n", "CMD_ICON_EN");
            // assert(0 && "CMD_ICON_EN");
            break;

        case CMD_DATA_SND:// SUPER NES WRAM Transfer 1
            printf("[SGB] %s\n", "CMD_DATA_SND");
            // assert(0 && "CMD_DATA_SND");
            break;

        case CMD_DATA_TRN:// SUPER NES WRAM Transfer 2
            printf("[SGB] %s\n", "CMD_DATA_TRN");
            // assert(0 && "CMD_DATA_TRN");
            break;

        case CMD_MLT_REG: // Controller 2 Request
            printf("[SGB] %s\n", "CMD_MLT_REG");
            cmd_mlt_reg(gb);
            break;

        case CMD_JUMP:    // Set SNES Program Counter
            printf("[SGB] %s\n", "CMD_JUMP");
            // assert(0 && "CMD_JUMP");
            break;

        case CMD_CHR_TRN: // Transfer Character Font Data
            printf("[SGB] %s\n", "CMD_CHR_TRN");
            // assert(0 && "CMD_CHR_TRN");
            break;

        case CMD_PCT_TRN: // Set Screen Data Color Data
            printf("[SGB] %s\n", "CMD_PCT_TRN");
            // assert(0 && "CMD_PCT_TRN");
            break;

        case CMD_ATTR_TRN:// Set Attribute from ATF
            printf("[SGB] %s\n", "CMD_ATTR_TRN");
            // assert(0 && "CMD_ATTR_TRN");
            break;

        case CMD_ATTR_SET:// Set Data to ATF
            printf("[SGB] %s\n", "CMD_ATTR_SET");
            // assert(0 && "CMD_ATTR_SET");
            break;

        case CMD_MASK_EN: // Game Boy Window Mask
            printf("[SGB] %s\n", "CMD_MASK_EN");
            cmd_mask_en(gb);
            break;

        case CMD_OBJ_TRN: // Super NES OBJ Mode
            printf("[SGB] %s\n", "CMD_OBJ_TRN");
            // assert(0 && "CMD_OBJ_TRN");
            break;

        default:
            printf("UNK SGB CMD: 0x%02X\n", transfer.header_byte.cmd);
            break;
    }
}


bool SGB_handle_joyp_read(const struct GB_Core* gb, uint8_t* data_out) {
    if (sgb_stub.mlt.count != MLT_1_PLAYERS && (IO_JYP & 0x30) == 0x30) {
        // static const uint8_t PLAYER_VALUE[4] = {
        //     0x0F, 0x0E, /*0x0D, 0x0C,*/ 0x00, 0x00
        // };

        *data_out = 0xF0 | 0x0E;
        return true;
    }

    return false;
}

void SGB_handle_joyp_write(struct GB_Core* gb, uint8_t value) {
    // bit-5 = 1. bit-4 = 0.
    const bool p15 = (value & 0x20) > 0;
    const bool p14 = (value & 0x10) > 0;

    // can only be 1 if p15 is lo and p14 is hi
    const bool bit_value = p15 == 0 && p14 == 1;

    if ((p14 && p15) || (!p14 && !p15 && transfer.state != 0)) {
        return;
    }

    // check which state we are in
    switch (transfer.state) {
        case WAITING_FOR_RESET:
            // both pins need to be lo!
            if (!p14 && !p15) {
                transfer.state = DATA_TRANSFER;

                if (transfer.header_byte.len > 0) {

                }
                else {
                    transfer.packet_index = 0;
                    memset(&transfer.header_byte, 0, sizeof(transfer.header_byte));
                    memset(transfer.packets, 0, sizeof(transfer.packets));
                }
            }
            break;


        case DATA_TRANSFER:
            // basically set 1-bit at a time, LSB.
            transfer.packets[transfer.packet_index].data[transfer.packets[transfer.packet_index].bit_index >> 3] |= bit_value << (transfer.packets[transfer.packet_index].bit_index & 7);
            ++transfer.packets[transfer.packet_index].bit_index;

            // check if we are done!
            if (transfer.packets[transfer.packet_index].bit_index == (16 * 8)) {
                transfer.state = ZERO_BIT;
            }
            break;

        case ZERO_BIT:
            // i am not sure if this HAS to be zero.
            // docs says it does, but some games (yellow) can write a 1 here.
            // rejecting it seems to allow the game to continue on just fine...
            if (bit_value != 0) {
                return;
            }

            // if we don't have a len set, then this means that this
            // transfer was a new one, ie, one that sets the len and
            // command!
            if (transfer.header_byte.len == 0) {
                // set len
                transfer.header_byte.len |= (transfer.packets[0].data[0] & 0x7);
                // set cmd
                transfer.header_byte.cmd |= (transfer.packets[0].data[0] >> 3) & 0x3F;
            }

            // len is inclusive, so 1 transfer will have a len of 1
            --transfer.header_byte.len;

            // check the len, if 0, no more transfers
            if (transfer.header_byte.len == 0) {
                execute_cmd(gb);
                transfer.state = WAITING_FOR_RESET;
            }
            else {
                // we are saving multiple packets!
                ++transfer.packet_index;
            }
            break;
    }
}
