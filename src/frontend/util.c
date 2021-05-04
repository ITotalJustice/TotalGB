#include "util.h"

#include <string.h>
#include <ctype.h>


// very basic case-insensitive compare
static int util_strcasecmp(const char* const a, const char* const b) {
    for (size_t i = 0; /* forever */ ; ++i) {
        // something is NULL, so we are at the end
        if (a[i] == '\0' || b[i] == '\0') {
            // if both are NULL, then we have a match!
            if (a[i] == b[i]) {
                return 0;
            }

            // otherwise, no match, return the corrrect value
            if (a[i] < b[i]) {
                return -1;
            }
            else {
                return +1;
            }
        }

        // no match, return correct value
        if (toupper(a[i]) != toupper(b[i])) {
            if (a[i] < b[i]) {
                return -1;
            }
            else {
                return +1;
            }
        }
    }
}

struct SafeString util_append_extension(
    const char* str, const char* ext, enum ExtensionOffsetType type
) {
    struct SafeString safe_string = {0};

    if (!str || !ext) {
        return safe_string;
    }

    size_t append_offset = 0;

    const size_t str_len = strlen(str);
    const size_t ext_len = strlen(ext);

    if (!str_len || !ext_len) {
        return safe_string;
    }

    const long ext_offset = util_get_extension_offset(str, type);

    if (ext_offset <= 0) {
        append_offset = str_len;
    }
    else {
        append_offset = (size_t)ext_offset;
    }

    // check that this fits okay, +2 for '.' and at least 1 char after
    if (append_offset + 2 > sizeof(safe_string.str)) {
        return safe_string;
    }

    strncpy(safe_string.str, str, append_offset);

    // check if the ext has the . prefix, if not, append it
    if (ext[0] != '.') {
        safe_string.str[append_offset] = '.';
        ++append_offset;
    }

    // we have the final size now, check if it will all fit!
    if (append_offset + ext_len > sizeof(safe_string.str)) {
        return safe_string;
    }

    strcpy(safe_string.str + append_offset, ext);
    safe_string.valid = true;

    return safe_string;
}

struct SafeString util_create_save_path(const char* str) {
    return util_append_extension(
        str, ".sav", ExtensionOffsetType_FIRST
    );
}

struct SafeString util_create_rtc_path(const char* str) {
    return util_append_extension(
        str, ".rtc", ExtensionOffsetType_FIRST
    );
}

struct SafeString util_create_state_path(const char* str) {
    return util_append_extension(
        str, ".state", ExtensionOffsetType_FIRST
    );
}

long util_get_extension_offset(
    const char* str, enum ExtensionOffsetType type
) {
    if (!str) {
        return -1;
    }

    const char* ext = NULL;

    // NOTE: this needs be a more sophisticated to handle
    // tar.xz files, and other multi-extension files.
    switch (type) {
        case ExtensionOffsetType_FIRST: ext = strchr(str, '.'); break;
        case ExtensionOffsetType_LAST: ext = strrchr(str, '.'); break;
    }

    if (!ext) {
        return -1;
    }

    // calculate the difference!
    return ext - str;
}

const char* util_get_extension(
    const char* str, enum ExtensionOffsetType type
) {
    const long offset = util_get_extension_offset(str, type);

    if (offset <= 0) {
        return NULL;
    }

    const size_t len = strlen(str + offset);

    if (len == 0) {
        return NULL;
    }

    return str + offset;
}

enum ExtensionType util_get_extension_type(
    const char* str, enum ExtensionOffsetType type
) {
    const char* ext = util_get_extension(str, type);

    if (!ext) {
        return ExtensionType_UNK;
    }

    const struct ExtPair {
        const char* const ext;
        enum ExtensionType type;
    } ext_pairs[] = {
        // [roms]
        { ".gb", ExtensionType_ROM },
        { ".gbc", ExtensionType_ROM },
        { ".cgb", ExtensionType_ROM },
        { ".sgb", ExtensionType_ROM },
        { ".dmg", ExtensionType_ROM },

        // [zip]
        { ".zip", ExtensionType_ZIP },
        { ".gzip", ExtensionType_GZIP },
        { ".7zip", ExtensionType_7ZIP },
        { ".rar", ExtensionType_RAR },

        // [save]
        { ".save", ExtensionType_SAVE },
        { ".rtc", ExtensionType_RTC },
        { ".state", ExtensionType_STATE },

        // [patch]
        { ".ips", ExtensionType_IPS },
        { ".ups", ExtensionType_UPS },
        { ".bps", ExtensionType_BPS },
    };

    for (size_t i = 0; i < ARRAY_SIZE(ext_pairs); ++i) {
        if (!util_strcasecmp(ext, ext_pairs[i].ext)) {
            return ext_pairs[i].type;
        }
    }

    // no match!
    return ExtensionType_UNK;
}

bool util_is_extension(
    const char* str,
    enum ExtensionOffsetType type, enum ExtensionType wanted
) {
    return wanted == util_get_extension_type(str, type);
}
