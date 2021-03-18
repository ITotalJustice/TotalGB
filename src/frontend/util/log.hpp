#pragma once

#include <string>
#if GB_LOG
#include <cstdio>
#include <cstdarg>
#endif

namespace mgb::log {

inline void log(const char c) {
    #if GB_LOG
    putchar(c);
    #endif
}

inline void log(const char* str, ...) {
    #if GB_LOG
    va_list v;
    va_start(v, str);
    vprintf(str, v);
    va_end(v);
    #endif
}

inline void log(const std::string& str) {
    #if GB_LOG
    printf(str.c_str());
    #endif
}

template<typename T>
T errReturn(T result, T wanted, const char* str, ...) {
    #if GB_LOG
    if (result != wanted) {
        va_list v;
        va_start(v, str);
        vfprintf(stderr, str, v);
        va_end(v);
    }
    #endif
    return result;
}

template<typename T>
T errReturn(T result, const char* str, ...) {
    #if GB_LOG
    va_list v;
    va_start(v, str);
    vfprintf(stderr, str, v);
    va_end(v);
    #endif
    return result;
}

template<typename T>
T errReturn(T result, T wanted, const std::string& str) {
    #if GB_LOG
    if (result != wanted) {
        fprintf(stderr, str.c_str());
    }
    #endif
    return result;
}

template<typename T>
T errReturn(T r, const std::string& str) {
    #if GB_LOG
    fprintf(stderr, str.c_str());
    #endif
    return r;
}

} // namespace mgb::log
