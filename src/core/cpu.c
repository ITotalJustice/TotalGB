#include "gb.h"
#include "internal.h"
#include "tables/cycle_table.h"

#include <assert.h>

static inline void UNK_OP(struct GB_Data* gb, GB_U8 opcode, GB_BOOL cb_prefix) {
	if (gb->error_cb != NULL) {
		struct GB_ErrorData data = {0};
		data.type = GB_ERROR_TYPE_UNKNOWN_INSTRUCTION;
		data.unk_instruction.cb_prefix = cb_prefix;
		data.unk_instruction.opcode = opcode;

		gb->error_cb(gb, gb->error_cb_user_data, &data);	
	}
}

// todo: prefix with GB
// SINGLE REGS
#define REG_B gb->cpu.registers[0]
#define REG_C gb->cpu.registers[1]
#define REG_D gb->cpu.registers[2]
#define REG_E gb->cpu.registers[3]
#define REG_H gb->cpu.registers[4]
#define REG_L gb->cpu.registers[5]
#define REG_F gb->cpu.registers[6]
// AF is flipped because of how the oprand decoding works.
// F flag isn't used, instead (HL) is reg 6.
// sperate instructions have been created for every (HL) instruction.
#define REG_A gb->cpu.registers[7]
// PAIRS
#if GB_ENDIAN == GB_LITTLE_ENDIAN
#define REG_BC ((REG_B << 8) | REG_C)
#define REG_DE ((REG_D << 8) | REG_E)
#define REG_HL ((REG_H << 8) | REG_L)
#define REG_AF ((REG_A << 8) | (REG_F & 0xF0))
#define SET_REG_BC(v) REG_B = (((v) >> 8) & 0xFF); REG_C = ((v) & 0xFF)
#define SET_REG_DE(v) REG_D = (((v) >> 8) & 0xFF); REG_E = ((v) & 0xFF)
#define SET_REG_HL(v) REG_H = (((v) >> 8) & 0xFF); REG_L = ((v) & 0xFF)
#define SET_REG_AF(v) REG_A = (((v) >> 8) & 0xFF); REG_F = ((v) & 0xF0)
#else
// TODO: BIG ENDIAN (PPC (WII / GAMECUBE))
#endif
#define REG_SP gb->cpu.SP
#define REG_PC gb->cpu.PC

// todo: prefix with GB
#define FLAG_C (!!(REG_F & 0x10))
#define FLAG_H (!!(REG_F & 0x20))
#define FLAG_N (!!(REG_F & 0x40))
#define FLAG_Z (!!(REG_F & 0x80))

#define REG(v) gb->cpu.registers[(v) & 0x7]

#if 1

#define SET_FLAG_C(n) REG_F = n ? REG_F | 0x10 : REG_F & ~(0x10)
#define SET_FLAG_H(n) REG_F = n ? REG_F | 0x20 : REG_F & ~(0x20)
#define SET_FLAG_N(n) REG_F = n ? REG_F | 0x40 : REG_F & ~(0x40)
#define SET_FLAG_Z(n) REG_F = n ? REG_F | 0x80 : REG_F & ~(0x80)

#else
// this (acording to msvc) is UB
// SOURCE: https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/c5105

#define SET_FLAG_C(n) REG_F ^= (-(!!(n)) ^ REG_F) & 0x10
#define SET_FLAG_H(n) REG_F ^= (-(!!(n)) ^ REG_F) & 0x20
#define SET_FLAG_N(n) REG_F ^= (-(!!(n)) ^ REG_F) & 0x40
#define SET_FLAG_Z(n) REG_F ^= (-(!!(n)) ^ REG_F) & 0x80

#endif

void GB_cpu_set_flag(struct GB_Data* gb, enum GB_CpuFlags flag, GB_BOOL value) {
	switch (flag) {
		case GB_CPU_FLAG_C: SET_FLAG_C(value); break;
		case GB_CPU_FLAG_H: SET_FLAG_H(value); break;
		case GB_CPU_FLAG_N: SET_FLAG_N(value); break;
		case GB_CPU_FLAG_Z: SET_FLAG_Z(value); break;
	}
}

GB_BOOL GB_cpu_get_flag(const struct GB_Data* gb, enum GB_CpuFlags flag) {
	switch (flag) {
		case GB_CPU_FLAG_C: return FLAG_C;
		case GB_CPU_FLAG_H: return FLAG_H;
		case GB_CPU_FLAG_N: return FLAG_N;
		case GB_CPU_FLAG_Z: return FLAG_Z;
	}
	
	GB_UNREACHABLE(GB_FALSE);
}

void GB_cpu_set_register(struct GB_Data* gb, enum GB_CpuRegisters reg, GB_U8 value) {
	switch (reg) {
		case GB_CPU_REGISTER_B: REG_B = value; break;
		case GB_CPU_REGISTER_C: REG_C = value; break;
		case GB_CPU_REGISTER_D: REG_D = value; break;
		case GB_CPU_REGISTER_E: REG_E = value; break;
		case GB_CPU_REGISTER_H: REG_H = value; break;
		case GB_CPU_REGISTER_L: REG_L = value; break;
		case GB_CPU_REGISTER_A: REG_A = value; break;
		case GB_CPU_REGISTER_F: REG_F = value; break;
	}
}

GB_U8 GB_cpu_get_register(const struct GB_Data* gb, enum GB_CpuRegisters reg) {
	switch (reg) {
		case GB_CPU_REGISTER_B: return REG_B;
		case GB_CPU_REGISTER_C: return REG_C;
		case GB_CPU_REGISTER_D: return REG_D;
		case GB_CPU_REGISTER_E: return REG_E;
		case GB_CPU_REGISTER_H: return REG_H;
		case GB_CPU_REGISTER_L: return REG_L;
		case GB_CPU_REGISTER_A: return REG_A;
		case GB_CPU_REGISTER_F: return REG_F;
	}

	GB_UNREACHABLE(0xFF);
}

void GB_cpu_set_register_pair(struct GB_Data* gb, enum GB_CpuRegisterPairs pair, GB_U16 value) {
	switch (pair) {
		case GB_CPU_REGISTER_PAIR_BC: SET_REG_BC(value); break;
		case GB_CPU_REGISTER_PAIR_DE: SET_REG_DE(value); break;
		case GB_CPU_REGISTER_PAIR_HL: SET_REG_HL(value); break;
		case GB_CPU_REGISTER_PAIR_AF: SET_REG_AF(value); break;
		case GB_CPU_REGISTER_PAIR_SP: REG_SP = value; break;
		case GB_CPU_REGISTER_PAIR_PC: REG_PC = value; break;
	}
}

GB_U16 GB_cpu_get_register_pair(const struct GB_Data* gb, enum GB_CpuRegisterPairs pair) {
	switch (pair) {
		case GB_CPU_REGISTER_PAIR_BC: return REG_BC;
		case GB_CPU_REGISTER_PAIR_DE: return REG_DE;
		case GB_CPU_REGISTER_PAIR_HL: return REG_HL;
		case GB_CPU_REGISTER_PAIR_AF: return REG_AF;
		case GB_CPU_REGISTER_PAIR_SP: return REG_SP;
		case GB_CPU_REGISTER_PAIR_PC: return REG_PC;
	}

	GB_UNREACHABLE(0xFF);
}

#define SET_FLAGS_HN(h,n) \
SET_FLAG_H(h); \
SET_FLAG_N(n);

#define SET_FLAGS_HZ(h,z) \
SET_FLAG_H(h); \
SET_FLAG_Z(z);

#define SET_FLAGS_HNZ(h,n,z) \
SET_FLAG_H(h); \
SET_FLAG_N(n); \
SET_FLAG_Z(z);

#define SET_FLAGS_CHN(c,h,n) \
SET_FLAG_C(c); \
SET_FLAG_H(h); \
SET_FLAG_N(n);

#define SET_ALL_FLAGS(c,h,n,z) \
SET_FLAG_C(c); \
SET_FLAG_H(h); \
SET_FLAG_N(n); \
SET_FLAG_Z(z);

#define read8(addr) GB_read8(gb, addr)
#define read16(addr) GB_read16(gb, addr)
#define write8(addr,value) GB_write8(gb, addr, value)
#define write16(addr,value) GB_write16(gb, addr, value)

// fwd
static void GB_execute(struct GB_Data* gb);
static void GB_execute_cb(struct GB_Data* gb);

static inline void GB_PUSH(struct GB_Data* gb, GB_U16 value) {
	write8(--REG_SP, (value >> 8) & 0xFF);
	write8(--REG_SP, value & 0xFF);
}

static inline GB_U16 GB_POP(struct GB_Data* gb) {
	const GB_U16 result = read16(REG_SP);
	REG_SP += 2;
	return result;
}

#define PUSH(value) GB_PUSH(gb, value)
#define POP() GB_POP(gb)

#define CALL() do { \
	const GB_U16 result = read16(REG_PC); \
	PUSH(REG_PC + 2); \
	REG_PC = result; \
} while(0)

#define CALL_NZ() do { \
	if (!FLAG_Z) { \
		CALL(); \
		gb->cpu.cycles += 12; \
	} else { \
		REG_PC += 2; \
	} \
} while(0)

#define CALL_Z() do { \
	if (FLAG_Z) { \
		CALL(); \
		gb->cpu.cycles += 12; \
	} else { \
		REG_PC += 2; \
	} \
} while(0)

#define CALL_NC() do { \
	if (!FLAG_C) { \
		CALL(); \
		gb->cpu.cycles += 12; \
	} else { \
		REG_PC += 2; \
	} \
} while(0)

#define CALL_C() do { \
	if (FLAG_C) { \
		CALL(); \
		gb->cpu.cycles += 12; \
	} else { \
		REG_PC += 2; \
	} \
} while(0)

#define JP() do { \
	REG_PC = read16(REG_PC); \
} while(0)

#define JP_HL() do { REG_PC = REG_HL; } while(0)

#define JP_NZ() do { \
	if (!FLAG_Z) { \
		JP(); \
		gb->cpu.cycles += 4; \
	} else { \
		REG_PC += 2; \
	} \
} while(0)

#define JP_Z() do { \
	if (FLAG_Z) { \
		JP(); \
		gb->cpu.cycles += 4; \
	} else { \
		REG_PC += 2; \
	} \
} while(0)

#define JP_NC() do { \
	if (!FLAG_C) { \
		JP(); \
		gb->cpu.cycles += 4; \
	} else { \
		REG_PC += 2; \
	} \
} while(0)

#define JP_C() do { \
	if (FLAG_C) { \
		JP(); \
		gb->cpu.cycles += 4; \
	} else { \
		REG_PC += 2; \
	} \
} while(0)

#define JR() do { \
	REG_PC += ((GB_S8)read8(REG_PC)) + 1; \
} while(0)

#define JR_NZ() do { \
	if (!FLAG_Z) { \
		JR(); \
		gb->cpu.cycles += 4; \
	} else { \
		++REG_PC; \
	} \
} while(0)

#define JR_Z() do { \
	if (FLAG_Z) { \
		JR(); \
		gb->cpu.cycles += 4; \
	} else { \
		++REG_PC; \
	} \
} while(0)

#define JR_NC() do { \
	if (!FLAG_C) { \
		JR(); \
		gb->cpu.cycles += 4; \
	} else { \
		++REG_PC; \
	} \
} while(0)

#define JR_C() do { \
	if (FLAG_C) { \
		JR(); \
		gb->cpu.cycles += 4; \
	} else { \
		++REG_PC; \
	} \
} while(0)

#define RET() do { \
	REG_PC = POP(); \
} while(0)

#define RET_NZ() do { \
	if (!FLAG_Z) { \
		RET(); \
		gb->cpu.cycles += 12; \
	} \
} while(0)

#define RET_Z() do { \
	if (FLAG_Z) { \
		RET(); \
		gb->cpu.cycles += 12; \
	} \
} while(0)

#define RET_NC() do { \
	if (!FLAG_C) { \
		RET(); \
		gb->cpu.cycles += 12; \
	} \
} while(0)

#define RET_C() do { \
	if (FLAG_C) { \
		RET(); \
		gb->cpu.cycles += 12; \
	} \
} while(0)

#define INC_r() do { \
	REG((opcode >> 3))++; \
	SET_FLAGS_HNZ(((REG((opcode >> 3)) & 0xF) == 0), GB_FALSE, REG((opcode >> 3)) == 0); \
} while(0)

#define INC_HLa() do { \
	const GB_U8 result = read8(REG_HL) + 1; \
	write8(REG_HL, result); \
	SET_FLAGS_HNZ((result & 0xF) == 0, GB_FALSE, result == 0); \
} while (0)

#define DEC_r() do { \
	--REG((opcode >> 3)); \
	SET_FLAGS_HNZ(((REG((opcode >> 3)) & 0xF) == 0xF), GB_TRUE, REG((opcode >> 3)) == 0); \
} while(0)

#define DEC_HLa() do { \
	const GB_U8 result = read8(REG_HL) - 1; \
	write8(REG_HL, result); \
	SET_FLAGS_HNZ((result & 0xF) == 0xF, GB_TRUE, result == 0); \
} while(0)

#define INC_BC() do { SET_REG_BC(REG_BC + 1); } while(0)
#define INC_DE() do { SET_REG_DE(REG_DE + 1); } while(0)
#define INC_HL() do { SET_REG_HL(REG_HL + 1); } while(0)
#define INC_SP() do { ++REG_SP; } while(0)
#define DEC_BC() do { SET_REG_BC(REG_BC - 1); } while(0)
#define DEC_DE() do { SET_REG_DE(REG_DE - 1); } while(0)
#define DEC_HL() do { SET_REG_HL(REG_HL - 1); } while(0)
#define DEC_SP() do { --REG_SP; } while(0)

#define CP() do { \
	const GB_U8 result = REG_A - REG(opcode); \
	SET_ALL_FLAGS(REG(opcode) < REG_A, (REG_A & 0xF) < (REG(opcode) & 0xF), GB_TRUE, result == 0); \
} while(0)

#define LD_r_r() do { REG(opcode >> 3) = REG(opcode); } while(0)
#define LD_r_u8() do { REG(opcode >> 3) = read8(REG_PC++); } while(0)
#define LD_HLa_r() do { write8(REG_HL, REG(opcode)); } while(0)
#define LD_HLa_u8() do { write8(REG_HL, read8(REG_PC++)); } while(0)
#define LD_r_HLa() do { REG(opcode >> 3) = read8(REG_HL); } while(0)
#define LD_SP_u16() do { REG_SP = read16(REG_PC); REG_PC+=2; } while(0)
#define LD_A_u16() do { REG_A = read8(read16(REG_PC)); REG_PC+=2; } while(0)
#define LD_u16_A() do { write8(read16(REG_PC), REG_A); REG_PC+=2; } while(0)

#define LD_HLi_A() do { write8(REG_HL, REG_A); INC_HL(); } while(0)
#define LD_A_HLi() do { REG_A = read8(REG_HL); INC_HL(); } while(0)
#define LD_HLd_A() do { write8(REG_HL, REG_A); DEC_HL(); } while(0)
#define LD_A_HLd() do { REG_A = read8(REG_HL); DEC_HL(); } while(0)

#define LD_A_BCa() do { REG_A = read8(REG_BC); } while(0)
#define LD_A_DEa() do { REG_A = read8(REG_DE); } while(0)
#define LD_A_HLa() do { REG_A = read8(REG_HL); } while(0)
#define LD_A_AFa() do { REG_A = read8(REG_AF); } while(0)
#define LD_BCa_A() do { write8(REG_BC, REG_A); } while(0)
#define LD_DEa_A() do { write8(REG_DE, REG_A); } while(0)
#define LD_HLa_A() do { write8(REG_HL, REG_A); } while(0)
#define LD_AFa_A() do { write8(REG_AF, REG_A); } while(0)

#define LD_FFRC_A() do { write8(0xFF00 | REG_C, REG_A); } while(0);
#define LD_A_FFRC() do { REG_A = read8(0xFF00 | REG_C); } while(0);

#define LD_BC_u16() do { \
	const GB_U16 result = read16(REG_PC); \
	SET_REG_BC(result); \
	REG_PC += 2; \
} while(0)
#define LD_DE_u16() do { \
	const GB_U16 result = read16(REG_PC); \
	SET_REG_DE(result); \
	REG_PC += 2; \
} while(0)
#define LD_HL_u16() do { \
	const GB_U16 result = read16(REG_PC); \
	SET_REG_HL(result); \
	REG_PC += 2; \
} while(0)

#define LD_SP_u16() do { REG_SP = read16(REG_PC); REG_PC+=2; } while(0)
#define LD_u16_BC() do { write16(read16(REG_PC), REG_BC); REG_PC+=2; } while(0)
#define LD_u16_DE() do { write16(read16(REG_PC), REG_DE); REG_PC+=2; } while(0)
#define LD_u16_HL() do { write16(read16(REG_PC), REG_HL); REG_PC+=2; } while(0)
#define LD_u16_SP() do { write16(read16(REG_PC), REG_SP); REG_PC+=2; } while(0)
#define LD_FFu8_A() do { write8((0xFF00 | read8(REG_PC++)), REG_A); } while(0)
#define LD_A_FFu8() do { REG_A = read8(0xFF00 | read8(REG_PC++)); } while(0)
#define LD_SP_HL() do { REG_SP = REG_HL; } while(0);

#define CP_r() do { \
	const GB_U8 value = REG(opcode); \
	const GB_U8 result = REG_A - value; \
	SET_ALL_FLAGS(value > REG_A, (REG_A & 0xF) < (value & 0xF), GB_TRUE, result == 0); \
} while(0)

#define CP_u8() do { \
	const GB_U8 value = read8(REG_PC++); \
	const GB_U8 result = REG_A - value; \
	SET_ALL_FLAGS(value > REG_A, (REG_A & 0xF) < (value & 0xF), GB_TRUE, result == 0); \
} while(0)

#define CP_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
	const GB_U8 result = REG_A - value; \
	SET_ALL_FLAGS(value > REG_A, (REG_A & 0xF) < (value & 0xF), GB_TRUE, result == 0); \
} while(0)

#define __ADD(value, carry) do { \
	const GB_U8 result = REG_A + value + carry; \
    SET_ALL_FLAGS((REG_A + value + carry) > 0xFF, ((REG_A & 0xF) + (value & 0xF) + carry) > 0xF, GB_FALSE, result == 0); \
    REG_A = result; \
} while (0)

#define ADD_r() do { \
	__ADD(REG(opcode), GB_FALSE); \
} while(0)

#define ADD_u8() do { \
	const GB_U8 value = read8(REG_PC++); \
	__ADD(value, GB_FALSE); \
} while(0)

#define ADD_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
	__ADD(value, GB_FALSE); \
} while(0)

#define __ADD_HL(value) do { \
	const GB_U16 result = REG_HL + value; \
	SET_FLAGS_CHN((REG_HL + value) > 0xFFFF, (REG_HL & 0xFFF) + (value & 0xFFF) > 0xFFF, GB_FALSE); \
	SET_REG_HL(result); \
} while(0)

#define ADD_HL_BC() do { __ADD_HL(REG_BC); } while(0)
#define ADD_HL_DE() do { __ADD_HL(REG_DE); } while(0)
#define ADD_HL_HL() do { __ADD_HL(REG_HL); } while(0)
#define ADD_HL_SP() do { __ADD_HL(REG_SP); } while(0)
#define ADD_SP_i8() do { \
	const GB_U8 value = read8(REG_PC++); \
    const GB_U16 result = REG_SP + (GB_S8)value; \
    SET_ALL_FLAGS(((REG_SP & 0xFF) + value) > 0xFF, ((REG_SP & 0xF) + (value & 0xF)) > 0xF, GB_FALSE, GB_FALSE); \
	REG_SP = result; \
} while (0)

#define LD_HL_SP_i8() do { \
	const GB_U8 value = read8(REG_PC++); \
    const GB_U16 result = REG_SP + (GB_S8)value; \
    SET_ALL_FLAGS(((REG_SP & 0xFF) + value) > 0xFF, ((REG_SP & 0xF) + (value & 0xF)) > 0xF, GB_FALSE, GB_FALSE); \
	SET_REG_HL(result); \
} while (0)

#define ADC_r() do { \
	const GB_BOOL fc = FLAG_C; \
	__ADD(REG(opcode), fc); \
} while(0)

#define ADC_u8() do { \
	const GB_U8 value = read8(REG_PC++); \
	const GB_BOOL fc = FLAG_C; \
	__ADD(value, fc); \
} while(0)

#define ADC_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
	const GB_BOOL fc = FLAG_C; \
	__ADD(value, fc); \
} while(0)

#define __SUB(value, carry) do { \
	const GB_U8 result = REG_A - value - carry; \
	SET_ALL_FLAGS((value + carry) > REG_A, (REG_A & 0xF) < ((value & 0xF) + carry), GB_TRUE, result == 0); \
    REG_A = result; \
} while (0)

#define SUB_r() do { \
	__SUB(REG(opcode), GB_FALSE); \
} while(0)

#define SUB_u8() do { \
	const GB_U8 value = read8(REG_PC++); \
	__SUB(value, GB_FALSE); \
} while(0)

#define SUB_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
	__SUB(value, GB_FALSE); \
} while(0)

#define SBC_r() do { \
	const GB_BOOL fc = FLAG_C; \
	__SUB(REG(opcode), fc); \
} while(0)

#define SBC_u8() do { \
	const GB_U8 value = read8(REG_PC++); \
	const GB_BOOL fc = FLAG_C; \
	__SUB(value, fc); \
} while(0)

#define SBC_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
	const GB_BOOL fc = FLAG_C; \
	__SUB(value, fc); \
} while(0)

#define AND_r() do { \
	REG_A &= REG(opcode); \
	SET_ALL_FLAGS(GB_FALSE, GB_TRUE, GB_FALSE, REG_A == 0); \
} while(0)

#define AND_u8() do { \
	REG_A &= read8(REG_PC++); \
	SET_ALL_FLAGS(GB_FALSE, GB_TRUE, GB_FALSE, REG_A == 0); \
} while(0)

#define AND_HLa() do { \
	REG_A &= read8(REG_HL); \
	SET_ALL_FLAGS(GB_FALSE, GB_TRUE, GB_FALSE, REG_A == 0); \
} while(0)

#define XOR_r() do { \
	REG_A ^= REG(opcode); \
	SET_ALL_FLAGS(GB_FALSE, GB_FALSE, GB_FALSE, REG_A == 0); \
} while(0)

#define XOR_u8() do { \
	REG_A ^= read8(REG_PC++); \
	SET_ALL_FLAGS(GB_FALSE, GB_FALSE, GB_FALSE, REG_A == 0); \
} while(0)

#define XOR_HLa() do { \
	REG_A ^= read8(REG_HL); \
	SET_ALL_FLAGS(GB_FALSE, GB_FALSE, GB_FALSE, REG_A == 0); \
} while(0)

#define OR_r() do { \
	REG_A |= REG(opcode); \
	SET_ALL_FLAGS(GB_FALSE, GB_FALSE, GB_FALSE, REG_A == 0); \
} while(0)

#define OR_u8() do { \
	REG_A |= read8(REG_PC++); \
	SET_ALL_FLAGS(GB_FALSE, GB_FALSE, GB_FALSE, REG_A == 0); \
} while(0)

#define OR_HLa() do { \
	REG_A |= read8(REG_HL); \
	SET_ALL_FLAGS(GB_FALSE, GB_FALSE, GB_FALSE, REG_A == 0); \
} while(0)

#define DI() do { gb->cpu.ime = GB_FALSE; } while(0)
#define EI() do { gb->cpu.ime = GB_TRUE; } while(0)

#define POP_BC() do { \
	const GB_U16 result = POP(); \
	SET_REG_BC(result); \
} while(0)

#define POP_DE() do { \
	const GB_U16 result = POP(); \
	SET_REG_DE(result); \
} while(0)

#define POP_HL() do { \
	const GB_U16 result = POP(); \
	SET_REG_HL(result); \
} while(0)

#define POP_AF() do { \
	const GB_U16 result = POP(); \
	SET_REG_AF(result); \
} while(0)

#define RL_r() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) = (REG(opcode) << 1) | (FLAG_C); \
	SET_ALL_FLAGS(value >> 7, GB_FALSE, GB_FALSE, REG(opcode) == 0); \
} while(0)

#define RLA() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) = (REG(opcode) << 1) | (FLAG_C); \
	SET_ALL_FLAGS(value >> 7, GB_FALSE, GB_FALSE, GB_FALSE); \
} while(0)

#define RL_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
	const GB_U8 result = (value << 1) | (FLAG_C); \
	write8(REG_HL, result); \
	SET_ALL_FLAGS(value >> 7, GB_FALSE, GB_FALSE, result == 0); \
} while (0)

#define RLC_r() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) = (REG(opcode) << 1) | ((REG(opcode) >> 7) & 1); \
	SET_ALL_FLAGS(value >> 7, GB_FALSE, GB_FALSE, REG(opcode) == 0); \
} while(0)

#define RLC_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
	const GB_U8 result = (value << 1) | ((value >> 7) & 1); \
	write8(REG_HL, result); \
	SET_ALL_FLAGS(value >> 7, GB_FALSE, GB_FALSE, result == 0); \
} while(0)

#define RLCA() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) = (REG(opcode) << 1) | ((REG(opcode) >> 7) & 1); \
	SET_ALL_FLAGS(value >> 7, GB_FALSE, GB_FALSE, GB_FALSE); \
} while(0)

#define RR_r() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) = (REG(opcode) >> 1) | (FLAG_C << 7); \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, REG(opcode) == 0); \
} while(0)

#define RR_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
	const GB_U8 result = (value >> 1) | (FLAG_C << 7); \
	write8(REG_HL, result); \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, result == 0); \
} while(0)

#define RRA() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) = (REG(opcode) >> 1) | (FLAG_C << 7); \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, GB_FALSE); \
} while(0)

#define RRC_r() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) = (REG(opcode) >> 1) | (REG(opcode) << 7); \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, REG(opcode) == 0); \
} while(0)

#define RRCA() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) = (REG(opcode) >> 1) | (REG(opcode) << 7); \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, GB_FALSE); \
} while(0)

#define RRC_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
    const GB_U8 result = (value >> 1) | (value << 7); \
	write8(REG_HL, result); \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, result == 0); \
} while (0)

#define SLA_r() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) <<= 1; \
	SET_ALL_FLAGS(value >> 7, GB_FALSE, GB_FALSE, REG(opcode) == 0); \
} while(0)

#define SLA_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
    const GB_U8 result = value << 1; \
	write8(REG_HL, result); \
	SET_ALL_FLAGS(value >> 7, GB_FALSE, GB_FALSE, result == 0); \
} while(0)

#define SRA_r() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) = (REG(opcode) >> 1) | (REG(opcode) & 0x80); \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, REG(opcode) == 0); \
} while(0)

#define SRA_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
    const GB_U8 result = (value >> 1) | (value & 0x80); \
	write8(REG_HL, result); \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, result == 0); \
} while(0)

#define SRL_r() do { \
	const GB_U8 value = REG(opcode); \
	REG(opcode) >>= 1; \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, REG(opcode) == 0); \
} while(0)

#define SRL_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
    const GB_U8 result = (value >> 1); \
	write8(REG_HL, result); \
	SET_ALL_FLAGS(value & 1, GB_FALSE, GB_FALSE, result == 0); \
} while(0)

#define SWAP_r() do { \
    REG(opcode) = (REG(opcode) << 4) | (REG(opcode) >> 4); \
	SET_ALL_FLAGS(GB_FALSE, GB_FALSE, GB_FALSE, REG(opcode) == 0); \
} while(0)

#define SWAP_HLa() do { \
	const GB_U8 value = read8(REG_HL); \
    const GB_U8 result = (value << 4) | (value >> 4); \
	write8(REG_HL, result); \
	SET_ALL_FLAGS(GB_FALSE, GB_FALSE, GB_FALSE, result == 0); \
} while(0)

#define BIT_r() do { \
    SET_FLAGS_HNZ(GB_TRUE, GB_FALSE, (REG(opcode) & (1 << ((opcode >> 3) & 0x7))) == 0); \
} while(0)

#define BIT_HLa() do { \
	SET_FLAGS_HNZ(GB_TRUE, GB_FALSE, (read8(REG_HL) & (1 << ((opcode >> 3) & 0x7))) == 0); \
} while(0)

#define RES_r() do { \
	REG(opcode) &= ~(1 << ((opcode >> 3) & 0x7)); \
} while(0)

#define RES_HLa() do { \
	write8(REG_HL, (read8(REG_HL)) & ~(1 << ((opcode >> 3) & 0x7))); \
} while(0)

#define SET_r() do { \
	REG(opcode) |= (1 << ((opcode >> 3) & 0x7)); \
} while(0)

#define SET_HLa() do { \
	write8(REG_HL, (read8(REG_HL)) | (1 << ((opcode >> 3) & 0x7))); \
} while(0)

#define DAA() do { \
	if (FLAG_N) { \
        if (FLAG_C) { \
            REG_A -= 0x60; \
            SET_FLAG_C(GB_TRUE); \
        } \
        if (FLAG_H) { \
            REG_A -= 0x6; \
        } \
    } else { \
        if (FLAG_C || REG_A > 0x99) { \
            REG_A += 0x60; \
            SET_FLAG_C(GB_TRUE); \
        } \
        if (FLAG_H || (REG_A & 0x0F) > 0x09) { \
            REG_A += 0x6; \
        } \
    } \
	SET_FLAGS_HZ(GB_FALSE, REG_A == 0); \
} while(0)

#define RETI() do { \
	REG_PC = POP(); \
	EI(); \
} while (0)

#define RST(value) do { \
	PUSH(REG_PC); \
	REG_PC = value; \
} while (0)

#define CPL() do { REG_A = ~REG_A; SET_FLAGS_HN(GB_TRUE, GB_TRUE); } while(0)
#define SCF() do { SET_FLAGS_CHN(GB_TRUE, GB_FALSE, GB_FALSE); } while (0)
#define CCF() do { SET_FLAGS_CHN(FLAG_C ^ 1, GB_FALSE, GB_FALSE); } while(0)

#define HALT() do { \
	gb->cpu.halt = GB_TRUE; \
	if (gb->halt_cb != NULL) { \
		gb->halt_cb(gb, gb->halt_cb_user_data); \
	} \
} while(0)

#define STOP() do { \
	if (GB_is_system_gbc(gb)) { \
		if ((IO_KEY1 & 1) == 1) { \
			gb->cpu.double_speed = !gb->cpu.double_speed; \
			IO_KEY1 = 0; \
		} \
	} \
	\
	gb->cpu.cycles += 2050; \
	if (gb->stop_cb != NULL) { \
		gb->stop_cb(gb, gb->stop_cb_user_data); \
	} \
} while (0)

static void GB_interrupt_handler(struct GB_Data* gb) {
	if (!gb->cpu.ime && !gb->cpu.halt) {
		return;
	}
	
	const GB_U8 live_interrupts = IO_IF & IO_IE;
	if (!live_interrupts) {
		return;
	}

	gb->cpu.halt = GB_FALSE;
	
	if (!gb->cpu.ime) {
		return;
	}
	gb->cpu.ime = GB_FALSE;

#if defined(__has_builtin) && __has_builtin(__builtin_ctz)

	const GB_U8 ctz = __builtin_ctz(live_interrupts);
	const GB_U8 reset_vector = 64 | (ctz << 3);
	RST(reset_vector);
	GB_disable_interrupt(gb, 1 << ctz);
#else
	if (live_interrupts & GB_INTERRUPT_VBLANK) {
		RST(64);
		GB_disable_interrupt(gb, GB_INTERRUPT_VBLANK);
    } else if (live_interrupts & GB_INTERRUPT_LCD_STAT) {
		RST(72);
		GB_disable_interrupt(gb, GB_INTERRUPT_LCD_STAT);
    } else if (live_interrupts & GB_INTERRUPT_TIMER) {
		RST(80);
		GB_disable_interrupt(gb, GB_INTERRUPT_TIMER);
    } else if (live_interrupts & GB_INTERRUPT_SERIAL) {
		RST(88);
		GB_disable_interrupt(gb, GB_INTERRUPT_SERIAL);
    } else if (live_interrupts & GB_INTERRUPT_JOYPAD) {
		RST(96);
		GB_disable_interrupt(gb, GB_INTERRUPT_JOYPAD);
    }

#endif /* __has_builtin && __has_builtin(__builtin_ctz) */

	gb->cpu.cycles += 20;
}

static void GB_execute(struct GB_Data* gb) {
	const GB_U8 opcode = read8(REG_PC++);

	switch (opcode) {
		case 0x00: break; // nop
		case 0x01: LD_BC_u16(); break;
		case 0x02: LD_BCa_A(); break;
		case 0x03: INC_BC(); break;
		
		case 0x04: case 0x0C: case 0x14: case 0x1C: case 0x24: case 0x2C: case 0x3C: INC_r(); break;
		case 0x05: case 0x0D: case 0x15: case 0x1D: case 0x25: case 0x2D: case 0x3D: DEC_r(); break;

		case 0x06: LD_r_u8(); break;
		case 0x07: RLCA(); break;
		case 0x08: LD_u16_SP(); break;
		case 0x0A: LD_A_BCa(); break;
		case 0x09: ADD_HL_BC(); break;
		case 0x0B: DEC_BC(); break;
		case 0x0E: LD_r_u8(); break;
		case 0x0F: RRCA(); break;
		case 0x10: STOP(); break;
		case 0x11: LD_DE_u16(); break;
		case 0x12: LD_DEa_A(); break;
		case 0x13: INC_DE(); break;
		case 0x16: LD_r_u8(); break;
		case 0x17: RLA(); break;
		case 0x18: JR(); break;
		case 0x19: ADD_HL_DE(); break;
		case 0x1A: LD_A_DEa(); break;
		case 0x1B: DEC_DE(); break;
		case 0x1E: LD_r_u8(); break;
		case 0x1F: RRA(); break;
		case 0x20: JR_NZ(); break;
		case 0x21: LD_HL_u16(); break;
		case 0x22: LD_HLi_A(); break;
		case 0x23: INC_HL(); break;
		case 0x26: LD_r_u8(); break;
		case 0x27: DAA(); break;
		case 0x28: JR_Z(); break;
		case 0x29: ADD_HL_HL(); break;
		case 0x2A: LD_A_HLi(); break;
		case 0x2B: DEC_HL(); break;
		case 0x2E: LD_r_u8(); break;
		case 0x2F: CPL(); break;
		case 0x30: JR_NC(); break;
		case 0x31: LD_SP_u16(); break;
		case 0x32: LD_HLd_A(); break;
		case 0x33: INC_SP(); break;
		case 0x34: INC_HLa(); break;
		case 0x35: DEC_HLa(); break;
		case 0x36: LD_HLa_u8(); break;
		case 0x37: SCF(); break;
		case 0x38: JR_C(); break;
		case 0x39: ADD_HL_SP(); break;
		case 0x3A: LD_A_HLd(); break;
		case 0x3B: DEC_SP(); break;
		case 0x3E: LD_r_u8(); break;
		case 0x3F: CCF(); break;

		case 0x41: case 0x42: case 0x43: case 0x44:
		case 0x45: case 0x47: case 0x48: case 0x4A:
		case 0x4B: case 0x4C: case 0x4D: case 0x4F:
		case 0x50: case 0x51: case 0x53: case 0x54:
		case 0x55: case 0x57: case 0x58: case 0x59:
		case 0x5A: case 0x5C: case 0x5D: case 0x5F:
		case 0x60: case 0x61: case 0x62: case 0x63:
		case 0x65: case 0x67: case 0x68: case 0x69:
		case 0x6A: case 0x6B: case 0x6C: case 0x6F:
		case 0x78: case 0x79: case 0x7A: case 0x7B:
		case 0x7C: case 0x7D:
			LD_r_r();
			break;

		case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: LD_r_HLa(); break;

		case 0x40: break; // nop LD b,b
		case 0x49: break; // nop LD c,c
		case 0x52: break; // nop LD d,d
		case 0x5B: break; // nop LD e,e
		case 0x64: break; // nop LD h,h
		case 0x6D: break; // nop LD l,l
		case 0x7F: break; // nop LD a,a

		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77: LD_HLa_r(); break;

		case 0x76: HALT(); break;
		case 0x7E: LD_A_HLa(); break;

		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87: ADD_r(); break;
		case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F: ADC_r(); break;
		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97: SUB_r(); break;
		case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F: SBC_r(); break;
		case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7: AND_r(); break;
		case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF: XOR_r(); break;
		case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7: OR_r(); break;
		case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF: CP_r(); break;

		case 0x86: ADD_HLa(); break;
		case 0x8E: ADC_HLa(); break;
		case 0x96: SUB_HLa(); break;
		case 0x9E: SBC_HLa(); break;
		case 0xA6: AND_HLa(); break;
		case 0xAE: XOR_HLa(); break;
		case 0xB6: OR_HLa(); break;
		case 0xBE: CP_HLa(); break;
		case 0xC0: RET_NZ(); break;
		case 0xC1: POP_BC(); break;
		case 0xC2: JP_NZ(); break;
		case 0xC3: JP();  break;
		case 0xC4: CALL_NZ(); break;
		case 0xC5: PUSH(REG_BC); break;
		case 0xC6: ADD_u8(); break;
		case 0xC7: RST(0x00); break;
		case 0xC8: RET_Z(); break;
		case 0xC9: RET(); break;
		case 0xCA: JP_Z(); break;
		// return here as to not increase the cycles from this opcode!
		case 0xCB: GB_execute_cb(gb); return;
		case 0xCC: CALL_Z(); break;
		case 0xCD: CALL(); break;
		case 0xCE: ADC_u8(); break;
		case 0xCF: RST(0x08); break;
		case 0xD0: RET_NC(); break;
		case 0xD1: POP_DE();  break;
		case 0xD2: JP_NC(); break;
		case 0xD4: CALL_NC(); break;
		case 0xD5: PUSH(REG_DE); break;
		case 0xD6: SUB_u8(); break;
		case 0xD7: RST(0x10); break;
		case 0xD8: RET_C(); break;
		case 0xD9: RETI(); break;
		case 0xDA: JP_C(); break;
		case 0xDC: CALL_C(); break;
		case 0xDE: SBC_u8(); break;
		case 0xDF: RST(0x18); break;
		case 0xE0: LD_FFu8_A(); break;
		case 0xE1: POP_HL(); break;
		case 0xE2: LD_FFRC_A(); break;
		case 0xE5: PUSH(REG_HL); break;
		case 0xE6: AND_u8(); break;
		case 0xE7: RST(0x20); break;
		case 0xE8: ADD_SP_i8(); break;
		case 0xE9: JP_HL(); break;
		case 0xEA: LD_u16_A(); break;
		case 0xEE: XOR_u8(); break;
		case 0xEF: RST(0x28); break;
		case 0xF0: LD_A_FFu8(); break;
		case 0xF1: POP_AF(); break;
		case 0xF2: LD_A_FFRC(); break;
		case 0xF3: DI(); break;
		case 0xF5: PUSH(REG_AF); break;
		case 0xF6: OR_u8(); break;
		case 0xF7: RST(0x30); break;
		case 0xF8: LD_HL_SP_i8(); break;
		case 0xF9: LD_SP_HL(); break;
		case 0xFA: LD_A_u16(); break;
		case 0xFB: EI(); break;
		case 0xFE: CP_u8(); break;
		case 0xFF: RST(0x38); break;

		default:
			UNK_OP(gb, opcode, GB_FALSE);
			break;
	}

	gb->cpu.cycles += CYCLE_TABLE[opcode];
}

static void GB_execute_cb(struct GB_Data* gb) {
	const GB_U8 opcode = read8(REG_PC++);

	switch (opcode) {
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x04: case 0x05: case 0x07:
			RLC_r();
			break;
		
		case 0x06:
			RLC_HLa();
			break;
		
		case 0x08: case 0x09: case 0x0A: case 0x0B:
		case 0x0C: case 0x0D: case 0x0F:
			RRC_r();
			break;
		
		case 0x0E:
			RRC_HLa();
			break;

		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x17:
			RL_r();
			break;
		
		case 0x16:
			RL_HLa();
			break;

		case 0x18: case 0x19: case 0x1A: case 0x1B:
		case 0x1C: case 0x1D: case 0x1F:
			RR_r();
			break;
		
		case 0x1E:
			RR_HLa();
			break;

		case 0x20: case 0x21: case 0x22: case 0x23:
		case 0x24: case 0x25: case 0x27:
			SLA_r();
			break;

		case 0x26:
			SLA_HLa();
			break;

		case 0x28: case 0x29: case 0x2A: case 0x2B:
		case 0x2C: case 0x2D: case 0x2F:
			SRA_r();
			break;
		
		case 0x2E:
			SRA_HLa();
			break;

		case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x34: case 0x35: case 0x37: SWAP_r(); break;
		
		case 0x36: SWAP_HLa(); break;

		case 0x38: case 0x39: case 0x3A: case 0x3B:
		case 0x3C: case 0x3D: case 0x3F:
			SRL_r();
			break;

		case 0x3E:
			SRL_HLa();
			break;

		case 0x40: case 0x41: case 0x42: case 0x43:
		case 0x44: case 0x45: case 0x47: case 0x48:
		case 0x49: case 0x4A: case 0x4B: case 0x4C:
		case 0x4D: case 0x4F: case 0x50: case 0x51:
		case 0x52: case 0x53: case 0x54: case 0x55:
		case 0x57: case 0x58: case 0x59: case 0x5A:
		case 0x5B: case 0x5C: case 0x5D: case 0x5F:
		case 0x60: case 0x61: case 0x62: case 0x63:
		case 0x64: case 0x65: case 0x67: case 0x68:
		case 0x69: case 0x6A: case 0x6B: case 0x6C:
		case 0x6D: case 0x6F: case 0x70: case 0x71:
		case 0x72: case 0x73: case 0x74: case 0x75:
		case 0x77: case 0x78: case 0x79: case 0x7A:
		case 0x7B: case 0x7C: case 0x7D: case 0x7F:
			BIT_r();
			break;

		case 0x46: case 0x4E: case 0x56: case 0x5E:
		case 0x66: case 0x6E: case 0x76: case 0x7E:
			BIT_HLa();
			break;

		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x87: case 0x88:
		case 0x89: case 0x8A: case 0x8B: case 0x8C:
		case 0x8D: case 0x8F: case 0x90: case 0x91:
		case 0x92: case 0x93: case 0x94: case 0x95:
		case 0x97: case 0x98: case 0x99: case 0x9A:
		case 0x9B: case 0x9C: case 0x9D: case 0x9F:
		case 0xA0: case 0xA1: case 0xA2: case 0xA3:
		case 0xA4: case 0xA5: case 0xA7: case 0xA8:
		case 0xA9: case 0xAA: case 0xAB: case 0xAC:
		case 0xAD: case 0xAF: case 0xB0: case 0xB1:
		case 0xB2: case 0xB3: case 0xB4: case 0xB5:
		case 0xB7: case 0xB8: case 0xB9: case 0xBA:
		case 0xBB: case 0xBC: case 0xBD: case 0xBF:
			RES_r();
			break;

		case 0x86: case 0x8E: case 0x96: case 0x9E:
		case 0xA6: case 0xAE: case 0xB6: case 0xBE: 
			RES_HLa();
			break;


		case 0xC6: case 0xCE: case 0xD6: case 0xDE:
		case 0xE6: case 0xEE: case 0xF6: case 0xFE:
			SET_HLa();
			break;
		
		case 0xC0: case 0xC1: case 0xC2: case 0xC3:
		case 0xC4: case 0xC5: case 0xC7: case 0xC8:
		case 0xC9: case 0xCA: case 0xCB: case 0xCC:
		case 0xCD: case 0xCF: case 0xD0: case 0xD1:
		case 0xD2: case 0xD3: case 0xD4: case 0xD5:
		case 0xD7: case 0xD8: case 0xD9: case 0xDA:
		case 0xDB: case 0xDC: case 0xDD: case 0xDF:
		case 0xE0: case 0xE1: case 0xE2: case 0xE3:
		case 0xE4: case 0xE5: case 0xE7: case 0xE8:
		case 0xE9: case 0xEA: case 0xEB: case 0xEC:
		case 0xED: case 0xEF: case 0xF0: case 0xF1:
		case 0xF2: case 0xF3: case 0xF4: case 0xF5:
		case 0xF7: case 0xF8: case 0xF9: case 0xFA:
		case 0xFB: case 0xFC: case 0xFD: case 0xFF:
			SET_r();
			break;
		
		default:
			UNK_OP(gb, opcode, GB_TRUE);
			break;
	}

	gb->cpu.cycles += CYCLE_TABLE_CB[opcode];
}

GB_U16 GB_cpu_run(struct GB_Data* gb, GB_U16 cycles) {
	GB_UNUSED(cycles);

	// reset cycles counter
	gb->cpu.cycles = 0;

	// check and handle interrupts
	GB_interrupt_handler(gb);

	// if halted, return early
	// todo: need accurate cycles elapsed.
	if (UNLIKELY(gb->cpu.halt)) {
		return 8;
	}

	GB_execute(gb);

	assert(gb->cpu.cycles != 0);

	return gb->cpu.cycles;
}
