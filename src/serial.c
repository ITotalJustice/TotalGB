#include "gb.h"
#include "internal.h"

#if GB_SRC_INCLUDE

#include <assert.h>
#include <stdio.h>

// SOURCE: https://gbdev.io/pandocs/#serial-data-transfer

static bool GB_has_link_cable(const struct GB_Core* gb) {
    return gb->link_cable != NULL;
}

static bool GB_is_serial_client(const struct GB_Core* gb) {
    return (IO_SC & 0x80) == 0x80;
}

static bool GB_is_serial_host(const struct GB_Core* gb) {
    return (IO_SC & 0x81) == 0x81;
}

// called when master sends data to slave
static bool GB_trade(struct GB_Core* gb, struct GB_LinkCableData* data) {
    // we are here so we have a link cable, check if we want data or not...
    // NOTE: according to the docs, this is optional!
    // however, in gen1 pokemon, this is the correct behaviour.
    if (!GB_is_serial_client(gb)) {
        printf("is not client, 0x%02X\n", IO_SC);
        return false;
    }

    // we have a cable and we want data, now fill the output!
    data->out_data = IO_SB;
    IO_SB = data->in_data;
    IO_SC &= ~(0x80);
    GB_enable_interrupt(gb, GB_INTERRUPT_SERIAL);

    return true;
}

static bool GB_builtin_link_cable_cb(void* user_data, struct GB_LinkCableData* data) {
    assert(data);

    // this is *this* instance of the gb struct, not the host!
    struct GB_Core* gb = (struct GB_Core*)user_data;

    // check if we have a link cable connected first!
    if (!GB_has_link_cable(gb)) {
        printf("no link cable connected\n");
        return false;
    }

    switch (data->type) {
        case GB_LINK_TRANSFER_TYPE_SLAVE_SET:
        // todo:
            return true;

        case GB_LINK_TRANSFER_TYPE_DATA:
            return GB_trade(gb, data);
    }

    // should be impossible to get here!
    assert(0);
    return false;
}

void GB_connect_link_cable(struct GB_Core* gb, GB_serial_transfer_t cb, void* user_data) {
    gb->link_cable = cb;
    gb->link_cable_user_data = user_data;
}

void GB_connect_link_cable_builtin(struct GB_Core* gb, struct GB_Core* gb2) {
    assert(gb);

    // if we already have a callback set, warn the user,
    // just in case...
    if (GB_has_link_cable(gb)) {
        printf("[WARN] overwriting link_cable callback with builtin!\n");
    }

    // the user_data is the gb struct itself.
    GB_connect_link_cable(gb, GB_builtin_link_cable_cb, gb2);
}

uint8_t GB_serial_sb_read(const struct GB_Core* gb) {
    // check if we have a serial cable connected
    if (GB_has_link_cable(gb) == false) {
        // printf("not link cable on sb read\n");
        return 0xFF;
    }

    return IO_SB;
}

void GB_serial_sc_write(struct GB_Core* gb, const uint8_t data) {
    // we always set the byte regardless if connected or not...
    IO_SC = data;

    // check if we have a serial cable connected and if we are host
    if (!GB_has_link_cable(gb)) {
        // printf("no link cable in sc write! value: 0x%02X SB: 0x%02X\n", data, IO_SB);
        return;
    }

#if 0
    // check if we are not already host, if so, tell the other gb
    if (!gb->is_master && GB_is_serial_host(gb)) {
        struct GB_LinkCableData link_data = {0};
        link_data.type = GB_LINK_TRANSFER_TYPE_SLAVE_SET;

        if (gb->link_cable(gb, gb->link_cable_user_data, &link_data)) {
            gb->is_master = true;
        } else {
            printf("failed to set master!\n");
        }

        return;
    }

#else

    // check if we are not already host, if so, tell the other gb
    if (GB_is_serial_host(gb) == false) {
        // printf("not host or master! value: 0x%02X SB: 0x%02X\n", data, IO_SB);
        return;
    }
#endif

    // if we are here, we have a link cable connected!

    // perform the transfer...
    struct GB_LinkCableData link_data = {0};
    link_data.type = GB_LINK_TRANSFER_TYPE_DATA;
    link_data.in_data = IO_SB;

    if (gb->link_cable(gb->link_cable_user_data, &link_data)) {
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

#endif //GB_SRC_INCLUDE
