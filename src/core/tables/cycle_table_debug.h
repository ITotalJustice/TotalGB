/*parse_ops - 0.0.1: auto generate C/C++ array from table data*/

#ifndef CYCLE_TABLE_DEBUG_H
#define CYCLE_TABLE_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char *name;
	const char *group;
	const char *flags;
	const unsigned char len;
	const unsigned char cycles;
	const unsigned char cycles2;
} opcode_t;

static const opcode_t CYCLE_TABLE_DEBUG[0x100] = {
	{"NOP","control/misc","----",1,4,4}, {"LD BC,u16","x16/lsm","----",3,12,12}, {"LD (BC),A","x8/lsm","----",1,8,8}, {"INC BC","x16/alu","----",1,8,8}, {"INC B","x8/alu","Z0H-",1,4,4}, {"DEC B","x8/alu","Z1H-",1,4,4}, {"LD B,u8","x8/lsm","----",2,8,8}, {"RLCA","x8/rsb","000C",1,4,4}, {"LD (u16),SP","x16/lsm","----",3,20,20}, {"ADD HL,BC","x16/alu","-0HC",1,8,8}, {"LD A,(BC)","x8/lsm","----",1,8,8}, {"DEC BC","x16/alu","----",1,8,8}, {"INC C","x8/alu","Z0H-",1,4,4}, {"DEC C","x8/alu","Z1H-",1,4,4}, {"LD C,u8","x8/lsm","----",2,8,8}, {"RRCA","x8/rsb","000C",1,4,4}, {"STOP","control/misc","----",2,4,4}, 
	{"LD DE,u16","x16/lsm","----",3,12,12}, {"LD (DE),A","x8/lsm","----",1,8,8}, {"INC DE","x16/alu","----",1,8,8}, {"INC D","x8/alu","Z0H-",1,4,4}, {"DEC D","x8/alu","Z1H-",1,4,4}, {"LD D,u8","x8/lsm","----",2,8,8}, {"RLA","x8/rsb","000C",1,4,4}, {"JR i8","control/br","----",2,12,12}, {"ADD HL,DE","x16/alu","-0HC",1,8,8}, {"LD A,(DE)","x8/lsm","----",1,8,8}, {"DEC DE","x16/alu","----",1,8,8}, {"INC E","x8/alu","Z0H-",1,4,4}, {"DEC E","x8/alu","Z1H-",1,4,4}, {"LD E,u8","x8/lsm","----",2,8,8}, {"RRA","x8/rsb","000C",1,4,4}, {"JR NZ,i8","control/br","----",2,8,12}, 
	{"LD HL,u16","x16/lsm","----",3,12,12}, {"LD (HL+),A","x8/lsm","----",1,8,8}, {"INC HL","x16/alu","----",1,8,8}, {"INC H","x8/alu","Z0H-",1,4,4}, {"DEC H","x8/alu","Z1H-",1,4,4}, {"LD H,u8","x8/lsm","----",2,8,8}, {"DAA","x8/alu","Z-0C",1,4,4}, {"JR Z,i8","control/br","----",2,8,12}, {"ADD HL,HL","x16/alu","-0HC",1,8,8}, {"LD A,(HL+)","x8/lsm","----",1,8,8}, {"DEC HL","x16/alu","----",1,8,8}, {"INC L","x8/alu","Z0H-",1,4,4}, {"DEC L","x8/alu","Z1H-",1,4,4}, {"LD L,u8","x8/lsm","----",2,8,8}, {"CPL","x8/alu","-11-",1,4,4}, {"JR NC,i8","control/br","----",2,8,12}, 
	{"LD SP,u16","x16/lsm","----",3,12,12}, {"LD (HL-),A","x8/lsm","----",1,8,8}, {"INC SP","x16/alu","----",1,8,8}, {"INC (HL)","x8/alu","Z0H-",1,12,12}, {"DEC (HL)","x8/alu","Z1H-",1,12,12}, {"LD (HL),u8","x8/lsm","----",2,12,12}, {"SCF","x8/alu","-001",1,4,4}, {"JR C,i8","control/br","----",2,8,12}, {"ADD HL,SP","x16/alu","-0HC",1,8,8}, {"LD A,(HL-)","x8/lsm","----",1,8,8}, {"DEC SP","x16/alu","----",1,8,8}, {"INC A","x8/alu","Z0H-",1,4,4}, {"DEC A","x8/alu","Z1H-",1,4,4}, {"LD A,u8","x8/lsm","----",2,8,8}, {"CCF","x8/alu","-00C",1,4,4}, {"LD B,B","x8/lsm","----",1,4,4}, 
	{"LD B,C","x8/lsm","----",1,4,4}, {"LD B,D","x8/lsm","----",1,4,4}, {"LD B,E","x8/lsm","----",1,4,4}, {"LD B,H","x8/lsm","----",1,4,4}, {"LD B,L","x8/lsm","----",1,4,4}, {"LD B,(HL)","x8/lsm","----",1,8,8}, {"LD B,A","x8/lsm","----",1,4,4}, {"LD C,B","x8/lsm","----",1,4,4}, {"LD C,C","x8/lsm","----",1,4,4}, {"LD C,D","x8/lsm","----",1,4,4}, {"LD C,E","x8/lsm","----",1,4,4}, {"LD C,H","x8/lsm","----",1,4,4}, {"LD C,L","x8/lsm","----",1,4,4}, {"LD C,(HL)","x8/lsm","----",1,8,8}, {"LD C,A","x8/lsm","----",1,4,4}, {"LD D,B","x8/lsm","----",1,4,4}, 
	{"LD D,C","x8/lsm","----",1,4,4}, {"LD D,D","x8/lsm","----",1,4,4}, {"LD D,E","x8/lsm","----",1,4,4}, {"LD D,H","x8/lsm","----",1,4,4}, {"LD D,L","x8/lsm","----",1,4,4}, {"LD D,(HL)","x8/lsm","----",1,8,8}, {"LD D,A","x8/lsm","----",1,4,4}, {"LD E,B","x8/lsm","----",1,4,4}, {"LD E,C","x8/lsm","----",1,4,4}, {"LD E,D","x8/lsm","----",1,4,4}, {"LD E,E","x8/lsm","----",1,4,4}, {"LD E,H","x8/lsm","----",1,4,4}, {"LD E,L","x8/lsm","----",1,4,4}, {"LD E,(HL)","x8/lsm","----",1,8,8}, {"LD E,A","x8/lsm","----",1,4,4}, {"LD H,B","x8/lsm","----",1,4,4}, 
	{"LD H,C","x8/lsm","----",1,4,4}, {"LD H,D","x8/lsm","----",1,4,4}, {"LD H,E","x8/lsm","----",1,4,4}, {"LD H,H","x8/lsm","----",1,4,4}, {"LD H,L","x8/lsm","----",1,4,4}, {"LD H,(HL)","x8/lsm","----",1,8,8}, {"LD H,A","x8/lsm","----",1,4,4}, {"LD L,B","x8/lsm","----",1,4,4}, {"LD L,C","x8/lsm","----",1,4,4}, {"LD L,D","x8/lsm","----",1,4,4}, {"LD L,E","x8/lsm","----",1,4,4}, {"LD L,H","x8/lsm","----",1,4,4}, {"LD L,L","x8/lsm","----",1,4,4}, {"LD L,(HL)","x8/lsm","----",1,8,8}, {"LD L,A","x8/lsm","----",1,4,4}, {"LD (HL),B","x8/lsm","----",1,8,8}, 
	{"LD (HL),C","x8/lsm","----",1,8,8}, {"LD (HL),D","x8/lsm","----",1,8,8}, {"LD (HL),E","x8/lsm","----",1,8,8}, {"LD (HL),H","x8/lsm","----",1,8,8}, {"LD (HL),L","x8/lsm","----",1,8,8}, {"HALT","control/misc","----",1,4,4}, {"LD (HL),A","x8/lsm","----",1,8,8}, {"LD A,B","x8/lsm","----",1,4,4}, {"LD A,C","x8/lsm","----",1,4,4}, {"LD A,D","x8/lsm","----",1,4,4}, {"LD A,E","x8/lsm","----",1,4,4}, {"LD A,H","x8/lsm","----",1,4,4}, {"LD A,L","x8/lsm","----",1,4,4}, {"LD A,(HL)","x8/lsm","----",1,8,8}, {"LD A,A","x8/lsm","----",1,4,4}, {"ADD A,B","x8/alu","Z0HC",1,4,4}, 
	{"ADD A,C","x8/alu","Z0HC",1,4,4}, {"ADD A,D","x8/alu","Z0HC",1,4,4}, {"ADD A,E","x8/alu","Z0HC",1,4,4}, {"ADD A,H","x8/alu","Z0HC",1,4,4}, {"ADD A,L","x8/alu","Z0HC",1,4,4}, {"ADD A,(HL)","x8/alu","Z0HC",1,8,8}, {"ADD A,A","x8/alu","Z0HC",1,4,4}, {"ADC A,B","x8/alu","Z0HC",1,4,4}, {"ADC A,C","x8/alu","Z0HC",1,4,4}, {"ADC A,D","x8/alu","Z0HC",1,4,4}, {"ADC A,E","x8/alu","Z0HC",1,4,4}, {"ADC A,H","x8/alu","Z0HC",1,4,4}, {"ADC A,L","x8/alu","Z0HC",1,4,4}, {"ADC A,(HL)","x8/alu","Z0HC",1,8,8}, {"ADC A,A","x8/alu","Z0HC",1,4,4}, {"SUB A,B","x8/alu","Z1HC",1,4,4}, 
	{"SUB A,C","x8/alu","Z1HC",1,4,4}, {"SUB A,D","x8/alu","Z1HC",1,4,4}, {"SUB A,E","x8/alu","Z1HC",1,4,4}, {"SUB A,H","x8/alu","Z1HC",1,4,4}, {"SUB A,L","x8/alu","Z1HC",1,4,4}, {"SUB A,(HL)","x8/alu","Z1HC",1,8,8}, {"SUB A,A","x8/alu","Z1HC",1,4,4}, {"SBC A,B","x8/alu","Z1HC",1,4,4}, {"SBC A,C","x8/alu","Z1HC",1,4,4}, {"SBC A,D","x8/alu","Z1HC",1,4,4}, {"SBC A,E","x8/alu","Z1HC",1,4,4}, {"SBC A,H","x8/alu","Z1HC",1,4,4}, {"SBC A,L","x8/alu","Z1HC",1,4,4}, {"SBC A,(HL)","x8/alu","Z1HC",1,8,8}, {"SBC A,A","x8/alu","Z1HC",1,4,4}, {"AND A,B","x8/alu","Z010",1,4,4}, 
	{"AND A,C","x8/alu","Z010",1,4,4}, {"AND A,D","x8/alu","Z010",1,4,4}, {"AND A,E","x8/alu","Z010",1,4,4}, {"AND A,H","x8/alu","Z010",1,4,4}, {"AND A,L","x8/alu","Z010",1,4,4}, {"AND A,(HL)","x8/alu","Z010",1,8,8}, {"AND A,A","x8/alu","Z010",1,4,4}, {"XOR A,B","x8/alu","Z000",1,4,4}, {"XOR A,C","x8/alu","Z000",1,4,4}, {"XOR A,D","x8/alu","Z000",1,4,4}, {"XOR A,E","x8/alu","Z000",1,4,4}, {"XOR A,H","x8/alu","Z000",1,4,4}, {"XOR A,L","x8/alu","Z000",1,4,4}, {"XOR A,(HL)","x8/alu","Z000",1,8,8}, {"XOR A,A","x8/alu","Z000",1,4,4}, {"OR A,B","x8/alu","Z000",1,4,4}, 
	{"OR A,C","x8/alu","Z000",1,4,4}, {"OR A,D","x8/alu","Z000",1,4,4}, {"OR A,E","x8/alu","Z000",1,4,4}, {"OR A,H","x8/alu","Z000",1,4,4}, {"OR A,L","x8/alu","Z000",1,4,4}, {"OR A,(HL)","x8/alu","Z000",1,8,8}, {"OR A,A","x8/alu","Z000",1,4,4}, {"CP A,B","x8/alu","Z1HC",1,4,4}, {"CP A,C","x8/alu","Z1HC",1,4,4}, {"CP A,D","x8/alu","Z1HC",1,4,4}, {"CP A,E","x8/alu","Z1HC",1,4,4}, {"CP A,H","x8/alu","Z1HC",1,4,4}, {"CP A,L","x8/alu","Z1HC",1,4,4}, {"CP A,(HL)","x8/alu","Z1HC",1,8,8}, {"CP A,A","x8/alu","Z1HC",1,4,4}, {"RET NZ","control/br","----",1,8,20}, 
	{"POP BC","x16/lsm","----",1,12,12}, {"JP NZ,u16","control/br","----",3,12,16}, {"JP u16","control/br","----",3,16,16}, {"CALL NZ,u16","control/br","----",3,12,24}, {"PUSH BC","x16/lsm","----",1,16,16}, {"ADD A,u8","x8/alu","Z0HC",2,8,8}, {"RST 00h","control/br","----",1,16,16}, {"RET Z","control/br","----",1,8,20}, {"RET","control/br","----",1,16,16}, {"JP Z,u16","control/br","----",3,12,16}, {"PREFIX CB","control/misc","----",1,4,4}, {"CALL Z,u16","control/br","----",3,12,24}, {"CALL u16","control/br","----",3,24,24}, {"ADC A,u8","x8/alu","Z0HC",2,8,8}, {"RST 08h","control/br","----",1,16,16}, {"RET NC","control/br","----",1,8,20}, 
	{"POP DE","x16/lsm","----",1,12,12}, {"JP NC,u16","control/br","----",3,12,16}, {"UNUSED","unused","----",1,0,0}, {"CALL NC,u16","control/br","----",3,12,24}, {"PUSH DE","x16/lsm","----",1,16,16}, {"SUB A,u8","x8/alu","Z1HC",2,8,8}, {"RST 10h","control/br","----",1,16,16}, {"RET C","control/br","----",1,8,20}, {"RETI","control/br","----",1,16,16}, {"JP C,u16","control/br","----",3,12,16}, {"UNUSED","unused","----",1,0,0}, {"CALL C,u16","control/br","----",3,12,24}, {"UNUSED","unused","----",1,0,0}, {"SBC A,u8","x8/alu","Z1HC",2,8,8}, {"RST 18h","control/br","----",1,16,16}, {"LD (FF00+u8),A","x8/lsm","----",2,12,12}, 
	{"POP HL","x16/lsm","----",1,12,12}, {"LD (FF00+C),A","x8/lsm","----",1,8,8}, {"UNUSED","unused","----",1,0,0}, {"UNUSED","unused","----",1,0,0}, {"PUSH HL","x16/lsm","----",1,16,16}, {"AND A,u8","x8/alu","Z010",2,8,8}, {"RST 20h","control/br","----",1,16,16}, {"ADD SP,i8","x16/alu","00HC",2,16,16}, {"JP HL","control/br","----",1,4,4}, {"LD (u16),A","x8/lsm","----",3,16,16}, {"UNUSED","unused","----",1,0,0}, {"UNUSED","unused","----",1,0,0}, {"UNUSED","unused","----",1,0,0}, {"XOR A,u8","x8/alu","Z000",2,8,8}, {"RST 28h","control/br","----",1,16,16}, {"LD A,(FF00+u8)","x8/lsm","----",2,12,12}, 
	{"POP AF","x16/lsm","ZNHC",1,12,12}, {"LD A,(FF00+C)","x8/lsm","----",1,8,8}, {"DI","control/misc","----",1,4,4}, {"UNUSED","unused","----",1,0,0}, {"PUSH AF","x16/lsm","----",1,16,16}, {"OR A,u8","x8/alu","Z000",2,8,8}, {"RST 30h","control/br","----",1,16,16}, {"LD HL,SP+i8","x16/alu","00HC",2,12,12}, {"LD SP,HL","x16/lsm","----",1,8,8}, {"LD A,(u16)","x8/lsm","----",3,16,16}, {"EI","control/misc","----",1,4,4}, {"UNUSED","unused","----",1,0,0}, {"UNUSED","unused","----",1,0,0}, {"CP A,u8","x8/alu","Z1HC",2,8,8}, {"RST 38h","control/br","----",1,16,16}, 
};

static const opcode_t CYCLE_TABLE_DEBUG_CB[0x100] = {
	{"RLC B","x8/rsb","Z00C",2,8,8}, {"RLC C","x8/rsb","Z00C",2,8,8}, {"RLC D","x8/rsb","Z00C",2,8,8}, {"RLC E","x8/rsb","Z00C",2,8,8}, {"RLC H","x8/rsb","Z00C",2,8,8}, {"RLC L","x8/rsb","Z00C",2,8,8}, {"RLC (HL)","x8/rsb","Z00C",2,16,16}, {"RLC A","x8/rsb","Z00C",2,8,8}, {"RRC B","x8/rsb","Z00C",2,8,8}, {"RRC C","x8/rsb","Z00C",2,8,8}, {"RRC D","x8/rsb","Z00C",2,8,8}, {"RRC E","x8/rsb","Z00C",2,8,8}, {"RRC H","x8/rsb","Z00C",2,8,8}, {"RRC L","x8/rsb","Z00C",2,8,8}, {"RRC (HL)","x8/rsb","Z00C",2,16,16}, {"RRC A","x8/rsb","Z00C",2,8,8}, {"RL B","x8/rsb","Z00C",2,8,8}, 
	{"RL C","x8/rsb","Z00C",2,8,8}, {"RL D","x8/rsb","Z00C",2,8,8}, {"RL E","x8/rsb","Z00C",2,8,8}, {"RL H","x8/rsb","Z00C",2,8,8}, {"RL L","x8/rsb","Z00C",2,8,8}, {"RL (HL)","x8/rsb","Z00C",2,16,16}, {"RL A","x8/rsb","Z00C",2,8,8}, {"RR B","x8/rsb","Z00C",2,8,8}, {"RR C","x8/rsb","Z00C",2,8,8}, {"RR D","x8/rsb","Z00C",2,8,8}, {"RR E","x8/rsb","Z00C",2,8,8}, {"RR H","x8/rsb","Z00C",2,8,8}, {"RR L","x8/rsb","Z00C",2,8,8}, {"RR (HL)","x8/rsb","Z00C",2,16,16}, {"RR A","x8/rsb","Z00C",2,8,8}, {"SLA B","x8/rsb","Z00C",2,8,8}, 
	{"SLA C","x8/rsb","Z00C",2,8,8}, {"SLA D","x8/rsb","Z00C",2,8,8}, {"SLA E","x8/rsb","Z00C",2,8,8}, {"SLA H","x8/rsb","Z00C",2,8,8}, {"SLA L","x8/rsb","Z00C",2,8,8}, {"SLA (HL)","x8/rsb","Z00C",2,16,16}, {"SLA A","x8/rsb","Z00C",2,8,8}, {"SRA B","x8/rsb","Z00C",2,8,8}, {"SRA C","x8/rsb","Z00C",2,8,8}, {"SRA D","x8/rsb","Z00C",2,8,8}, {"SRA E","x8/rsb","Z00C",2,8,8}, {"SRA H","x8/rsb","Z00C",2,8,8}, {"SRA L","x8/rsb","Z00C",2,8,8}, {"SRA (HL)","x8/rsb","Z00C",2,16,16}, {"SRA A","x8/rsb","Z00C",2,8,8}, {"SWAP B","x8/rsb","Z000",2,8,8}, 
	{"SWAP C","x8/rsb","Z000",2,8,8}, {"SWAP D","x8/rsb","Z000",2,8,8}, {"SWAP E","x8/rsb","Z000",2,8,8}, {"SWAP H","x8/rsb","Z000",2,8,8}, {"SWAP L","x8/rsb","Z000",2,8,8}, {"SWAP (HL)","x8/rsb","Z000",2,16,16}, {"SWAP A","x8/rsb","Z000",2,8,8}, {"SRL B","x8/rsb","Z00C",2,8,8}, {"SRL C","x8/rsb","Z00C",2,8,8}, {"SRL D","x8/rsb","Z00C",2,8,8}, {"SRL E","x8/rsb","Z00C",2,8,8}, {"SRL H","x8/rsb","Z00C",2,8,8}, {"SRL L","x8/rsb","Z00C",2,8,8}, {"SRL (HL)","x8/rsb","Z00C",2,16,16}, {"SRL A","x8/rsb","Z00C",2,8,8}, {"BIT 0,B","x8/rsb","Z01-",2,8,8}, 
	{"BIT 0,C","x8/rsb","Z01-",2,8,8}, {"BIT 0,D","x8/rsb","Z01-",2,8,8}, {"BIT 0,E","x8/rsb","Z01-",2,8,8}, {"BIT 0,H","x8/rsb","Z01-",2,8,8}, {"BIT 0,L","x8/rsb","Z01-",2,8,8}, {"BIT 0,(HL)","x8/rsb","Z01-",2,12,12}, {"BIT 0,A","x8/rsb","Z01-",2,8,8}, {"BIT 1,B","x8/rsb","Z01-",2,8,8}, {"BIT 1,C","x8/rsb","Z01-",2,8,8}, {"BIT 1,D","x8/rsb","Z01-",2,8,8}, {"BIT 1,E","x8/rsb","Z01-",2,8,8}, {"BIT 1,H","x8/rsb","Z01-",2,8,8}, {"BIT 1,L","x8/rsb","Z01-",2,8,8}, {"BIT 1,(HL)","x8/rsb","Z01-",2,12,12}, {"BIT 1,A","x8/rsb","Z01-",2,8,8}, {"BIT 2,B","x8/rsb","Z01-",2,8,8}, 
	{"BIT 2,C","x8/rsb","Z01-",2,8,8}, {"BIT 2,D","x8/rsb","Z01-",2,8,8}, {"BIT 2,E","x8/rsb","Z01-",2,8,8}, {"BIT 2,H","x8/rsb","Z01-",2,8,8}, {"BIT 2,L","x8/rsb","Z01-",2,8,8}, {"BIT 2,(HL)","x8/rsb","Z01-",2,12,12}, {"BIT 2,A","x8/rsb","Z01-",2,8,8}, {"BIT 3,B","x8/rsb","Z01-",2,8,8}, {"BIT 3,C","x8/rsb","Z01-",2,8,8}, {"BIT 3,D","x8/rsb","Z01-",2,8,8}, {"BIT 3,E","x8/rsb","Z01-",2,8,8}, {"BIT 3,H","x8/rsb","Z01-",2,8,8}, {"BIT 3,L","x8/rsb","Z01-",2,8,8}, {"BIT 3,(HL)","x8/rsb","Z01-",2,12,12}, {"BIT 3,A","x8/rsb","Z01-",2,8,8}, {"BIT 4,B","x8/rsb","Z01-",2,8,8}, 
	{"BIT 4,C","x8/rsb","Z01-",2,8,8}, {"BIT 4,D","x8/rsb","Z01-",2,8,8}, {"BIT 4,E","x8/rsb","Z01-",2,8,8}, {"BIT 4,H","x8/rsb","Z01-",2,8,8}, {"BIT 4,L","x8/rsb","Z01-",2,8,8}, {"BIT 4,(HL)","x8/rsb","Z01-",2,12,12}, {"BIT 4,A","x8/rsb","Z01-",2,8,8}, {"BIT 5,B","x8/rsb","Z01-",2,8,8}, {"BIT 5,C","x8/rsb","Z01-",2,8,8}, {"BIT 5,D","x8/rsb","Z01-",2,8,8}, {"BIT 5,E","x8/rsb","Z01-",2,8,8}, {"BIT 5,H","x8/rsb","Z01-",2,8,8}, {"BIT 5,L","x8/rsb","Z01-",2,8,8}, {"BIT 5,(HL)","x8/rsb","Z01-",2,12,12}, {"BIT 5,A","x8/rsb","Z01-",2,8,8}, {"BIT 6,B","x8/rsb","Z01-",2,8,8}, 
	{"BIT 6,C","x8/rsb","Z01-",2,8,8}, {"BIT 6,D","x8/rsb","Z01-",2,8,8}, {"BIT 6,E","x8/rsb","Z01-",2,8,8}, {"BIT 6,H","x8/rsb","Z01-",2,8,8}, {"BIT 6,L","x8/rsb","Z01-",2,8,8}, {"BIT 6,(HL)","x8/rsb","Z01-",2,12,12}, {"BIT 6,A","x8/rsb","Z01-",2,8,8}, {"BIT 7,B","x8/rsb","Z01-",2,8,8}, {"BIT 7,C","x8/rsb","Z01-",2,8,8}, {"BIT 7,D","x8/rsb","Z01-",2,8,8}, {"BIT 7,E","x8/rsb","Z01-",2,8,8}, {"BIT 7,H","x8/rsb","Z01-",2,8,8}, {"BIT 7,L","x8/rsb","Z01-",2,8,8}, {"BIT 7,(HL)","x8/rsb","Z01-",2,12,12}, {"BIT 7,A","x8/rsb","Z01-",2,8,8}, {"RES 0,B","x8/rsb","----",2,8,8}, 
	{"RES 0,C","x8/rsb","----",2,8,8}, {"RES 0,D","x8/rsb","----",2,8,8}, {"RES 0,E","x8/rsb","----",2,8,8}, {"RES 0,H","x8/rsb","----",2,8,8}, {"RES 0,L","x8/rsb","----",2,8,8}, {"RES 0,(HL)","x8/rsb","----",2,16,16}, {"RES 0,A","x8/rsb","----",2,8,8}, {"RES 1,B","x8/rsb","----",2,8,8}, {"RES 1,C","x8/rsb","----",2,8,8}, {"RES 1,D","x8/rsb","----",2,8,8}, {"RES 1,E","x8/rsb","----",2,8,8}, {"RES 1,H","x8/rsb","----",2,8,8}, {"RES 1,L","x8/rsb","----",2,8,8}, {"RES 1,(HL)","x8/rsb","----",2,16,16}, {"RES 1,A","x8/rsb","----",2,8,8}, {"RES 2,B","x8/rsb","----",2,8,8}, 
	{"RES 2,C","x8/rsb","----",2,8,8}, {"RES 2,D","x8/rsb","----",2,8,8}, {"RES 2,E","x8/rsb","----",2,8,8}, {"RES 2,H","x8/rsb","----",2,8,8}, {"RES 2,L","x8/rsb","----",2,8,8}, {"RES 2,(HL)","x8/rsb","----",2,16,16}, {"RES 2,A","x8/rsb","----",2,8,8}, {"RES 3,B","x8/rsb","----",2,8,8}, {"RES 3,C","x8/rsb","----",2,8,8}, {"RES 3,D","x8/rsb","----",2,8,8}, {"RES 3,E","x8/rsb","----",2,8,8}, {"RES 3,H","x8/rsb","----",2,8,8}, {"RES 3,L","x8/rsb","----",2,8,8}, {"RES 3,(HL)","x8/rsb","----",2,16,16}, {"RES 3,A","x8/rsb","----",2,8,8}, {"RES 4,B","x8/rsb","----",2,8,8}, 
	{"RES 4,C","x8/rsb","----",2,8,8}, {"RES 4,D","x8/rsb","----",2,8,8}, {"RES 4,E","x8/rsb","----",2,8,8}, {"RES 4,H","x8/rsb","----",2,8,8}, {"RES 4,L","x8/rsb","----",2,8,8}, {"RES 4,(HL)","x8/rsb","----",2,16,16}, {"RES 4,A","x8/rsb","----",2,8,8}, {"RES 5,B","x8/rsb","----",2,8,8}, {"RES 5,C","x8/rsb","----",2,8,8}, {"RES 5,D","x8/rsb","----",2,8,8}, {"RES 5,E","x8/rsb","----",2,8,8}, {"RES 5,H","x8/rsb","----",2,8,8}, {"RES 5,L","x8/rsb","----",2,8,8}, {"RES 5,(HL)","x8/rsb","----",2,16,16}, {"RES 5,A","x8/rsb","----",2,8,8}, {"RES 6,B","x8/rsb","----",2,8,8}, 
	{"RES 6,C","x8/rsb","----",2,8,8}, {"RES 6,D","x8/rsb","----",2,8,8}, {"RES 6,E","x8/rsb","----",2,8,8}, {"RES 6,H","x8/rsb","----",2,8,8}, {"RES 6,L","x8/rsb","----",2,8,8}, {"RES 6,(HL)","x8/rsb","----",2,16,16}, {"RES 6,A","x8/rsb","----",2,8,8}, {"RES 7,B","x8/rsb","----",2,8,8}, {"RES 7,C","x8/rsb","----",2,8,8}, {"RES 7,D","x8/rsb","----",2,8,8}, {"RES 7,E","x8/rsb","----",2,8,8}, {"RES 7,H","x8/rsb","----",2,8,8}, {"RES 7,L","x8/rsb","----",2,8,8}, {"RES 7,(HL)","x8/rsb","----",2,16,16}, {"RES 7,A","x8/rsb","----",2,8,8}, {"SET 0,B","x8/rsb","----",2,8,8}, 
	{"SET 0,C","x8/rsb","----",2,8,8}, {"SET 0,D","x8/rsb","----",2,8,8}, {"SET 0,E","x8/rsb","----",2,8,8}, {"SET 0,H","x8/rsb","----",2,8,8}, {"SET 0,L","x8/rsb","----",2,8,8}, {"SET 0,(HL)","x8/rsb","----",2,16,16}, {"SET 0,A","x8/rsb","----",2,8,8}, {"SET 1,B","x8/rsb","----",2,8,8}, {"SET 1,C","x8/rsb","----",2,8,8}, {"SET 1,D","x8/rsb","----",2,8,8}, {"SET 1,E","x8/rsb","----",2,8,8}, {"SET 1,H","x8/rsb","----",2,8,8}, {"SET 1,L","x8/rsb","----",2,8,8}, {"SET 1,(HL)","x8/rsb","----",2,16,16}, {"SET 1,A","x8/rsb","----",2,8,8}, {"SET 2,B","x8/rsb","----",2,8,8}, 
	{"SET 2,C","x8/rsb","----",2,8,8}, {"SET 2,D","x8/rsb","----",2,8,8}, {"SET 2,E","x8/rsb","----",2,8,8}, {"SET 2,H","x8/rsb","----",2,8,8}, {"SET 2,L","x8/rsb","----",2,8,8}, {"SET 2,(HL)","x8/rsb","----",2,16,16}, {"SET 2,A","x8/rsb","----",2,8,8}, {"SET 3,B","x8/rsb","----",2,8,8}, {"SET 3,C","x8/rsb","----",2,8,8}, {"SET 3,D","x8/rsb","----",2,8,8}, {"SET 3,E","x8/rsb","----",2,8,8}, {"SET 3,H","x8/rsb","----",2,8,8}, {"SET 3,L","x8/rsb","----",2,8,8}, {"SET 3,(HL)","x8/rsb","----",2,16,16}, {"SET 3,A","x8/rsb","----",2,8,8}, {"SET 4,B","x8/rsb","----",2,8,8}, 
	{"SET 4,C","x8/rsb","----",2,8,8}, {"SET 4,D","x8/rsb","----",2,8,8}, {"SET 4,E","x8/rsb","----",2,8,8}, {"SET 4,H","x8/rsb","----",2,8,8}, {"SET 4,L","x8/rsb","----",2,8,8}, {"SET 4,(HL)","x8/rsb","----",2,16,16}, {"SET 4,A","x8/rsb","----",2,8,8}, {"SET 5,B","x8/rsb","----",2,8,8}, {"SET 5,C","x8/rsb","----",2,8,8}, {"SET 5,D","x8/rsb","----",2,8,8}, {"SET 5,E","x8/rsb","----",2,8,8}, {"SET 5,H","x8/rsb","----",2,8,8}, {"SET 5,L","x8/rsb","----",2,8,8}, {"SET 5,(HL)","x8/rsb","----",2,16,16}, {"SET 5,A","x8/rsb","----",2,8,8}, {"SET 6,B","x8/rsb","----",2,8,8}, 
	{"SET 6,C","x8/rsb","----",2,8,8}, {"SET 6,D","x8/rsb","----",2,8,8}, {"SET 6,E","x8/rsb","----",2,8,8}, {"SET 6,H","x8/rsb","----",2,8,8}, {"SET 6,L","x8/rsb","----",2,8,8}, {"SET 6,(HL)","x8/rsb","----",2,16,16}, {"SET 6,A","x8/rsb","----",2,8,8}, {"SET 7,B","x8/rsb","----",2,8,8}, {"SET 7,C","x8/rsb","----",2,8,8}, {"SET 7,D","x8/rsb","----",2,8,8}, {"SET 7,E","x8/rsb","----",2,8,8}, {"SET 7,H","x8/rsb","----",2,8,8}, {"SET 7,L","x8/rsb","----",2,8,8}, {"SET 7,(HL)","x8/rsb","----",2,16,16}, {"SET 7,A","x8/rsb","----",2,8,8}, 
};

#ifdef __cplusplus
}
#endif

#endif // CYCLE_TABLE_DEBUG_H
