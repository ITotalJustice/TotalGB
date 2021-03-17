#pragma once

#include "common.hpp"
#include "dmg.hpp"

namespace dmg::inst {

enum class Reg8Type {
    B, C, D, E, H, L, HL, A
};

enum class Reg16TypeA {
    BC, DE, HL, SP
};

enum class Reg16TypeB {
    BC, DE, HL, AF
};

enum class Reg16TypeC { // used for LD (R16)
    BC, DE, HLp, HLn
};

template <Reg8Type type>
constexpr auto get_reg(System& sys) noexcept {
    if constexpr(type == Reg8Type::B) { return sys.get_reg<Reg8::B>(); }
    if constexpr(type == Reg8Type::C) { return sys.get_reg<Reg8::C>(); }
    if constexpr(type == Reg8Type::D) { return sys.get_reg<Reg8::D>(); }
    if constexpr(type == Reg8Type::E) { return sys.get_reg<Reg8::E>(); }
    if constexpr(type == Reg8Type::H) { return sys.get_reg<Reg8::H>(); }
    if constexpr(type == Reg8Type::L) { return sys.get_reg<Reg8::L>(); }
    if constexpr(type == Reg8Type::A) { return sys.get_reg<Reg8::A>(); }
    if constexpr(type == Reg8Type::HL) { return sys.mmio->read(sys.get_reg<Reg16::HL>()); }
}

template <Reg8Type type>
constexpr auto set_reg(System& sys, const u8 v) noexcept -> void {
    if constexpr(type == Reg8Type::B) { sys.set_reg<Reg8::B>(v); }
    if constexpr(type == Reg8Type::C) { sys.set_reg<Reg8::C>(v); }
    if constexpr(type == Reg8Type::D) { sys.set_reg<Reg8::D>(v); }
    if constexpr(type == Reg8Type::E) { sys.set_reg<Reg8::E>(v); }
    if constexpr(type == Reg8Type::H) { sys.set_reg<Reg8::H>(v); }
    if constexpr(type == Reg8Type::L) { sys.set_reg<Reg8::L>(v); }
    if constexpr(type == Reg8Type::A) { sys.set_reg<Reg8::A>(v); }
    if constexpr(type == Reg8Type::HL) { sys.mmio->write(sys.get_reg<Reg16::HL>(), v); }
}

template <Reg16TypeA type>
constexpr auto get_reg(System& sys) noexcept {
    if constexpr(type == Reg16TypeA::BC) { return sys.get_reg<Reg16::BC>(); }
    if constexpr(type == Reg16TypeA::DE) { return sys.get_reg<Reg16::DE>(); }
    if constexpr(type == Reg16TypeA::HL) { return sys.get_reg<Reg16::HL>(); }
    if constexpr(type == Reg16TypeA::SP) { return sys.get_reg<Reg16::SP>(); }
}

template <Reg16TypeA type>
constexpr auto set_reg(System& sys, const u16 v) noexcept -> void {
    if constexpr(type == Reg16TypeA::BC) { sys.set_reg<Reg16::BC>(v); }
    if constexpr(type == Reg16TypeA::DE) { sys.set_reg<Reg16::DE>(v); }
    if constexpr(type == Reg16TypeA::HL) { sys.set_reg<Reg16::HL>(v); }
    if constexpr(type == Reg16TypeA::SP) { sys.set_reg<Reg16::SP>(v); }
}

template <Reg16TypeB type>
constexpr auto get_reg(System& sys) noexcept {
    if constexpr(type == Reg16TypeB::BC) { return sys.get_reg<Reg16::BC>(); }
    if constexpr(type == Reg16TypeB::DE) { return sys.get_reg<Reg16::DE>(); }
    if constexpr(type == Reg16TypeB::HL) { return sys.get_reg<Reg16::HL>(); }
    if constexpr(type == Reg16TypeB::AF) { return sys.get_reg<Reg16::AF>(); }
}

template <Reg16TypeB type>
constexpr auto set_reg(System& sys, const u16 v) noexcept -> void {
    if constexpr(type == Reg16TypeB::BC) { sys.set_reg<Reg16::BC>(v); }
    if constexpr(type == Reg16TypeB::DE) { sys.set_reg<Reg16::DE>(v); }
    if constexpr(type == Reg16TypeB::HL) { sys.set_reg<Reg16::HL>(v); }
    if constexpr(type == Reg16TypeB::AF) { sys.set_reg<Reg16::AF>(v); }
}

template <Reg16TypeC type>
constexpr auto get_reg(System& sys) noexcept {
    if constexpr(type == Reg16TypeC::BC) {
        const auto value = sys.get_reg<Reg16::BC>();
        const auto result = sys.mmio->read16(value);
        return result;
    }
    if constexpr(type == Reg16TypeC::DE) {
        const auto value = sys.get_reg<Reg16::DE>();
        const auto result = sys.mmio->read16(value);
        return result;
    }
    if constexpr(type == Reg16TypeC::HLp) {
        const auto value = sys.get_reg<Reg16::HL>();
        const auto result = sys.mmio->read16(value);
        sys.set_reg<Reg16::HL>(value + 1);
        return result;
    }
    if constexpr(type == Reg16TypeC::HLn) {
        const auto value = sys.get_reg<Reg16::HL>();
        const auto result = sys.mmio->read16(value);
        sys.set_reg<Reg16::HL>(value - 1);
        return result;
    }
}

template <Reg16TypeC type>
constexpr auto set_reg(System& sys, const u16 v) noexcept -> void {
    if constexpr(type == Reg16TypeC::BC) {
        const auto value = sys.get_reg<Reg16::BC>();
        sys.mmio->write16(value, v);
    }
    if constexpr(type == Reg16TypeC::DE) {
        const auto value = sys.get_reg<Reg16::DE>();
        sys.mmio->write16(value, v);
    }
    if constexpr(type == Reg16TypeC::HLp) {
        const auto value = sys.get_reg<Reg16::HL>();
        sys.mmio->write16(value, v);
        sys.set_reg<Reg16::HL>(value + 1);
    }
    if constexpr(type == Reg16TypeC::HLn) {
        const auto value = sys.get_reg<Reg16::SP>();
        sys.mmio->write16(value, v);
        sys.set_reg<Reg16::HL>(value - 1);
    }
}

enum class CondType {
    NZ, Z, NC, C
};

template <CondType type> [[nodiscard]] // TODO:
constexpr auto helper_cond([[maybe_unused]] System& sys) noexcept {
    if constexpr (type == CondType::NZ) { return false; }
    if constexpr (type == CondType::Z) { return false; }
    if constexpr (type == CondType::NC) { return false; }
    if constexpr (type == CondType::C) { return false; }
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
constexpr auto inst_swap([[maybe_unused]] System& sys) noexcept -> void {
    const auto value = get_reg<type>(sys);
    const auto result = (value << 4) | (value >> 4);
    set_reg<type>(sys, result);
    sys.set_flags(
        Flag::C, false, 
        Flag::H, false, 
        Flag::N, false,
        Flag::Z, result == 0
    );
}

template <Reg8Type type>
constexpr auto inst_srl([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type type, u8 Bit>
constexpr auto inst_bit([[maybe_unused]] System& sys) noexcept -> void {
    const auto value = get_reg<type>(sys);
    const auto result = bit::is_set<Bit>(value);
    sys.set_flags(
        Flag::H, true,
        Flag::N, false,
        Flag::Z, result == 0
    );
}

template <Reg8Type type, u8 Bit>
constexpr auto inst_res([[maybe_unused]] System& sys) noexcept -> void {
    const auto value = get_reg<type>(sys);
    const auto result = bit::unset<Bit>(value);
    set_reg<type>(sys, result);
}

template <Reg8Type type, u8 Bit>
constexpr auto inst_set([[maybe_unused]] System& sys) noexcept -> void {
    const auto value = get_reg<type>(sys);
    const auto result = bit::set<Bit>(value, true);
    set_reg<type>(sys, result);
}

template <Reg8Type src, Reg8Type dst>
constexpr auto inst_ld([[maybe_unused]] System& sys) noexcept -> void {
    const auto result = get_reg<src>(sys);
    set_reg<dst>(sys, result);
}

template <Reg16TypeA dst>
constexpr auto inst_ld([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg16TypeC src, Reg8Type dst>
constexpr auto inst_ld([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type dst, Reg16TypeC src>
constexpr auto inst_ld([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type dst>
constexpr auto inst_ld([[maybe_unused]] System& sys) noexcept -> void {
    const auto result = sys.mmio->read(sys.reg_pc++);
    set_reg<dst>(sys, result);
}

template <Reg8Type type>
constexpr auto inst_add(System& sys, const bool carry) noexcept -> void {
    const auto value = get_reg<type>(sys) + carry;
    const auto result = sys.reg_a + value;
    sys.set_flags(
        Flag::C, sys.reg_a + value > 0xFF, 
        Flag::H, (sys.reg_a & 0xF) + (value & 0xF) > 0xF, 
        Flag::N, false,
        Flag::Z, result == 0
    );
    sys.reg_a = result;
}

template <Reg8Type type>
constexpr auto inst_add(System& sys) noexcept -> void {
    inst_add<type>(sys, false);
}

template <Reg8Type type>
constexpr auto inst_adc(System& sys) noexcept -> void {
    inst_add<type>(sys, sys.get_flag<Flag::C>());
}

template <Reg8Type type>
constexpr auto inst_sub(System& sys, const bool carry) noexcept -> void {
    const auto value = get_reg<type>(sys) + carry;
    const auto result = sys.reg_a - value;
    sys.set_flags(
        Flag::C, value < sys.reg_a, 
        Flag::H, (sys.reg_a & 0xF) < (value & 0xF), 
        Flag::N, true,
        Flag::Z, result == 0
    );
    sys.reg_a = result;
}

template <Reg8Type type>
constexpr auto inst_sub(System& sys) noexcept -> void {
    inst_sub<type>(sys, false);
}

template <Reg8Type type>
constexpr auto inst_sbc(System& sys) noexcept -> void {
    inst_sub<type>(sys, sys.get_flag<Flag::C>());
}

template <Reg8Type type>
constexpr auto inst_and(System& sys) noexcept -> void {
    const auto value = get_reg<type>(sys);
    const auto result = sys.reg_a & value;
    sys.set_flags(
        Flag::C, false, 
        Flag::H, false, 
        Flag::N, false,
        Flag::Z, result == 0
    );
    sys.reg_a = result;
}

template <Reg8Type type>
constexpr auto inst_xor(System& sys) noexcept -> void {
    const auto value = get_reg<type>(sys);
    const auto result = sys.reg_a ^ value;
    sys.set_flags(
        Flag::C, false, 
        Flag::H, false, 
        Flag::N, false,
        Flag::Z, result == 0
    );
    sys.reg_a = result;
}

template <Reg8Type type>
constexpr auto inst_or(System& sys) noexcept -> void {
    const auto value = get_reg<type>(sys);
    const auto result = sys.reg_a | value;
    sys.set_flags(
        Flag::C, false, 
        Flag::H, false, 
        Flag::N, false,
        Flag::Z, result == 0
    );
    sys.reg_a = result;
}

template <Reg8Type type>
constexpr auto inst_cp(System& sys) noexcept -> void {
    const auto value = get_reg<type>(sys);
    const auto result = sys.reg_a - value;
    sys.set_flags(
        Flag::C, value < sys.reg_a, 
        Flag::H, (sys.reg_a & 0xF) < (value & 0xF), 
        Flag::N, true,
        Flag::Z, result == 0
    );
}

template <Reg8Type type>
constexpr auto inst_inc(System& sys) noexcept -> void { 
    const auto value = get_reg<type>(sys);
    const auto result = value + 1;
    sys.set_flags(
        Flag::H, (result & 0xF) == 0,
        Flag::N, false,
        Flag::Z, result == 0
    );
    set_reg<type>(sys, result);
}

template <Reg16TypeA type>
constexpr auto inst_inc(System& sys) noexcept -> void { 
    const auto value = get_reg<type>(sys);
    const auto result = value + 1;
    set_reg<type>(sys, result);
}

template <Reg8Type type>
constexpr auto inst_dec(System& sys) noexcept -> void {
    const auto value = get_reg<type>(sys);
    const auto result = value + 1;
    sys.set_flags(
        Flag::H, (result & 0xF) == 0xF,
        Flag::N, true,
        Flag::Z, result == 0
    );
    set_reg<type>(sys, result);
}

template <Reg16TypeA type>
constexpr auto inst_dec(System& sys) noexcept -> void { 
    const auto value = get_reg<type>(sys);
    const auto result = value - 1;
    set_reg<type>(sys, result);
}

[[nodiscard]]
constexpr auto inst_pop(System& sys) noexcept -> u16 {
    const auto result = sys.mmio->read16(sys.reg_sp);
    sys.reg_sp += 2;
    return result;
}

constexpr auto inst_push(System& sys, const u16 v) noexcept -> void {
    sys.mmio->write(--sys.reg_sp, v >> 8);
    sys.mmio->write(--sys.reg_sp, v & 0xFF);
}

template <Reg16TypeB type>
constexpr auto inst_pop(System& sys) noexcept -> void {
    const auto result = inst_pop(sys);
    set_reg<type>(sys, result);
}

template <Reg16TypeB type>
constexpr auto inst_push(System& sys) noexcept -> void {
    const auto result = get_reg<type>(sys);
    inst_push(sys, result);
}

constexpr auto inst_rst(System& sys, const u8 v) noexcept -> void {
    inst_push(sys, sys.reg_pc);
    sys.reg_pc = v;
}

template <u8 Value>
constexpr auto inst_rst(System& sys) noexcept -> void {
    inst_rst(sys, Value);
}

constexpr auto inst_jr(System& sys) noexcept -> void {
    const auto value = static_cast<s8>(sys.mmio->read(sys.reg_pc) + 1);
    sys.reg_pc += value;
}

constexpr auto inst_ret(System& sys) noexcept -> void {
    sys.reg_pc = inst_pop(sys);
}

constexpr auto inst_jp(System& sys) noexcept -> void {
    sys.reg_pc = sys.mmio->read16(sys.reg_pc);
}

constexpr auto inst_call(System& sys) noexcept -> void {
    const auto result = sys.mmio->read16(sys.reg_pc);
    inst_push(sys, sys.reg_pc + 2);
    sys.reg_pc = result;
}

template <CondType type>
constexpr auto inst_jr(System& sys) noexcept -> void {
    if (helper_cond<type>(sys)) {
        inst_jr(sys);
    } else {
        ++sys.reg_pc;
    }
}

template <CondType type>
constexpr auto inst_ret(System& sys) noexcept -> void {
     if (helper_cond<type>(sys)) {
        inst_ret(sys);
    }
}

template <CondType type>
constexpr auto inst_jp(System& sys) noexcept -> void {
     if (helper_cond<type>(sys)) {
        inst_jp(sys);
    } else {
        sys.reg_pc += 2;
    }
}

template <CondType type>
constexpr auto inst_call(System& sys) noexcept -> void {
     if (helper_cond<type>(sys)) {
        inst_call(sys);
    } else {
        sys.reg_pc += 2;
    }
}

constexpr auto inst_nop([[maybe_unused]] System& sys) noexcept -> void { }

constexpr auto inst_stop([[maybe_unused]] System& sys) noexcept -> void {
    // should never be called by a game...
}

constexpr auto inst_halt([[maybe_unused]] System& sys) noexcept -> void { 
    // todo: finish this...
    sys.halt = true;
}

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
    if constexpr(Start == 0x100) { return; }

    // todo: give better names
    #define SET_REG2(func, idx) table[idx] = func<static_cast<Reg8Type>((idx >> 3) & 7)>;
    // used for inc / dec, LD (u16)
    #define SET_REG3(func, idx) table[idx] = func<static_cast<Reg16TypeA>((idx >> 4) & 3)>;
    // used for pop / push
    #define SET_REG4(func, idx) table[idx] = func<static_cast<Reg16TypeB>((idx >> 4) & 3)>;
    // used for RST
    #define SET_REG5(func, idx) table[idx] = func<idx & 0x38>;
    // used for COND jumps
    #define SET_REG6(func, idx) table[idx] = func<static_cast<CondType>((idx >> 3) & 3)>;
    // used ld (r16), A
    #define SET_REG7(func, idx) table[idx] = func<static_cast<Reg16TypeC>((idx >> 4) & 3), Reg8Type::A>;
    // used ld A,(r16)
    #define SET_REG8(func, idx) table[idx] = func<Reg8Type::A, static_cast<Reg16TypeC>((idx >> 4) & 3)>;

    else {
        if constexpr(Start < 0x40) { // top
            if constexpr ((Start & 0xF) == 0x01) { SET_REG3(inst_ld, Start);    }
            if constexpr ((Start & 0xF) == 0x02) { SET_REG7(inst_ld, Start);    }
            if constexpr ((Start & 0xF) == 0x0A) { SET_REG8(inst_ld, Start);    }

            if constexpr((Start & 0xF) == 0x03) { SET_REG3(inst_inc, Start);    }
            if constexpr((Start & 0xF) == 0x04) { SET_REG2(inst_inc, Start);    }
            if constexpr((Start & 0xF) == 0x05) { SET_REG2(inst_dec, Start);    }
            if constexpr((Start & 0xF) == 0x06) { SET_REG2(inst_ld, Start);    }
            //
            // if constexpr((Start & 0xF) == 0x09) { SET_REG(inst_add, Start);    }
            // if constexpr((Start & 0xF) == 0x0A) { SET_REG(inst_adc, Start);    }
            if constexpr((Start & 0xF) == 0x0B) { SET_REG3(inst_dec, Start);    }
            if constexpr((Start & 0xF) == 0x0C) { SET_REG2(inst_inc, Start);    }
            if constexpr((Start & 0xF) == 0x0D) { SET_REG2(inst_dec, Start);    }
            if constexpr((Start & 0xF) == 0x0E) { SET_REG2(inst_ld, Start);    }
        }
        
        if constexpr(Start >= 0x40 && Start <= 0x7F) { // mid ld
            SET_LD(inst_ld, Start);
        }

        if constexpr(Start >= 0x80 && Start <= 0xBF) { // mid alu
            if constexpr(Start >= 0x80 && Start <= 0x87) { SET_REG(inst_add, Start);    }
            if constexpr(Start >= 0x88 && Start <= 0x8F) { SET_REG(inst_adc, Start);    }
            if constexpr(Start >= 0x90 && Start <= 0x97) { SET_REG(inst_sub, Start);    }
            if constexpr(Start >= 0x98 && Start <= 0x9F) { SET_REG(inst_sbc, Start);    }
            if constexpr(Start >= 0xA0 && Start <= 0xA7) { SET_REG(inst_and, Start);    }
            if constexpr(Start >= 0xA8 && Start <= 0xAF) { SET_REG(inst_xor, Start);    }
            if constexpr(Start >= 0xB0 && Start <= 0xB7) { SET_REG(inst_or, Start);     }
            if constexpr(Start >= 0xB8 && Start <= 0xBF) { SET_REG(inst_cp, Start);     }
        }

        if constexpr(Start >= 0xC0 && Start <= 0xFF) { // bottom
            if constexpr(Start < 0xE0 && (Start & 0xF) == 0x00) { SET_REG6(inst_ret, Start); }
            if constexpr(Start < 0xE0 && (Start & 0xF) == 0x08) { SET_REG6(inst_ret, Start); }
            if constexpr(Start < 0xE0 && (Start & 0xF) == 0x02) { SET_REG6(inst_jp, Start); }
            if constexpr(Start < 0xE0 && (Start & 0xF) == 0x0A) { SET_REG6(inst_jp, Start); }
            if constexpr(Start < 0xE0 && (Start & 0xF) == 0x04) { SET_REG6(inst_call, Start); }
            if constexpr(Start < 0xE0 && (Start & 0xF) == 0x0C) { SET_REG6(inst_call, Start); }

            if constexpr((Start & 0xF) == 0x01) { SET_REG4(inst_pop, Start); }
            if constexpr((Start & 0xF) == 0x05) { SET_REG4(inst_push, Start); }
            if constexpr((Start & 0xF) == 0x07) { SET_REG5(inst_rst, Start); }
            if constexpr((Start & 0xF) == 0x0F) { SET_REG5(inst_rst, Start); }
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
    table.fill(inst_unk);
    setup_cb(table);
    return table;
}

#undef SET_REG
#undef SET_BIT
#undef SET_LD
#undef SET_REG2
#undef SET_REG3
#undef SET_REG4
#undef SET_REG5
#undef SET_REG6

constexpr std::array INSTRUCTION_TABLE = gen_table();
constexpr std::array INSTRUCTION_TABLE_CB = gen_table_cb();

static_assert(INSTRUCTION_TABLE.size() == 0x100);
static_assert(INSTRUCTION_TABLE_CB.size() == 0x100);

// quick test that the table is valid
constexpr void test() {
    constexpr auto func = []() {
        dmg::System system{};
        auto mmio = mem::make_shared<Mmio>();
        mmio->setup_mmap();
        mmio->vram[0][0] = 0x69;
        system.mmio = mmio;

        for (auto function : INSTRUCTION_TABLE_CB) {
            function(system);
        }

        return system.reg_pc == 0;
    };

    static_assert(func());
}

} // namespace dmg::inst
