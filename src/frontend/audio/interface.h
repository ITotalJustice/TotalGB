#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


struct AudioInterface {
    void* _private;

    void (*quit)(
        void* _private
    );
};

#ifdef __cplusplus
}
#endif
