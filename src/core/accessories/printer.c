#include "printer.h"
#include "../gb.h"
#include "../internal.h"

#include <stdio.h>
#include <string.h>

static int file_time = 0;

// the printer is always the slave device, meaning, it never tries to send data
// it *can* send data of course by returning a value once the DMG
// transfers data, however the DMG is always the driver, so transfers happen
// when it decides.

enum StatusByte {
    CHECKSUM_ERROR = 1 << 0,
    CURRENTLY_PRINTING = 1 << 1,
    IMAGE_DATA_FULL = 1 << 2,
    UNPROCESSED_DATA = 1 << 3,
    PACKET_ERROR = 1 << 4,
    PAPER_JAM = 1 << 5,
    OTHER_ERROR = 1 << 6,
    LOW_BATTERY = 1 << 7
};

enum CompressionType {
    NONE = 0,
    RLE = 1,
};

enum MagicNum {
    MAGIC_NUM_0 = 0x88,
    MAGIC_NUM_1 = 0x33,

    MAGIC_NUM_PAIR = (MAGIC_NUM_1 << 8) | MAGIC_NUM_0
};

static void recievie_printer_magic_0(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    printer->magic = data->in_data;
    printer->mode = WAITING_FOR_MAGIC_1;
}

static void recievie_printer_magic_1(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    printer->magic |= data->in_data << 8;

    if (printer->magic == MAGIC_NUM_PAIR) {
        // printf("got magic!\n");
        printer->mode = WAITING_FOR_COMMAND;
    } else {
        printf("[ERROR-PRINTER] invalid magic val: 0x%02X\n", data->in_data);
        printer->mode = WAITING_FOR_MAGIC_0;
    }
}

static void handle_printer_command(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    printer->command = data->in_data;

    switch (printer->command) {
        case INIT: case PRINT: case DATA: case STATUS:
            printer->mode = WAITING_FOR_COMPRESSION;
            break;

        default:
            // if an invalid cmd is passed, reset back to magic.
            printer->mode = WAITING_FOR_MAGIC_0;
            printf("[ERROR-PRINTER] unk command! 0x%02X\n", data->in_data);
    }
}


static void recievie_printer_compression(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    if (data->in_data != NONE && data->in_data != RLE) {
        printf("invalid compression data %u\n", data->in_data);
    }

    if (printer->compression_flag == RLE) {
        printf("uses RLE!\n");
    }

    printer->compression_flag = data->in_data;
    printer->mode = WAITING_FOR_DATA_LENGTH_0;
}

static void recievie_printer_data_length_0(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    printer->data_len = data->in_data;
    printer->mode = WAITING_FOR_DATA_LENGTH_1;
}

static void recievie_printer_data_length_1(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    printer->data_len |= data->in_data << 8;

    // a few checks are needed to be performed to ensure that the
    // the length is valid.
    // however, i am not sure what the printer should do if the len is
    // invalid, IE, len > 0 for INIT cmd.
    if (printer->data_len > 0) {
        printer->mode = RECIEVING_DATA;
    } else {
        printer->mode = WAITING_FOR_CHECKSUM_0;
    }
}

static void recievie_printer_data(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    printer->ram[printer->data_offset] = data->in_data;
    ++printer->data_offset;

    if (printer->data_offset == printer->data_len) {
        // printer->data_offset = 0;
        printer->mode = WAITING_FOR_CHECKSUM_0;
    }
}

static void recievie_printer_checksum_0(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    printer->checksum = data->in_data;
    printer->mode = WAITING_FOR_CHECKSUM_1;
}

static void recievie_printer_checksum_1(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    printer->checksum |= data->in_data << 8;

    // to perform the checksum, loop over all recieved data (not including magic)
    GB_U16 checksum_value = 0;

    // save the header data (cmd, comp, len0, len1)
    checksum_value += printer->command;
    checksum_value += printer->compression_flag;
    checksum_value += printer->data_len;

    // now loop the recieved data (if any)
    for (GB_U16 i = 0; i < printer->data_len; ++i) {
        checksum_value += printer->ram[i];
    }

    // let the DMG know if we fail...
    if (checksum_value == printer->checksum) {
        if (printer->data_offset > 4) {
        printf("checksum match! %u\n", printer->data_offset);
        }printer->status = 0;
    } else {
        // printf("checksum missmatch! got: %u wanted: %u\n", checksum_value, printer->checksum);
        printer->status = 0;
        // printer->status = CHECKSUM_ERROR;
    }

    printer->data_offset = 0;
    printer->mode = ACKNOWLEDGEMENT_0;
}

static void execute_printer_command(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    switch (printer->command) {
        case INIT:
            memset(printer->ram, 0, sizeof(printer->ram));
            printf("executing CMD INIT\n");
            break;

        case PRINT:
            printf("executing CMD PRINT\n");
            printer->print_data = *(struct GB_PrinterPrintData*)printer->ram;
            printf("\tunk: 0x%02X\n", printer->print_data.unk);
            printf("\tmargins lo: 0x%02X\n", printer->print_data.margins & 0xF);
            printf("\tmargins hi: 0x%02X\n", printer->print_data.margins >> 4);
            printf("\tpalette: 0x%02X\n", printer->print_data.palette);
            printf("\texposure: 0x%02X\n", printer->print_data.exposure);
            // if (file_time == 0) {
            //     FILE* f = fopen("print_0.bin", "wb");
            //     fwrite(printer->ram, 1, printer->data_len, f);
            //     fclose(f);
            //     ++file_time;
            // } else {
            //     FILE* f = fopen("print_1.bin", "wb");
            //     fwrite(printer->ram, 1, printer->data_len, f);
            //     fclose(f);
            // }
            
            break;

        case DATA:
            if (printer->data_len == 0) {
                printf("executing CMD empty DATA\n");
            } else {
                // printf("executing CMD DATA\n");
            }
            if (printer->data_len > 4) {
                printf("printing big data %u\n", printer->data_len);
            }
            break;

        case STATUS:
            data->out_data = printer->status;
            // printf("executing CMD STATUS\n");
            break;
    }

    // go back to the start
    printer->mode = WAITING_FOR_MAGIC_0;
}

static void send_printer_acknowledgement_0(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    data->out_data = 0x81;
    printer->mode = ACKNOWLEDGEMENT_1;
}

static void send_printer_acknowledgement_1(struct GB_Printer* printer, struct GB_LinkCableData* data) {
    data->out_data = printer->status;
    execute_printer_command(printer, data);
}

static GB_BOOL printer_cb(void* user_data, struct GB_LinkCableData* data) {
    struct GB_Printer* printer = (struct GB_Printer*)user_data;

    // most commands set the out_data to zero.
    // for the few that don't, the data is manually set the inside the function
    data->out_data = 0;

    switch (printer->mode) {
        // Sync
        case WAITING_FOR_MAGIC_0:
            recievie_printer_magic_0(printer, data);
            break;
        case WAITING_FOR_MAGIC_1:
            recievie_printer_magic_1(printer, data);
            break;

        // Header
        case WAITING_FOR_COMMAND:
            handle_printer_command(printer, data);
            break;
        case WAITING_FOR_COMPRESSION:
            recievie_printer_compression(printer, data);
            break;
        case WAITING_FOR_DATA_LENGTH_0:
            recievie_printer_data_length_0(printer, data);
            break;
        case WAITING_FOR_DATA_LENGTH_1:
            recievie_printer_data_length_1(printer, data);
            break;

        // Data
        case RECIEVING_DATA:
            recievie_printer_data(printer, data);
            break;

        // Checksum
        case WAITING_FOR_CHECKSUM_0:
            recievie_printer_checksum_0(printer, data);
            break;
        case WAITING_FOR_CHECKSUM_1:
            recievie_printer_checksum_1(printer, data);
            break;

        // Verify status
        case ACKNOWLEDGEMENT_0:
            send_printer_acknowledgement_0(printer, data);
            break;
        case ACKNOWLEDGEMENT_1:
            send_printer_acknowledgement_1(printer, data);
            break;
    }

    return GB_TRUE;
}

void GB_connect_printer(struct GB_Data* gb, struct GB_Printer* printer, GB_print_callback_t cb, void* user_data) {
    // if NULL, this is a disconnect, so NULL the link_cb
    // and NULL the user_data
    if (!printer) {
        gb->link_cable = NULL;
        gb->link_cable_user_data = NULL;
    } else {
        printer->print_callback = cb;
        printer->print_callback_user_data = user_data;
        gb->link_cable = printer_cb;
        gb->link_cable_user_data = printer;
    }
}
