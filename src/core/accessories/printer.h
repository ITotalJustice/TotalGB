#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/types.h"

enum GB_PrinterMode {
    // Sync
    WAITING_FOR_MAGIC_0,
    WAITING_FOR_MAGIC_1,

    // Header
    WAITING_FOR_COMMAND,
    WAITING_FOR_COMPRESSION,
    WAITING_FOR_DATA_LENGTH_0,
    WAITING_FOR_DATA_LENGTH_1,

    // Data
    RECIEVING_DATA,

    // Checksum
    WAITING_FOR_CHECKSUM_0,
    WAITING_FOR_CHECKSUM_1,

    // check on the status of the printer after cmd execution.
    ACKNOWLEDGEMENT_0,
    ACKNOWLEDGEMENT_1,
};

enum GB_PrinterCommand {
    INIT = 0x1,
    PRINT = 0x2,
    DATA = 0x4,
    STATUS = 0xF
};

enum PrinterDataType {
    PRINTER_DATA_TYPE_INFO,
    PRINTER_DATA_TYPE_DATA,
    PRINTER_DATA_TYPE_END_PAGE,
};

struct PrinterDataInfo {
    uint16_t data_length;

    uint8_t margin_lo : 4;
    uint8_t margin_hi : 4;

    uint8_t exposure;
};

struct PrinterCallbackData {
    enum PrinterDataType type;

    union {
        struct PrinterDataInfo info;
    } data;
};

typedef void(*GB_print_callback_t)(void* user, struct PrinterCallbackData* data);

struct GB_PrinterPrintData {
    uint8_t unk;
    uint8_t margins;
    uint8_t palette;
    uint8_t exposure;
};

struct GB_Printer {
    // set by either the user or the connect printer function
    // called during a PRINT command.
    GB_print_callback_t print_callback;
    void* print_callback_user_data;

    //sync
    uint16_t magic;

    // header
    uint8_t command;
    uint8_t compression_flag;
    uint16_t data_len;

    // data
    uint8_t ram[0x2000]; // 8-KiB
    
    // checksum
    uint16_t checksum;

    uint8_t status;

    enum GB_PrinterMode mode;

    struct GB_PaletteEntry palette;
    struct GB_PrinterPrintData print_data;

    // used when recieving data, offset into the ram array
    uint16_t data_offset;
};

// internally this saves the passed in *printer* pointer, so be sure that
// this is a VALID pointer and not temporary mem that will go out
// of scope!

// the cb is called when a print command is executed.
// first, an info type data will be sent to the callback, this will tell you
// what data to expect so you can allocate enough memory for it.
// then, a single line will be sent at a time.
// please pay attention to the margins sent!
// this will let you know the x offset and max width in the actual pixel data!
// passing the cb as NULL IS valid, this will just execute print.

// passing NULL will disconect the printer from the GB and also
// set the link_cable callback to NULL.
void GB_connect_printer(struct GB_Core* gb, struct GB_Printer* printer, GB_print_callback_t cb, void* user_data);

#ifdef __cplusplus
}
#endif
