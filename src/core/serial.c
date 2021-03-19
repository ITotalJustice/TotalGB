#include "gb.h"
#include "internal.h"

#include <assert.h>
#include <stdio.h>


static GB_BOOL GB_has_link_cable(const struct GB_Data* gb) {
    return gb->link_cable != NULL;
}

static GB_BOOL GB_is_serial_client(const struct GB_Data* gb) {
    return (IO_SC & 0x80) == 0x80;
}

static GB_BOOL GB_is_serial_host(const struct GB_Data* gb) {
    return (IO_SC & 0x81) == 0x81;
}

static GB_BOOL GB_builtin_link_cable_cb(struct GB_Data* host_gb, void* user_data, GB_U8 in_data, GB_U8* out_data) {
    assert(host_gb && user_data && out_data);

    // this is *this* instance of the gb struct, not the host!
    struct GB_Data* gb = (struct GB_Data*)user_data;

    // check if we have a link cable connected first!
    if (!GB_has_link_cable(gb)) {
        return GB_FALSE;
    }

    // we are here so we have a link cable, check if we want data or not...
    // NOTE: according to the docs, this is optional!
    // however, in gen1 pokemon, this is the correct behaviour.
    if (!GB_is_serial_client(gb)) {
        return GB_FALSE;
    }

    // we have a cable and we want data, now fill the output!
    *out_data = IO_SB;
    IO_SB = in_data;
    IO_SC &= ~(0x80);
    GB_enable_interrupt(gb, GB_INTERRUPT_SERIAL);

    return GB_TRUE;
}

void GB_connect_link_cable_builtin(struct GB_Data* gb) {
    assert(gb);

    // if we already have a callback set, warn the user,
    // just in case...
    if (GB_has_link_cable(gb)) {
        printf("[WARN] overwriting link_cable callback with builtin!\n");
    }

    // the user_data is the gb struct itself.
    GB_connect_link_cable(gb, GB_builtin_link_cable_cb, gb);
}

GB_U8 GB_serial_sb_read(const struct GB_Data* gb) {
    // check if we have a serial cable connected
    if (GB_has_link_cable(gb) == GB_FALSE) {
        return 0xFF;
    }

    return IO_SB;
}

void GB_serial_sc_write(struct GB_Data* gb, const GB_U8 data) {
    // we always set the byte regardless if connected or not...
    IO_SC = data;


    // check if we have a serial cable connected and if we are host
    if (!GB_is_serial_host(gb) || !GB_has_link_cable(gb)) {
        return;
    }

    printf("SC = 0x81, start transfer!\n");

    // if we are here, we have a link cable connected!
    GB_U8 data_out;
    GB_BOOL result;

    // perform the transfer...
    result = gb->link_cable(gb, gb->link_cable_user_data, IO_SB, &data_out);
    
    if (result == GB_TRUE) {
        // if the transfer was successful, then we have to set IO_SB to
        // the data_out, clear bit-7 of IO_SC and then fire an
        // SERIAL_INTERRUPT.
        IO_SB = data_out;
        IO_SC &= ~(0x80);
        GB_enable_interrupt(gb, GB_INTERRUPT_SERIAL);

    } else {
        // failed to do a transfer, or was rejected...
        // TODO: throw an error to the user (just in case!)
    }
}
