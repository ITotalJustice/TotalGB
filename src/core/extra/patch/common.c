#include "common.h"

struct StreamInternalR {
    const uint8_t* data;
    size_t offset;
};

struct StreamInternalW {
    uint8_t* data;
    size_t offset;
};

struct StreamInternalRW {
    uint8_t* data;
    size_t offset;
};
