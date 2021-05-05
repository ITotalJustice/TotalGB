#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "util.h"

#include "video/interface.h"
#include "audio/interface.h"

#include "../core/gb.h"


typedef struct mgb {
    struct GB_Core* gameboy;

    uint8_t* rom_data;
    size_t rom_size;

    struct SafeString rom_path;

    struct SafeString custom_save_path;
    struct SafeString custom_rtc_path;
    struct SafeString custom_state_path;

    bool rom_loaded;
} mgb_t;


bool mgb_init(mgb_t* self);
void mgb_exit(mgb_t* self);
void mgb_run(mgb_t* self);

bool mgb_load_rom_file(mgb_t* self, const char* path);
bool mgb_load_rom_data(mgb_t* self,
    const char* path, const uint8_t* data, size_t size
);

bool mgb_load_save_file(mgb_t* self, const char* path);
bool mgb_load_rtc_file(mgb_t* self, const char* path);
bool mgb_load_state_file(mgb_t* self, const char* path);

bool mgb_load_save_data(mgb_t* self, const uint8_t* data, size_t size);
bool mgb_load_rtc_data(mgb_t* self, const uint8_t* data, size_t size);
bool mgb_load_state_data(mgb_t* self, const uint8_t* data, size_t size);

bool mgb_write_save_file(mgb_t* self, const char* path);
bool mgb_write_rtc_file(mgb_t* self, const char* path);
bool mgb_write_state_file(mgb_t* self, const char* path);

// all of these return true if it handled the input.
// this may be used later on by [mui]
bool mgb_on_key(mgb_t* self,
    enum VideoInterfaceKey key, bool down
);

bool mgb_on_button(mgb_t* self,
    enum VideoInterfaceButton button, bool down
);

bool mgb_on_axis(mgb_t* self,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
);

#ifdef __cplusplus
}
#endif
