#include "core/bit.hpp"

#include <cstdint>
#include <cassert>
#include <array>

namespace dmg {

using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;

enum class Flag {
    C, H, N, Z
};

struct System {
    u16 reg_pc;
    u16 reg_sp;
    u8 reg_b;
    u8 reg_c;
    u8 reg_d;
    u8 reg_e;
    u8 reg_h;
    u8 reg_l;
    u8 reg_a;

    bool flag_c;
    bool flag_h;
    bool flag_n;
    bool flag_z;

    template <Flag flag> [[nodiscard]]
    constexpr auto get_flag() const noexcept {
        if constexpr(flag == Flag::C) { return this->flag_c; }
        if constexpr(flag == Flag::H) { return this->flag_h; }
        if constexpr(flag == Flag::N) { return this->flag_n; }
        if constexpr(flag == Flag::Z) { return this->flag_z; }
    }

    template <Flag flag>
    constexpr auto set_flag(const bool v) noexcept -> void {
        if constexpr(flag == Flag::C) { this->flag_c = v; }
        if constexpr(flag == Flag::H) { this->flag_h = v; }
        if constexpr(flag == Flag::N) { this->flag_n = v; }
        if constexpr(flag == Flag::Z) { this->flag_z = v; }
    }

    [[nodiscard]]
    constexpr auto get_reg_f() const noexcept -> u8 {
        return (this->get_flag<Flag::C>() << 7) | (this->get_flag<Flag::H>() << 6)
            | (this->get_flag<Flag::N>() << 5) | (this->get_flag<Flag::Z>() << 4);
    }

    constexpr auto set_reg_f(const u8 v) noexcept {
        this->set_flag<Flag::C>(bit::is_set<7>(v));
        this->set_flag<Flag::H>(bit::is_set<6>(v));
        this->set_flag<Flag::N>(bit::is_set<5>(v));
        this->set_flag<Flag::Z>(bit::is_set<4>(v));
    }
};

enum class Reg8Type {
    B, C, D, E, H, L, HL, A
};

enum class Reg16Type {
    BC, DE, HL, SP
};

template <Reg8Type type>
constexpr auto get_reg(System& sys) noexcept {
    if constexpr(type == Reg8Type::B) { return sys.reg_b; }
    if constexpr(type == Reg8Type::C) { return sys.reg_c; }
    if constexpr(type == Reg8Type::D) { return sys.reg_d; }
    if constexpr(type == Reg8Type::E) { return sys.reg_e; }
    if constexpr(type == Reg8Type::H) { return sys.reg_h; }
    if constexpr(type == Reg8Type::L) { return sys.reg_l; }
    if constexpr(type == Reg8Type::A) { return sys.reg_a; }
    if constexpr(type == Reg8Type::HL) { // todo:
        return sys.reg_b;
    }
}

template <Reg8Type type>
constexpr auto set_reg(System& sys, const u8 v) noexcept -> void {
    if constexpr(type == Reg8Type::B) { sys.reg_b = v; }
    if constexpr(type == Reg8Type::C) { sys.reg_c = v; }
    if constexpr(type == Reg8Type::D) { sys.reg_d = v; }
    if constexpr(type == Reg8Type::E) { sys.reg_e = v; }
    if constexpr(type == Reg8Type::H) { sys.reg_h = v; }
    if constexpr(type == Reg8Type::L) { sys.reg_l = v; }
    if constexpr(type == Reg8Type::A) { sys.reg_a = v; }
    if constexpr(type == Reg8Type::HL) { // todo:
        sys.reg_b = v;
    }
}

template <Reg16Type type>
constexpr auto get_reg(System& sys) noexcept {
    if constexpr(type == Reg16Type::BC) { return sys.reg_b; }
    if constexpr(type == Reg16Type::DE) { return sys.reg_c; }
    if constexpr(type == Reg16Type::HL) { return sys.reg_d; }
    if constexpr(type == Reg16Type::SP) { return sys.reg_e; }
}

template <Reg16Type type>
constexpr auto set_reg(System& sys, const u16 v) noexcept -> void {
    if constexpr(type == Reg16Type::BC) { sys.reg_b = v; }
    if constexpr(type == Reg16Type::DE) { sys.reg_c = v; }
    if constexpr(type == Reg16Type::HL) { sys.reg_d = v; }
    if constexpr(type == Reg16Type::SP) { sys.reg_e = v; }
}

enum class CondType {
    NZ, Z, NC, C, NONE /* NONE = always jump */
};

template <CondType type> [[nodiscard]] // TODO:
constexpr auto helper_cond([[maybe_unused]] System& sys) noexcept {
    if constexpr (type == CondType::NZ) { return false; }
    if constexpr (type == CondType::Z) { return false; }
    if constexpr (type == CondType::NC) { return false; }
    if constexpr (type == CondType::C) { return false; }
    if constexpr (type == CondType::NONE) { return false; }
}

template <Reg8Type type>
constexpr auto inst_rlc([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_rrc([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_rl([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_rr([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_sla([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_sra([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_swap([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_srl([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type, u8 Bit>
constexpr auto inst_bit([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type, u8 Bit>
constexpr auto inst_res([[maybe_unused]] System& sys) noexcept -> void {
    const auto reg = get_reg<type>(sys);
    const auto result = bit::unset<Bit>(reg);
    set_reg<type>(sys, result);
}

template <Reg8Type type, u8 Bit>
constexpr auto inst_set([[maybe_unused]] System& sys) noexcept -> void {
    const auto reg = get_reg<type>(sys);
    const auto result = bit::set<Bit>(reg, true);
    set_reg<type>(sys, result);
}

template <Reg8Type src, Reg8Type dst>
constexpr auto inst_ld([[maybe_unused]] System& sys) noexcept -> void {
    set_reg<dst>(sys, get_reg<src>(sys));
}

template <Reg16Type src, Reg8Type dst>
constexpr auto inst_ld([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type dst>
constexpr auto inst_ld_u8([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_add([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_adc([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_sub([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_sbc([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_and([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_xor([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_or([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_cp([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_inc([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg16Type type>
constexpr auto inst_inc([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type>
constexpr auto inst_dec([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg16Type type>
constexpr auto inst_dec([[maybe_unused]] System& sys) noexcept -> void { }

constexpr auto inst_nop([[maybe_unused]] System& sys) noexcept -> void { }

constexpr auto inst_stop([[maybe_unused]] System& sys) noexcept -> void { }

constexpr auto inst_halt([[maybe_unused]] System& sys) noexcept -> void { }

constexpr auto inst_cb([[maybe_unused]] System& sys) noexcept -> void { }

constexpr auto inst_di([[maybe_unused]] System& sys) noexcept -> void { }

constexpr auto inst_ei([[maybe_unused]] System& sys) noexcept -> void { }

auto inst_unk([[maybe_unused]] System& sys) -> void { assert(0); }

#define SET_REG(func, idx) table[idx] = func<static_cast<Reg8Type>(idx & 7)>;
#define SET_LD(func, idx) table[idx] = func<static_cast<Reg8Type>(idx & 7), static_cast<Reg8Type>(((idx >> 3) & 7))>;
#define SET_BIT(func, idx) table[idx] = func<static_cast<Reg8Type>(idx & 7), (idx >> 3) & 0x7>;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-compare"

template <std::size_t Start = 0x00>
consteval auto setup_top_row(auto& table) {
    if constexpr (Start == 0x100) { return; }

    // todo: give better names
    #define SET_REG2(func, idx) table[idx] = func<static_cast<Reg8Type>((idx >> 3) & 7)>;
    #define SET_REG3(func, idx) table[idx] = func<static_cast<Reg16Type>((idx >> 4) & 3)>;

    else {
        if constexpr (Start < 0x40) { // top
            // if constexpr ((Start & 0xF) == 0x01) { SET_REG(inst_add, Start);    }
            // if constexpr ((Start & 0xF) == 0x02) { SET_REG(inst_adc, Start);    }
            if constexpr ((Start & 0xF) == 0x03) { SET_REG3(inst_inc, Start);    }
            if constexpr ((Start & 0xF) == 0x04) { SET_REG2(inst_inc, Start);    }
            if constexpr ((Start & 0xF) == 0x05) { SET_REG2(inst_dec, Start);    }
            if constexpr ((Start & 0xF) == 0x06) { SET_REG2(inst_ld_u8, Start);    }
            //
            // if constexpr ((Start & 0xF) == 0x09) { SET_REG(inst_add, Start);    }
            // if constexpr ((Start & 0xF) == 0x0A) { SET_REG(inst_adc, Start);    }
            if constexpr ((Start & 0xF) == 0x0B) { SET_REG3(inst_dec, Start);    }
            if constexpr ((Start & 0xF) == 0x0C) { SET_REG2(inst_inc, Start);    }
            if constexpr ((Start & 0xF) == 0x0D) { SET_REG2(inst_dec, Start);    }
            if constexpr ((Start & 0xF) == 0x0E) { SET_REG2(inst_ld_u8, Start);    }
        }
        
        if constexpr (Start >= 0x40 && Start <= 0x7F) { // mid ld
            SET_LD(inst_ld, Start);
        }

        if constexpr (Start >= 0x80 && Start <= 0xBF) { // mid alu
            if constexpr (Start >= 0x80 && Start <= 0x87) { SET_REG(inst_add, Start);    }
            if constexpr (Start >= 0x88 && Start <= 0x8F) { SET_REG(inst_adc, Start);    }
            if constexpr (Start >= 0x90 && Start <= 0x97) { SET_REG(inst_sub, Start);    }
            if constexpr (Start >= 0x98 && Start <= 0x9F) { SET_REG(inst_sbc, Start);    }
            if constexpr (Start >= 0xA0 && Start <= 0xA7) { SET_REG(inst_and, Start);    }
            if constexpr (Start >= 0xA8 && Start <= 0xAF) { SET_REG(inst_xor, Start);    }
            if constexpr (Start >= 0xB0 && Start <= 0xB7) { SET_REG(inst_or, Start);     }
            if constexpr (Start >= 0xB8 && Start <= 0xBF) { SET_REG(inst_cp, Start);     }
        }

        if constexpr (Start >= 0xC0 && Start <= 0xFF) { // bottom

        }

        setup_top_row<Start + 1>(table);
    }
}

#pragma GCC diagnostic pop

template <std::size_t Start = 0>
consteval auto setup_cb(auto& table) {
    if constexpr (Start == 0x100) { return; }

    else {
        if constexpr (Start >= 0x00 && Start <= 0x07) { SET_REG(inst_rlc, Start);    }
        if constexpr (Start >= 0x08 && Start <= 0x0F) { SET_REG(inst_rrc, Start);    }
        if constexpr (Start >= 0x10 && Start <= 0x17) { SET_REG(inst_rl, Start);     }
        if constexpr (Start >= 0x18 && Start <= 0x1F) { SET_REG(inst_rr, Start);     }
        if constexpr (Start >= 0x20 && Start <= 0x27) { SET_REG(inst_sla, Start);    }
        if constexpr (Start >= 0x28 && Start <= 0x2F) { SET_REG(inst_sra, Start);    }
        if constexpr (Start >= 0x30 && Start <= 0x37) { SET_REG(inst_swap, Start);   }
        if constexpr (Start >= 0x38 && Start <= 0x3F) { SET_REG(inst_srl, Start);    }
        if constexpr (Start >= 0x40 && Start <= 0x7F) { SET_BIT(inst_bit, Start);    }
        if constexpr (Start >= 0x80 && Start <= 0xBF) { SET_BIT(inst_res, Start);    }
        if constexpr (Start >= 0xC0 && Start <= 0xFF) { SET_BIT(inst_set, Start);    }
        
        setup_cb<Start + 1>(table);
    }
}

consteval auto gen_table() {
    std::array<std::enable_if<true, auto (*)(dmg::System &sys)->void>::type, 0x100> table{};
    table.fill(inst_unk);
    setup_top_row(table);
    return table;
}

consteval auto gen_table_cb() {
    std::array<std::enable_if<true, auto (*)(dmg::System &sys)->void>::type, 0x100> table{};
    setup_cb(table);
    return table;
}

#undef SET_REG
#undef SET_BIT
#undef SET_LD

constexpr std::array INSTRUCTION_TABLE = gen_table();
constexpr std::array INSTRUCTION_TABLE_CB = gen_table_cb();

static_assert(INSTRUCTION_TABLE.size() == 0x100);
static_assert(INSTRUCTION_TABLE_CB.size() == 0x100);

// quick test that the table is valid
constexpr void test() {
    constexpr auto func = []() {
        dmg::System system{};
        for (auto function : dmg::INSTRUCTION_TABLE_CB) {
            function(system);
        }
    
        return system.reg_pc == 0;
    };

    static_assert(func());
}

} // dmg

auto main() -> int {

    return 0;
}