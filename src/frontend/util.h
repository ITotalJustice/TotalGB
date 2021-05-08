#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


// simple macro which silences warnings for stuff that i want to quickly ignore
#define UNUSED(x) (void)x

// same as above [UNUSED], but more clear that this var is yet to be implemented
#define STUB(x) (void)x

// only pass in c-arrays...
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


enum ExtensionType {
    ExtensionType_UNK,
    ExtensionType_ROM,
    ExtensionType_SAVE,
    ExtensionType_RTC,
    ExtensionType_STATE,
    ExtensionType_ZIP,
    ExtensionType_GZIP,
    ExtensionType_7ZIP,
    ExtensionType_RAR,
    ExtensionType_IPS,
    ExtensionType_UPS,
    ExtensionType_BPS,
};

enum ExtensionOffsetType {
    ExtensionOffsetType_FIRST,
    ExtensionOffsetType_LAST
};

// used to return strings from functions, without malloc / free
// thus clearing strings on the stack when out of scope.
struct SafeString {
    char str[0x100];
    bool valid; // set to false if the string is NULL
};


// struct SafeString util_write_to_safe_string(const char* str);

// appends the standard extension to the given string
struct SafeString util_create_save_path(const char* str);
struct SafeString util_create_rtc_path(const char* str);
struct SafeString util_create_state_path(const char* str);

// appends ext extension to str
struct SafeString util_append_extension(
    const char* str, const char* ext, enum ExtensionOffsetType type
);

// returns offset, -1 on error
long util_get_extension_offset(
    const char* str, enum ExtensionOffsetType type
);

// returns the extension
const char* util_get_extension(
    const char* str, enum ExtensionOffsetType type
);

// returns the type based on the extension.
// this is case-insensitive (.zip == .ZIP)
enum ExtensionType util_get_extension_type(
    const char* str, enum ExtensionOffsetType type
);

// simple [return util_get_extension_type() == wanted]
bool util_is_extension(
    const char* str,
    enum ExtensionOffsetType type, enum ExtensionType wanted
);

#ifdef __cplusplus
}
#endif

#endif // _UTIL_H_
