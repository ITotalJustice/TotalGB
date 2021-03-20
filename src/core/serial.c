#include "gb.h"
#include "internal.h"

#include <assert.h>
#include <stdio.h>

// SOURCE: https://gbdev.io/pandocs/#serial-data-transfer

static GB_BOOL GB_has_link_cable(const struct GB_Data* gb) {
    return gb->link_cable != NULL;
}

static GB_BOOL GB_is_serial_client(const struct GB_Data* gb) {
    return (IO_SC & 0x80) == 0x80;
}

static GB_BOOL GB_is_serial_host(const struct GB_Data* gb) {
    return (IO_SC & 0x81) == 0x81;
}

// called when master sends data to slave
static GB_BOOL GB_trade(struct GB_Data* gb, struct GB_LinkCableData* data) {
    // we are here so we have a link cable, check if we want data or not...
    // NOTE: according to the docs, this is optional!
    // however, in gen1 pokemon, this is the correct behaviour.
    if (!GB_is_serial_client(gb)) {
        printf("is not client, 0x%02X\n", IO_SC);
        return GB_FALSE;
    }

    // we have a cable and we want data, now fill the output!
    data->out_data = IO_SB;
    IO_SB = data->in_data;
    IO_SC &= ~(0x80);
    GB_enable_interrupt(gb, GB_INTERRUPT_SERIAL);

    return GB_TRUE;
}

static GB_BOOL GB_builtin_link_cable_cb(struct GB_Data* host_gb, void* user_data, struct GB_LinkCableData* data) {
    assert(host_gb && data);

    // this is *this* instance of the gb struct, not the host!
    struct GB_Data* gb = (struct GB_Data*)user_data;

    // check if we have a link cable connected first!
    if (!GB_has_link_cable(gb)) {
        printf("no link cable connected\n");
        return GB_FALSE;
    }

    switch (data->type) {
        case GB_LINK_TRANSFER_TYPE_SLAVE_SET:
        // todo:
            return GB_TRUE;

        case GB_LINK_TRANSFER_TYPE_DATA:
            return GB_trade(gb, data);
    }

    // should be impossible to get here!
    assert(0);
    return GB_FALSE;
}

void GB_connect_link_cable_builtin(struct GB_Data* gb, struct GB_Data* gb2) {
    assert(gb);

    // if we already have a callback set, warn the user,
    // just in case...
    if (GB_has_link_cable(gb)) {
        printf("[WARN] overwriting link_cable callback with builtin!\n");
    }

    // the user_data is the gb struct itself.
    GB_connect_link_cable(gb, GB_builtin_link_cable_cb, gb2);
}

GB_U8 GB_serial_sb_read(const struct GB_Data* gb) {
    // check if we have a serial cable connected
    if (GB_has_link_cable(gb) == GB_FALSE) {
        printf("not link cable on sb read\n");
        return 0xFF;
    }

    return IO_SB;
}

void GB_serial_sc_write(struct GB_Data* gb, const GB_U8 data) {
    // we always set the byte regardless if connected or not...
    IO_SC = data;

    // check if we have a serial cable connected and if we are host
    if (!GB_has_link_cable(gb)) {
        printf("no link cable in sc write! value: 0x%02X\n", data);
        return;
    }

    if ((GB_is_serial_host(gb) || gb->is_master) == GB_FALSE) {
        printf("not host or master! value: 0x%02X\n", data);
        return;
    }

    // check if we are not already host, if so, tell the other gb
    if (!gb->is_master && GB_is_serial_host(gb)) {
        struct GB_LinkCableData link_data = {0};
        link_data.type = GB_LINK_TRANSFER_TYPE_SLAVE_SET;

        if (gb->link_cable(gb, gb->link_cable_user_data, &link_data)) {
            gb->is_master = GB_TRUE;
        } else {
            printf("failed to set master!\n");
        }

        return;
    }

    // if we are here, we have a link cable connected!
    printf("SC = 0x81, start transfer!\n");

    // perform the transfer...
    struct GB_LinkCableData link_data = {0};
    link_data.type = GB_LINK_TRANSFER_TYPE_DATA;
    link_data.in_data = IO_SB;
    
    if (gb->link_cable(gb, gb->link_cable_user_data, &link_data)) {
        // if the transfer was successful, then we have to set IO_SB to
        // the data_out, clear bit-7 of IO_SC and then fire an
        // SERIAL_INTERRUPT.
        IO_SB = link_data.out_data;
        IO_SC &= ~(0x80);
        GB_enable_interrupt(gb, GB_INTERRUPT_SERIAL);

    } else {
        // failed to do a transfer, or was rejected...
        // TODO: throw an error to the user (just in case!)
        printf("failed to do transfer...\n");
    }
}
