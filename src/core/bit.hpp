#pragma once

#include <cstdint>
#include <cassert>
#include <concepts>

#ifdef __ARM_ACLE
#include <arm_acle.h>
#endif

namespace bit {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

template <typename T>
concept IntV = std::is_integral_v<T>;

template <IntV T> [[nodiscard]]
consteval auto get_mask() -> T {
    if constexpr(sizeof(T) == sizeof(u8)) {
        return 0xFF;
    }
    if constexpr(sizeof(T) == sizeof(u16)) {
        return 0xFF'FF;
    }
    if constexpr(sizeof(T) == sizeof(u32)) {
        return 0xFF'FF'FF'FF;
    }
    if constexpr(sizeof(T) == sizeof(u64)) {
        return 0xFF'FF'FF'FF'FF'FF'FF'FF;
    }
}

[[nodiscard]]
constexpr u32 rotr(const u32 value, const u32 shift) {
// on arm, ror is an actual instruction, however i am not sure how to force
// gcc to generate a ror without using instrinsics.
// i don't imagine the arm_acle is avaliable on all platforms either.
// if compiling for arm AND the header isn't avaliable, then i should write inline asm
// for the instruction.
#ifdef __ARM_ACLE
    return __ror(value, shift & 31U);
#else
    return (value >> (shift & 31U)) | static_cast<u32>(((static_cast<u64>(value) << 32U) >> (shift & 31U)));
#endif
}

static_assert(
    rotr(0b1111'1111'1111'1111'1111'1111'1111'1111, 2) == 0b1111'1111'1111'1111'1111'1111'1111'1111 &&
    rotr(0b0011'1111'1111'1111'1111'1111'1111'1111, 2) == 0b1100'1111'1111'1111'1111'1111'1111'1111 &&
    // all 1's
    rotr(0b1111'1111'1111'1111'1111'1111'1111'1111, 69) == 0b1111'1111'1111'1111'1111'1111'1111'1111 &&
    // same as rotr(x, 2)
    rotr(0b0011'1111'1111'1111'1111'1111'1111'1111, 34) == 0b1100'1111'1111'1111'1111'1111'1111'1111,
    "bit::rotr is broken!"
);

// bit value should probably be either masked or, in debug mode,
// check if bit is ever >= 32, if so, throw.
template <IntV T> [[nodiscard]]
constexpr bool is_set(const T value, const u32 bit) {
    assert(bit < (sizeof(T) * 8) && "bit value out of bounds!");
    return (value & (1ULL << bit)) > 0;
}

// template <IntV T> [[nodiscard]]
// constexpr T set(const T value, const u32 bit, const bool on) {
//     assert(bit < (sizeof(T) * 8) && "bit value out of bounds!");
//     return value | (on << bit);
// }

template <IntV T> [[nodiscard]]
constexpr T unset(const T value, const u32 bit) {
    assert(bit < (sizeof(T) * 8) && "bit value out of bounds!");
    return value & (~(1ULL << bit));
}

// compile-time bit-size checked checked alternitives
template <u8 bit, IntV T> [[nodiscard]]
constexpr bool is_set(const T value) {
    static_assert(bit < (sizeof(T) * 8), "bit value out of bounds!");
    return (value & (1ULL << bit)) > 0;
}

template <u8 bit, IntV T> [[nodiscard]]
constexpr T set(const T value, const bool on) {
    constexpr auto bit_width = sizeof(T) * 8;

    static_assert(bit < bit_width, "bit value out of bounds!");

    // create a mask with all bits set apart from THE `bit`
    // this allows toggling the bit.
    // if on = true, the bit is set, else nothing is set
    constexpr auto mask = get_mask<T>() & (~(1ULL << bit));

    return (value & mask) | (on << bit);
}

template <u8 bit, IntV T> [[nodiscard]]
constexpr T unset(const T value) {
    constexpr auto bit_width = sizeof(T) * 8;
    static_assert(bit < bit_width, "bit value out of bounds!");

    constexpr auto mask = ~(1ULL << bit);

    return value & mask;
}

static_assert(
    bit::set<0, u8>(0b1100, true) == 0b1101 &&
    bit::set<3, u8>(0b1101, false) == 0b0101,
    "bit::set is broken!"
);

// CREDIT: thanks to peach teaching me asr carries down the sign bit!
// as of c++20, a >> b, where a is negative-signed, this will perform asr shift.
// prior to c++20 this was implementation defiened, so it could be lsr
// however gcc and clang both did asr anyway.
template <u8 start_size> [[nodiscard]]
constexpr u32 sign_extend(const u32 value) {
    static_assert(start_size <= 31, "bit start size is out of bounds!");

    const u8 bits = 32 - start_size;
    return static_cast<u32>(static_cast<s32>(value << bits) >> bits);
}

static_assert(
    // simple 24-bit asr
    bit::sign_extend<24>(0b1100'1111'1111'1111'1111'1111) == 0b1111'1111'1100'1111'1111'1111'1111'1111 &&
    // set the sign-bit to bit 1, then asr 31-bits
    bit::sign_extend<1>(0b0001) == 0b1111'1111'1111'1111'1111'1111'1111'1111 &&
    // this is used in thumb ldr halword sign
    bit::sign_extend<16>(0b0000'0000'1110'0000'1111'1111'1111'1111) == 0b1111'1111'1111'1111'1111'1111'1111'1111 &&
    // same as above but no sign
    bit::sign_extend<16>(0b0000'0000'1110'0000'0111'1111'1111'1111) == 0b0000'0000'0000'0000'0111'1111'1111'1111,
    "sign_extend is broken!"
);

template <u8 start, u8 end, IntV T> [[nodiscard]]
consteval auto get_mask_range() -> T {
    static_assert(start < end);
    static_assert(end < (sizeof(T) * 8));

    T result = 0;
    for (auto bit = start; bit <= end; ++bit) {
        result |= (1 << bit);
    }

    return result;
}

static_assert(
    bit::get_mask_range<3, 5, u32>() == 0b111'000 &&
    bit::get_mask_range<0, 2, u32>() == 0b000'111 &&
    bit::get_mask_range<1, 5, u32>() == 0b111'110 &&
    bit::get_mask_range<4, 5, u32>() == 0b110'000,
    "bit::get_mask_range is broken!"
);

template <u8 start, u8 end, IntV T> [[nodiscard]]
constexpr auto get_range(const T value) {
    static_assert(start < end, "range is invalid!");
    static_assert(end < (sizeof(T) * 8));

    constexpr auto mask = get_mask_range<start, end, T>() >> start;

    return (value >> start) & mask;
}

static_assert(
    bit::get_range<3, 5, u32>(0b111'000) == 0b000'111 &&
    bit::get_range<0, 2, u32>(0b000'010) == 0b000'010 &&
    bit::get_range<1, 5, u32>(0b111'110) == 0b011'111 &&
    bit::get_range<4, 5, u32>(0b110'000) == 0b000'011,
    "bit::get_range is broken!"
);

template <u8 start, u8 end, IntV Int, IntV Slice> [[nodiscard]]
constexpr auto set_range(const Int value, const Slice slice) {
    static_assert(start < end, "range is invalid!");
    static_assert(end < (sizeof(Int) * 8));

    // invert them as we want to set the range
    constexpr auto mask = get_mask_range<start, end, Int>();
    constexpr auto inverted_mask = ~mask;

    return (value & inverted_mask) | ((slice << start) & mask);
}

} // namespace bit

