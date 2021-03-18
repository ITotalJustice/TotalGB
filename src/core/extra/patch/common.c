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

// static size_t stream_internal_read(
//     void* data, size_t size, size_t nmemb, void* user
// ) {
//     return 0;
// }

// struct StreamInternalR patch_stream_read_setup_from_data2(
//     const uint8_t* data, size_t size
// ) {
//     const struct StreamInternalR internal_data = {
//         .data = data,
//         .offset = 0
//     };
    
//     return internal_data;
// }

// struct PatchStreamR patch_stream_read_setup_from_data(
//     const uint8_t* data, size_t size
// ) {
//     struct StreamInternalR internal_data = {
//         .data = data,
//         .offset = 0
//     };


// }

// struct PatchStreamW patch_stream_write_setup_from_data(
//     uint8_t* data, size_t size
// ) {

// }
