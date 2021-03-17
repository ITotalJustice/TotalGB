#pragma once

#include "common.hpp"
#include "mmio.hpp"
#include "bit.hpp"
#include "mem.hpp"

namespace dmg {

enum class Flag {
    C, H, N, Z
};

enum class Reg8 {
    B, C, D, E, H, L, A, F
};

enum class Reg16 {
    BC, DE, HL, AF, SP, PC
};

[[nodiscard]]
constexpr auto pair_u16(const u8 lo, const u8 hi) noexcept -> u16 {
    return (lo << 8) | hi;
}

struct System {
    mem::shared_ptr<Mmio> mmio;

    u16 reg_pc;
    u16 reg_sp;

    u16 cycles;

    // might just represent these are bytes to avoid shifting
    u8 flag_z : 1;
    u8 flag_n : 1;
    u8 flag_h : 1;
    u8 flag_c : 1;
    u8 ime : 1;
    u8 halt : 1;

    u8 reg_b;
    u8 reg_c;
    u8 reg_d;
    u8 reg_e;
    u8 reg_h;
    u8 reg_l;
    u8 reg_a;

    u8 _pad_; // free byte added because of padding

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

    // todo: try and replace the set_flags with something that sets the
    // flags at compile time...
    constexpr auto set_flag(const Flag flag, const bool v) noexcept -> void {
        switch (flag) {
            case Flag::C: this->flag_c = v; break;
            case Flag::H: this->flag_h = v; break;
            case Flag::N: this->flag_n = v; break;
            case Flag::Z: this->flag_z = v; break;
        }
    }

    constexpr auto set_flags(const Flag flag, const bool v, const auto ...args) noexcept -> void {
        this->set_flag(flag, v);
        if constexpr(sizeof...(args) > 0) {
            this->set_flags(args...);
        }
    }

    [[nodiscard]]
    constexpr auto get_reg_f() const noexcept -> u8 {
        return (this->get_flag<Flag::Z>() << 7) | (this->get_flag<Flag::N>() << 6) | (this->get_flag<Flag::H>() << 5) | (this->get_flag<Flag::C>() << 4);
    }

    constexpr auto set_reg_f(const u8 v) noexcept {
        this->set_flag<Flag::Z>(bit::is_set<7>(v));
        this->set_flag<Flag::N>(bit::is_set<6>(v));
        this->set_flag<Flag::H>(bit::is_set<5>(v));
        this->set_flag<Flag::C>(bit::is_set<4>(v));
    }

    template <Reg8 type>
    constexpr auto get_reg() const noexcept {
        if constexpr(type == Reg8::B) { return this->reg_b; }
        if constexpr(type == Reg8::C) { return this->reg_c; }
        if constexpr(type == Reg8::D) { return this->reg_d; }
        if constexpr(type == Reg8::E) { return this->reg_e; }
        if constexpr(type == Reg8::H) { return this->reg_h; }
        if constexpr(type == Reg8::L) { return this->reg_l; }
        if constexpr(type == Reg8::A) { return this->reg_a; }
        if constexpr(type == Reg8::F) { return this->get_reg_f(); }
    }

    template <Reg8 type>
    constexpr auto set_reg(const u8 v) noexcept -> void {
        if constexpr(type == Reg8::B) { this->reg_b = v; }
        if constexpr(type == Reg8::C) { this->reg_c = v; }
        if constexpr(type == Reg8::D) { this->reg_d = v; }
        if constexpr(type == Reg8::E) { this->reg_e = v; }
        if constexpr(type == Reg8::H) { this->reg_h = v; }
        if constexpr(type == Reg8::L) { this->reg_l = v; }
        if constexpr(type == Reg8::A) { this->reg_a = v; }
        if constexpr(type == Reg8::F) { this->set_reg_f(v); }
    }

    template <Reg16 type> [[nodiscard]]
    constexpr auto get_reg() const noexcept {
        if constexpr(type == Reg16::BC) { return pair_u16(this->get_reg<Reg8::B>(), this->get_reg<Reg8::C>()); }
        if constexpr(type == Reg16::DE) { return pair_u16(this->get_reg<Reg8::D>(), this->get_reg<Reg8::E>()); }
        if constexpr(type == Reg16::HL) { return pair_u16(this->get_reg<Reg8::H>(), this->get_reg<Reg8::L>()); }
        if constexpr(type == Reg16::AF) { return pair_u16(this->get_reg<Reg8::A>(), this->get_reg<Reg8::F>()); }
        if constexpr(type == Reg16::SP) { return this->reg_sp; }
        if constexpr(type == Reg16::PC) { return this->reg_pc; }
    }

    template <Reg16 type> [[nodiscard]]
    constexpr auto get_reg2() const noexcept {
        if constexpr(type == Reg16::BC) { return pair_u16(this->get_reg<Reg8::B>(), this->get_reg<Reg8::C>()); }
        if constexpr(type == Reg16::DE) { return pair_u16(this->get_reg<Reg8::D>(), this->get_reg<Reg8::E>()); }
        if constexpr(type == Reg16::HL) { return pair_u16(this->get_reg<Reg8::H>(), this->get_reg<Reg8::L>()); }
        if constexpr(type == Reg16::AF) { return pair_u16(this->get_reg<Reg8::A>(), this->get_reg<Reg8::F>()); }
        if constexpr(type == Reg16::SP) { return this->reg_sp; }
        if constexpr(type == Reg16::PC) { return this->reg_pc; }
    }

    template <Reg16 type>
    constexpr auto set_reg(const u16 v) noexcept -> void {
        if constexpr(type == Reg16::BC) { this->set_reg<Reg8::B>(v >> 8); this->set_reg<Reg8::C>(v & 0xFF); }
        if constexpr(type == Reg16::DE) { this->set_reg<Reg8::D>(v >> 8); this->set_reg<Reg8::E>(v & 0xFF); }
        if constexpr(type == Reg16::HL) { this->set_reg<Reg8::H>(v >> 8); this->set_reg<Reg8::L>(v & 0xFF); }
        if constexpr(type == Reg16::AF) { this->set_reg<Reg8::A>(v >> 8); this->set_reg<Reg8::F>(v & 0xFF); }
        if constexpr(type == Reg16::SP) { this->reg_sp = v; }
        if constexpr(type == Reg16::PC) { this->reg_pc = v; }
    }
};

constexpr auto sys_size = sizeof(System);

} // namespace dmg
