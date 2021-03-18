#include "gb.h"
#include "internal.h"
#include "tables/cycle_table.h"

#ifdef GB_DEBUG
#include "tables/cycle_table_debug.h"
#include <stdio.h>
#include <assert.h>

#define UNK_OP() \
printf("\nMISSING OP: 0x%02X\n", opcode); \
LOG_INST() \
assert(0);

#define UNK_OP_CB() \
printf("\nMISSING OP: 0xCB%02X\n", opcode); \
LOG_INST_CB() \
assert(0); \

#define LOG_INST() \
printf("name: %s flags: %s len: %u\n", \
	OP_TABLE[opcode].name, OP_TABLE[opcode].flags, OP_TABLE[opcode].len);
#define LOG_INST_CB() \
printf("name: %s flags: %s len: %u\n", \
	OPCB_TABLE[opcode].name, OPCB_TABLE[opcode].flags, OPCB_TABLE[opcode].len);

#elif GB_SPEED
#define UNK_OP() __builtin_unreachable()
#define UNK_OP_CB() __builtin_unreachable()
#else
#define UNK_OP() __builtin_trap()
#define UNK_OP_CB() __builtin_trap()
#endif

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

#define SET_FLAG_C(n) REG_F ^= (-(!!(n)) ^ REG_F) & 0x10
#define SET_FLAG_H(n) REG_F ^= (-(!!(n)) ^ REG_F) & 0x20
#define SET_FLAG_N(n) REG_F ^= (-(!!(n)) ^ REG_F) & 0x40
#define SET_FLAG_Z(n) REG_F ^= (-(!!(n)) ^ REG_F) & 0x80

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
	__builtin_unreachable();
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
	__builtin_unreachable();
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
	__builtin_unreachable();
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
static GB_U8 GB_execute(struct GB_Data* gb);
static GB_U8 GB_execute_cb(struct GB_Data* gb);

static void GB_PUSH(struct GB_Data* gb, GB_U16 value) {
	write8(--REG_SP, (value >> 8) & 0xFF);
	write8(--REG_SP, value & 0xFF);
}

static GB_U16 GB_POP(struct GB_Data* gb) {
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
#define HALT() do { gb->cpu.halt = GB_TRUE; } while(0)

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

	if (live_interrupts & 0x01) { // vblank
		++gb->vblank_int;
        RST(64);
		IO_IF &= ~(0x01);
    } else if (live_interrupts & 0x02) { // stat
        RST(72);
		IO_IF &= ~(0x02);
    } else if (live_interrupts & 0x04) { // timer
        RST(80);
		IO_IF &= ~(0x04);
    } else if (live_interrupts & 0x08) { // serial
        RST(88);
		IO_IF &= ~(0x08);
    } else if (live_interrupts & 0x10) { // joypad
        RST(96);
		IO_IF &= ~(0x10);
    }

	gb->cpu.cycles += 20;
}

static GB_U8 GB_execute(struct GB_Data* gb) {
	const GB_U8 opcode = read8(REG_PC++);

	switch (opcode) {
	case 0x00: break; // nop
	case 0x01: LD_BC_u16(); break;
	case 0x02: LD_BCa_A(); break;
	case 0x03: INC_BC(); break;
	case 0x04: INC_r(); break;
	case 0x05: DEC_r(); break;
	case 0x06: LD_r_u8(); break;
	case 0x07: RLCA(); break;
	case 0x08: LD_u16_SP(); break;
	case 0x0A: LD_A_BCa(); break;
	case 0x09: ADD_HL_BC(); break;
	case 0x0B: DEC_BC(); break;
	case 0x0C: INC_r(); break;
	case 0x0D: DEC_r(); break;
	case 0x0E: LD_r_u8(); break;
	case 0x0F: RRCA(); break;
	case 0x11: LD_DE_u16(); break;
	case 0x12: LD_DEa_A(); break;
	case 0x13: INC_DE(); break;
	case 0x14: INC_r(); break;
	case 0x15: DEC_r(); break;
	case 0x16: LD_r_u8(); break;
	case 0x17: RLA(); break;
	case 0x18: JR(); break;
	case 0x19: ADD_HL_DE(); break;
	case 0x1A: LD_A_DEa(); break;
	case 0x1B: DEC_DE(); break;
	case 0x1C: INC_r(); break;
	case 0x1D: DEC_r(); break;
	case 0x1E: LD_r_u8(); break;
	case 0x1F: RRA(); break;
	case 0x20: JR_NZ(); break;
	case 0x21: LD_HL_u16(); break;
	case 0x22: LD_HLi_A(); break;
	case 0x23: INC_HL(); break;
	case 0x24: INC_r(); break;
	case 0x25: DEC_r(); break;
	case 0x26: LD_r_u8(); break;
	case 0x27: DAA(); break;
	case 0x28: JR_Z(); break;
	case 0x29: ADD_HL_HL(); break;
	case 0x2A: LD_A_HLi(); break;
	case 0x2B: DEC_HL(); break;
	case 0x2C: INC_r(); break;
	case 0x2D: DEC_r(); break;
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
	case 0x3C: INC_r(); break;
	case 0x3D: DEC_r(); break;
	case 0x3E: LD_r_u8(); break;
	case 0x3F: CCF(); break;
	case 0x40: break; // nop b,b
	case 0x41: LD_r_r(); break;
	case 0x42: LD_r_r(); break;
	case 0x43: LD_r_r(); break;
	case 0x44: LD_r_r(); break;
	case 0x45: LD_r_r(); break;
	case 0x46: LD_r_HLa(); break;
	case 0x47: LD_r_r(); break;
	case 0x48: LD_r_r(); break;
	case 0x49: break; // nop c,c
	case 0x4A: LD_r_r(); break;
	case 0x4B: LD_r_r(); break;
	case 0x4C: LD_r_r(); break;
	case 0x4D: LD_r_r(); break;
	case 0x4E: LD_r_HLa(); break;
	case 0x4F: LD_r_r(); break;
	case 0x50: LD_r_r(); break;
	case 0x51: LD_r_r(); break;
	case 0x52: break; // nop d,d
	case 0x53: LD_r_r(); break;
	case 0x54: LD_r_r(); break;
	case 0x55: LD_r_r(); break;
	case 0x56: LD_r_HLa(); break;
	case 0x57: LD_r_r(); break;
	case 0x58: LD_r_r(); break;
	case 0x59: LD_r_r(); break;
	case 0x5A: LD_r_r(); break;
	case 0x5B: break; // nop e,e
	case 0x5C: LD_r_r(); break;
	case 0x5D: LD_r_r(); break;
	case 0x5E: LD_r_HLa(); break;
	case 0x5F: LD_r_r(); break;
	case 0x60: LD_r_r(); break;
	case 0x61: LD_r_r(); break;
	case 0x62: LD_r_r(); break;
	case 0x63: LD_r_r(); break;
	case 0x64: break; // nop h,h
	case 0x65: LD_r_r(); break;
	case 0x66: LD_r_HLa(); break;
	case 0x67: LD_r_r(); break;
	case 0x68: LD_r_r(); break;
	case 0x69: LD_r_r(); break;
	case 0x6A: LD_r_r(); break;
	case 0x6B: LD_r_r(); break;
	case 0x6C: LD_r_r(); break;
	case 0x6D: break; // nop l,l
	case 0x6E: LD_r_HLa(); break;
	case 0x6F: LD_r_r(); break;
	case 0x70: LD_HLa_r(); break;
	case 0x71: LD_HLa_r(); break;
	case 0x72: LD_HLa_r(); break;
	case 0x73: LD_HLa_r(); break;
	case 0x74: LD_HLa_r(); break;
	case 0x75: LD_HLa_r(); break;
	case 0x76: HALT(); break;
	case 0x77: LD_HLa_r(); break;
	case 0x78: LD_r_r(); break;
	case 0x79: LD_r_r(); break;
	case 0x7A: LD_r_r(); break;
	case 0x7B: LD_r_r(); break;
	case 0x7C: LD_r_r(); break;
	case 0x7D: LD_r_r(); break;
	case 0x7E: LD_A_HLa(); break;
	case 0x7F: break; // nop a,a
	case 0x80: ADD_r(); break;
	case 0x81: ADD_r(); break;
	case 0x82: ADD_r(); break;
	case 0x83: ADD_r(); break;
	case 0x84: ADD_r(); break;
	case 0x85: ADD_r(); break;
	case 0x86: ADD_HLa(); break;
	case 0x87: ADD_r(); break;
	case 0x88: ADC_r(); break;
	case 0x89: ADC_r(); break;
	case 0x8A: ADC_r(); break;
	case 0x8B: ADC_r(); break;
	case 0x8C: ADC_r(); break;
	case 0x8D: ADC_r(); break;
	case 0x8E: ADC_HLa(); break;
	case 0x8F: ADC_r(); break;
	case 0x90: SUB_r(); break;
	case 0x91: SUB_r(); break;
	case 0x92: SUB_r(); break;
	case 0x93: SUB_r(); break;
	case 0x94: SUB_r(); break;
	case 0x95: SUB_r(); break;
	case 0x96: SUB_HLa(); break;
	case 0x97: SUB_r(); break;
	case 0x98: SBC_r(); break;
	case 0x99: SBC_r(); break;
	case 0x9A: SBC_r(); break;
	case 0x9B: SBC_r(); break;
	case 0x9C: SBC_r(); break;
	case 0x9D: SBC_r(); break;
	case 0x9E: SBC_HLa(); break;
	case 0x9F: SBC_r(); break;
	case 0xA0: AND_r(); break;
	case 0xA1: AND_r(); break;
	case 0xA2: AND_r(); break;
	case 0xA3: AND_r(); break;
	case 0xA4: AND_r(); break;
	case 0xA5: AND_r(); break;
	case 0xA6: AND_HLa(); break;
	case 0xA7: AND_r(); break;
	case 0xA8: XOR_r(); break;
	case 0xA9: XOR_r(); break;
	case 0xAA: XOR_r(); break;
	case 0xAB: XOR_r(); break;
	case 0xAC: XOR_r(); break;
	case 0xAD: XOR_r(); break;
	case 0xAE: XOR_HLa(); break;
	case 0xAF: XOR_r(); break;
	case 0xB0: OR_r(); break;
	case 0xB1: OR_r(); break;
	case 0xB2: OR_r(); break;
	case 0xB3: OR_r(); break;
	case 0xB4: OR_r(); break;
	case 0xB5: OR_r(); break;
	case 0xB6: OR_HLa(); break;
	case 0xB7: OR_r(); break;
	case 0xB8: CP_r(); break;
	case 0xB9: CP_r(); break;
	case 0xBA: CP_r(); break;
	case 0xBB: CP_r(); break;
	case 0xBC: CP_r(); break;
	case 0xBD: CP_r(); break;
	case 0xBE: CP_HLa(); break;
	case 0xBF: CP_r(); break;
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
	case 0xCB: return GB_execute_cb(gb);
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
	default: UNK_OP(); break;
	}

	return CYCLE_TABLE[opcode] + gb->cpu.cycles;
}

static GB_U8 GB_execute_cb(struct GB_Data* gb) {
	const GB_U8 opcode = read8(REG_PC++);

	switch (opcode) {
	case 0x00: RLC_r(); break;
	case 0x01: RLC_r(); break;
	case 0x02: RLC_r(); break;
	case 0x03: RLC_r(); break;
	case 0x04: RLC_r(); break;
	case 0x05: RLC_r(); break;
	case 0x06: RLC_HLa(); break;
	case 0x07: RLC_r(); break;
	case 0x08: RRC_r(); break;
	case 0x09: RRC_r(); break;
	case 0x0A: RRC_r(); break;
	case 0x0B: RRC_r(); break;
	case 0x0C: RRC_r(); break;
	case 0x0D: RRC_r(); break;
	case 0x0E: RRC_HLa(); break;
	case 0x0F: RRC_r(); break;
	case 0x10: RL_r(); break;
	case 0x11: RL_r(); break;
	case 0x12: RL_r(); break;
	case 0x13: RL_r(); break;
	case 0x14: RL_r(); break;
	case 0x15: RL_r(); break;
	case 0x16: RL_HLa(); break;
	case 0x17: RL_r(); break;
	case 0x18: RR_r(); break;
	case 0x19: RR_r(); break;
	case 0x1A: RR_r(); break;
	case 0x1B: RR_r(); break;
	case 0x1C: RR_r(); break;
	case 0x1D: RR_r(); break;
	case 0x1E: RR_HLa(); break;
	case 0x1F: RR_r(); break;
	case 0x20: SLA_r(); break;
	case 0x21: SLA_r(); break;
	case 0x22: SLA_r(); break;
	case 0x23: SLA_r(); break;
	case 0x24: SLA_r(); break;
	case 0x25: SLA_r(); break;
	case 0x26: SLA_HLa(); break;
	case 0x27: SLA_r(); break;
	case 0x28: SRA_r(); break;
	case 0x29: SRA_r(); break;
	case 0x2A: SRA_r(); break;
	case 0x2B: SRA_r(); break;
	case 0x2C: SRA_r(); break;
	case 0x2D: SRA_r(); break;
	case 0x2E: SRA_HLa(); break;
	case 0x2F: SRA_r(); break;
	case 0x30: SWAP_r(); break;
	case 0x31: SWAP_r(); break;
	case 0x32: SWAP_r(); break;
	case 0x33: SWAP_r(); break;
	case 0x34: SWAP_r(); break;
	case 0x35: SWAP_r(); break;
	case 0x36: SWAP_HLa(); break;
	case 0x37: SWAP_r(); break;
	case 0x38: SRL_r(); break;
	case 0x39: SRL_r(); break;
	case 0x3A: SRL_r(); break;
	case 0x3B: SRL_r(); break;
	case 0x3C: SRL_r(); break;
	case 0x3D: SRL_r(); break;
	case 0x3E: SRL_HLa(); break;
	case 0x3F: SRL_r(); break;
	case 0x40: BIT_r(); break;
	case 0x41: BIT_r(); break;
	case 0x42: BIT_r(); break;
	case 0x43: BIT_r(); break;
	case 0x44: BIT_r(); break;
	case 0x45: BIT_r(); break;
	case 0x46: BIT_HLa(); break;
	case 0x47: BIT_r(); break;
	case 0x48: BIT_r(); break;
	case 0x49: BIT_r(); break;
	case 0x4A: BIT_r(); break;
	case 0x4B: BIT_r(); break;
	case 0x4C: BIT_r(); break;
	case 0x4D: BIT_r(); break;
	case 0x4E: BIT_HLa(); break;
	case 0x4F: BIT_r(); break;
	case 0x50: BIT_r(); break;
	case 0x51: BIT_r(); break;
	case 0x52: BIT_r(); break;
	case 0x53: BIT_r(); break;
	case 0x54: BIT_r(); break;
	case 0x55: BIT_r(); break;
	case 0x56: BIT_HLa(); break;
	case 0x57: BIT_r(); break;
	case 0x58: BIT_r(); break;
	case 0x59: BIT_r(); break;
	case 0x5A: BIT_r(); break;
	case 0x5B: BIT_r(); break;
	case 0x5C: BIT_r(); break;
	case 0x5D: BIT_r(); break;
	case 0x5E: BIT_HLa(); break;
	case 0x5F: BIT_r(); break;
	case 0x60: BIT_r(); break;
	case 0x61: BIT_r(); break;
	case 0x62: BIT_r(); break;
	case 0x63: BIT_r(); break;
	case 0x64: BIT_r(); break;
	case 0x65: BIT_r(); break;
	case 0x66: BIT_HLa(); break;
	case 0x67: BIT_r(); break;
	case 0x68: BIT_r(); break;
	case 0x69: BIT_r(); break;
	case 0x6A: BIT_r(); break;
	case 0x6B: BIT_r(); break;
	case 0x6C: BIT_r(); break;
	case 0x6D: BIT_r(); break;
	case 0x6E: BIT_HLa(); break;
	case 0x6F: BIT_r(); break;
	case 0x70: BIT_r(); break;
	case 0x71: BIT_r(); break;
	case 0x72: BIT_r(); break;
	case 0x73: BIT_r(); break;
	case 0x74: BIT_r(); break;
	case 0x75: BIT_r(); break;
	case 0x76: BIT_HLa(); break;
	case 0x77: BIT_r(); break;
	case 0x78: BIT_r(); break;
	case 0x79: BIT_r(); break;
	case 0x7A: BIT_r(); break;
	case 0x7B: BIT_r(); break;
	case 0x7C: BIT_r(); break;
	case 0x7D: BIT_r(); break;
	case 0x7E: BIT_HLa(); break;
	case 0x7F: BIT_r(); break;
	case 0x80: RES_r(); break;
	case 0x81: RES_r(); break;
	case 0x82: RES_r(); break;
	case 0x83: RES_r(); break;
	case 0x84: RES_r(); break;
	case 0x85: RES_r(); break;
	case 0x86: RES_HLa(); break;
	case 0x87: RES_r(); break;
	case 0x88: RES_r(); break;
	case 0x89: RES_r(); break;
	case 0x8A: RES_r(); break;
	case 0x8B: RES_r(); break;
	case 0x8C: RES_r(); break;
	case 0x8D: RES_r(); break;
	case 0x8E: RES_HLa(); break;
	case 0x8F: RES_r(); break;
	case 0x90: RES_r(); break;
	case 0x91: RES_r(); break;
	case 0x92: RES_r(); break;
	case 0x93: RES_r(); break;
	case 0x94: RES_r(); break;
	case 0x95: RES_r(); break;
	case 0x96: RES_HLa(); break;
	case 0x97: RES_r(); break;
	case 0x98: RES_r(); break;
	case 0x99: RES_r(); break;
	case 0x9A: RES_r(); break;
	case 0x9B: RES_r(); break;
	case 0x9C: RES_r(); break;
	case 0x9D: RES_r(); break;
	case 0x9E: RES_HLa(); break;
	case 0x9F: RES_r(); break;
	case 0xA0: RES_r(); break;
	case 0xA1: RES_r(); break;
	case 0xA2: RES_r(); break;
	case 0xA3: RES_r(); break;
	case 0xA4: RES_r(); break;
	case 0xA5: RES_r(); break;
	case 0xA6: RES_HLa(); break;
	case 0xA7: RES_r(); break;
	case 0xA8: RES_r(); break;
	case 0xA9: RES_r(); break;
	case 0xAA: RES_r(); break;
	case 0xAB: RES_r(); break;
	case 0xAC: RES_r(); break;
	case 0xAD: RES_r(); break;
	case 0xAE: RES_HLa(); break;
	case 0xAF: RES_r(); break;
	case 0xB0: RES_r(); break;
	case 0xB1: RES_r(); break;
	case 0xB2: RES_r(); break;
	case 0xB3: RES_r(); break;
	case 0xB4: RES_r(); break;
	case 0xB5: RES_r(); break;
	case 0xB6: RES_HLa(); break;
	case 0xB7: RES_r(); break;
	case 0xB8: RES_r(); break;
	case 0xB9: RES_r(); break;
	case 0xBA: RES_r(); break;
	case 0xBB: RES_r(); break;
	case 0xBC: RES_r(); break;
	case 0xBD: RES_r(); break;
	case 0xBE: RES_HLa(); break;
	case 0xBF: RES_r(); break;
	case 0xC0: SET_r(); break;
	case 0xC1: SET_r(); break;
	case 0xC2: SET_r(); break;
	case 0xC3: SET_r(); break;
	case 0xC4: SET_r(); break;
	case 0xC5: SET_r(); break;
	case 0xC6: SET_HLa(); break;
	case 0xC7: SET_r(); break;
	case 0xC8: SET_r(); break;
	case 0xC9: SET_r(); break;
	case 0xCA: SET_r(); break;
	case 0xCB: SET_r(); break;
	case 0xCC: SET_r(); break;
	case 0xCD: SET_r(); break;
	case 0xCE: SET_HLa(); break;
	case 0xCF: SET_r(); break;
	case 0xD0: SET_r(); break;
	case 0xD1: SET_r(); break;
	case 0xD2: SET_r(); break;
	case 0xD3: SET_r(); break;
	case 0xD4: SET_r(); break;
	case 0xD5: SET_r(); break;
	case 0xD6: SET_HLa(); break;
	case 0xD7: SET_r(); break;
	case 0xD8: SET_r(); break;
	case 0xD9: SET_r(); break;
	case 0xDA: SET_r(); break;
	case 0xDB: SET_r(); break;
	case 0xDC: SET_r(); break;
	case 0xDD: SET_r(); break;
	case 0xDE: SET_HLa(); break;
	case 0xDF: SET_r(); break;
	case 0xE0: SET_r(); break;
	case 0xE1: SET_r(); break;
	case 0xE2: SET_r(); break;
	case 0xE3: SET_r(); break;
	case 0xE4: SET_r(); break;
	case 0xE5: SET_r(); break;
	case 0xE6: SET_HLa(); break;
	case 0xE7: SET_r(); break;
	case 0xE8: SET_r(); break;
	case 0xE9: SET_r(); break;
	case 0xEA: SET_r(); break;
	case 0xEB: SET_r(); break;
	case 0xEC: SET_r(); break;
	case 0xED: SET_r(); break;
	case 0xEE: SET_HLa(); break;
	case 0xEF: SET_r(); break;
	case 0xF0: SET_r(); break;
	case 0xF1: SET_r(); break;
	case 0xF2: SET_r(); break;
	case 0xF3: SET_r(); break;
	case 0xF4: SET_r(); break;
	case 0xF5: SET_r(); break;
	case 0xF6: SET_HLa(); break;
	case 0xF7: SET_r(); break;
	case 0xF8: SET_r(); break;
	case 0xF9: SET_r(); break;
	case 0xFA: SET_r(); break;
	case 0xFB: SET_r(); break;
	case 0xFC: SET_r(); break;
	case 0xFD: SET_r(); break;
	case 0xFE: SET_HLa(); break;
	case 0xFF: SET_r(); break;
	default: UNK_OP_CB(); break;
	}

	return CYCLE_TABLE_CB[opcode] + gb->cpu.cycles;
}

GB_U16 GB_cpu_run(struct GB_Data* gb, GB_U16 cycles) {
	UNUSED(cycles);

	// reset cycles counter
	gb->cpu.cycles = 0;

	GB_interrupt_handler(gb);

	// if halted, return early
	// todo: need accurate cycles elapsed.
	if (UNLIKELY(gb->cpu.halt)) {
		return 8;
	}

	GB_execute(gb);

	return gb->cpu.cycles;
}
