#include "core/bit.hpp"
#include "core/mem.hpp"

#include <cstdint>
#include <cstring>
#include <cassert>
#include <array>
#include <span>
#include <optional>
#include <string_view>

namespace dmg {

using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;

constexpr auto SCREEN_WIDTH = 160;
constexpr auto SCREEN_HEIGHT = 144;

struct CartHeader {
	u8 entry_point[0x4];
	u8 logo[0x30];
	char title[0x10];
	u16 new_licensee_code;
	u8 sgb_flag;
	u8 cart_type;
	u8 rom_size;
	u8 ram_size;
	u8 destination_code;
	u8 old_licensee_code;
	u8 rom_version;
	u8 header_checksum;
	u16 global_checksum;

    constexpr auto get_title() -> std::string_view {
        return std::string_view{this->title, std::size(this->title)};
    }
};

[[nodiscard]]
auto get_cart_header(std::span<const u8> data) -> std::optional<CartHeader> {
    if (data.size() < sizeof(CartHeader)) {
        return {};
    }

    CartHeader header{};
    std::memcpy(&header, data.data(), sizeof(header));

    return header;
}

struct Mmio {
    std::array<const u8*, 0x10> mmap{};

    std::array<std::array<u8, 0x2000>, 2> vram{};
    std::array<u8, 0xA0> oam{};

    std::array<std::array<u8, 4>, 8> bg_colour{};
    std::array<std::array<u8, 4>, 8> obj_colour{};

    std::array<u8, 0x40> bg_palette{};
    std::array<u8, 0x40> obj_palette{};

    std::array<std::array<u8, 0x1000>, 8> wram{};
    std::array<u8, 0x80> hram{};

    u8 IE;
    u8 IF;

    // these are tests for if they work in constexpr (they do!)
    constexpr auto setup_mmap() noexcept -> void {
        this->mmap[0] = this->vram[0].data();
    }

    constexpr auto io_read(const u16 addr) noexcept -> u8 {
        switch (addr & 0x7F) {
            case 0x00: // joypad
                return 0xFF;
            case 0x01: // serial
                return 0xFF;
            case 0x02: // serial
                return 0xFF;
            case 0x4D: // double speed
                return 0xFF;
            case 0x70: // SVBK
                return 0xFF;
        }

        return 0xFF;
    }

    constexpr auto io_write(const u16 addr, [[maybe_unused]] const u8 v) noexcept -> void {
        switch (addr & 0x7F) {
            case 0x00: // joypad
                break;
            case 0x01: // serial
                break;
            case 0x02: // serial
                break;
            case 0x4D: // double speed
                break;
            case 0x70: // SVBK
                break;
        }
    }

    constexpr auto read(const u16 addr) -> u8 {
        return 0;
        if (addr < 0xFE00) [[likely]] {
            return this->mmap[(addr >> 12)][addr & 0x0FFF];
        } else if (addr <= 0xFE9F) {
            return this->oam[addr & 0x9F];
        } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
            return this->io_read(addr);
        } else if (addr >= 0xFF80) {
            return this->hram[addr & 0x7F];
        } else if (addr == 0xFFFF) {
            return 0xFF;
        }

        return 0xFF;
    }

    constexpr auto write(const u16 addr, const u8 v) -> void {
        if (addr < 0xFE00) [[likely]] {
            switch ((addr >> 12) & 0xF) {
                case 0x0: case 0x1: case 0x2: case 0x3: case 0x4:
                case 0x5: case 0x6: case 0x7: case 0xA: case 0xB:
                    // mbc write
                    break;
                case 0x8: case 0x9:
                    this->vram[0][addr & 0x1FFF] = v;
                    break;
                case 0xC: case 0xE:
                    this->wram[0][addr & 0x0FFF] = v;
                    break;
                case 0xD: case 0xF:
                    this->wram[1][addr & 0x0FFF] = v;
                    break;
            }
        } else if (addr <= 0xFE9F) {
            this->oam[addr & 0x9F] = v;
        } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
            this->io_write(addr, v);
        } else if (addr >= 0xFF80) {
            this->hram[addr & 0x7F] = v;
        } else if (addr == 0xFFFF) {
            // IE
        }
    }
};

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
// static_assert(14 == sizeof(System), "Sys size changed!");

namespace inst {

enum class Reg8Type {
    B, C, D, E, H, L, HL, A
};

enum class Reg16Type {
    BC, DE, HL, SP
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

template <Reg16Type type>
constexpr auto get_reg(System& sys) noexcept {
    if constexpr(type == Reg16Type::BC) { return sys.get_reg<Reg16::BC>(); }
    if constexpr(type == Reg16Type::DE) { return sys.get_reg<Reg16::DE>(); }
    if constexpr(type == Reg16Type::HL) { return sys.get_reg<Reg16::HL>(); }
    if constexpr(type == Reg16Type::SP) { return sys.get_reg<Reg16::SP>(); }
}

template <Reg16Type type>
constexpr auto set_reg(System& sys, const u16 v) noexcept -> void {
    if constexpr(type == Reg16Type::BC) { sys.set_reg<Reg16::BC>(v); }
    if constexpr(type == Reg16Type::DE) { sys.set_reg<Reg16::DE>(v); }
    if constexpr(type == Reg16Type::HL) { sys.set_reg<Reg16::HL>(v); }
    if constexpr(type == Reg16Type::SP) { sys.set_reg<Reg16::SP>(v); }
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

template <Reg16Type src, Reg8Type dst>
constexpr auto inst_ld([[maybe_unused]] System& sys) noexcept -> void { }

template <Reg8Type dst>
constexpr auto inst_ld_u8([[maybe_unused]] System& sys) noexcept -> void {
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

template <Reg16Type type>
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

template <Reg16Type type>
constexpr auto inst_dec(System& sys) noexcept -> void { 
    const auto value = get_reg<type>(sys);
    const auto result = value - 1;
    set_reg<type>(sys, result);
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
    table.fill(inst_unk);
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
        auto mmio = mem::make_shared<Mmio>();
        mmio->setup_mmap();
        mmio->vram[0][0] = 0x69;
        system.mmio = mmio;

        for (auto function : INSTRUCTION_TABLE_CB) {
            function(system);
        }

        // return system.mmio->read(0) == 0x69;
        return system.reg_pc == 0;
    };

    static_assert(func());
}

} // namespace inst
} // namespace dmg

auto main() -> int {

    return 0;
}