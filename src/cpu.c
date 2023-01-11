#include "cpu.h"
#include "instructions.h"
#include <assert.h>

uint8_t mem_read_8(nes_cpu_t *, uint16_t);
uint16_t mem_read_16(nes_cpu_t *, uint16_t);

void cpu_init(nes_cpu_t *cpu, nes_memory_t *memory, nes_ppu_t *ppu)
{
	cpu->mem = memory;
	cpu->ppu = ppu;

	cpu->sp = 0xfd;
	cpu->sr = 0x24;

	//cpu->sr ^= cpu->sp;

	cpu->wait_cycles = 7;
	cpu->total_cycles = 0;
	cpu->total_cycles += cpu->wait_cycles;

	cpu->use_mmc1 = false;
	
	cpu->mmc1.shift_register = 0;
	cpu->mmc1.shift_writes = 0;

}


void cpu_reset(nes_cpu_t *cpu)
{
	//cpu->pc = 0xc000;
	cpu->pc = mem_read_16(cpu, RESET_VECTOR_ADDR);
	printf("Starting PC at: 0x%04x\n", cpu->mem->data[RESET_VECTOR_ADDR]);
}
 
static void do_irq_interrupt(nes_cpu_t *cpu)
{
	//printf("taking IRQ interrupt\n");

	oper_push_16(cpu, cpu->pc);
	oper_push_8(cpu, cpu_get_sr(cpu));
	cpu->pc = mem_read_16(cpu, IRQ_INTERRUPT_VECTOR_ADDR);
	instr_SEI(cpu, 1);
}

static void do_nmi_interrupt(nes_cpu_t *cpu)
{
	//printf("taking NMI interrupt\n");

	oper_push_16(cpu, cpu->pc);
	oper_push_8(cpu, cpu_get_sr(cpu));
	cpu->pc = mem_read_16(cpu, NMI_INTERRUPT_VECTOR_ADDR);
	cpu->ppu->NMI_output = false;
}

void cpu_check_interrupts(nes_cpu_t *cpu)
{
	// IRQ interrupt
	if (!get_flag(cpu, FLAG_I)) {
		do_irq_interrupt(cpu);
	}

	// NMI interrupt
	if (cpu->ppu->NMI_occurred && cpu->ppu->NMI_output) {
		do_nmi_interrupt(cpu);
	}
}


typedef enum {
	X_IND = 0,
	ZPG,
	IMM,
	ABS,
	IND_Y,
	ZPG_X,
	ABS_Y,
	ABS_X
} instr_addressing_mode;

const char *str_for_addressing_mode(instr_addressing_mode mode)
{
	assert(mode >= 0 && mode < 8);
	static const char *strs[] = {"X_IND", "ZPG", "IMM", "ABS", "IND_Y", "ZPG_X", "ABS_Y", "ABS_X"};
	return strs[mode];
}

instr_addressing_mode get_addressing_mode(uint32_t opcode)
{
	int base = (opcode & 0xf) >> 2;
	int odd_addr = (((opcode & 0xf0) >> 4) & 1);

	return base + (odd_addr << 2);
}



void NONE(nes_cpu_t *cpu, uint32_t instr)
{
	printf("ILLEGAL INSTRUCTION @ %04x: %06x\nAborting execution...\n", cpu->pc, instr);
	exit(-1);
}

#ifdef CPU_IMPLEMENT_ILLEGAL_OPCODES
static void (*const opcode_table[256])(nes_cpu_t *, uint32_t) = {
	//0                 1           			2           		3          				4             		 	5           			6           		7       					8          		9          					A          		B           		C          				D          				E     				F
	instr_BRK,  		instr_ORA_x_ind, 		iinstr_KILL, 		iinstr_SLO_x_ind, 		iinstr_NOP_zpg, 		instr_ORA_zpg,  		instr_ASL_zpg, 		iinstr_SLO_zpg, 			instr_PHP, 		instr_ORA_imm, 				instr_ASL_A, 	iinstr_ANC_imm, 	iinstr_NOP_abs, 		instr_ORA_abs, 			instr_ASL_abs, 		iinstr_SLO_abs,
	instr_BPL, 			instr_ORA_ind_y, 		iinstr_KILL, 		iinstr_SLO_ind_y, 		iinstr_NOP_zpg_x, 		instr_ORA_zpg_x,  		instr_ASL_zpg_x, 	iinstr_SLO_zpg_x, 			instr_CLC, 		instr_ORA_abs_y, 			iinstr_NOP, 	iinstr_SLO_abs_y, 	iinstr_NOP_abs_x, 		instr_ORA_abs_x, 		instr_ASL_abs_x, 	iinstr_SLO_abs_x,

	instr_JSR, 			instr_AND_x_ind, 		iinstr_KILL, 		iinstr_RLA_x_ind, 		instr_BIT_zpg, 			instr_AND_zpg,  		instr_ROL_zpg, 		iinstr_RLA_zpg, 			instr_PLP, 		instr_AND_imm, 				instr_ROL_A, 	iinstr_ANC_imm, 	instr_BIT_abs, 			instr_AND_abs, 			instr_ROL_abs, 		iinstr_RLA_abs,
	instr_BMI, 			instr_AND_ind_y, 		iinstr_KILL, 		iinstr_RLA_ind_y, 		iinstr_NOP_zpg_x, 		instr_AND_zpg_x,  		instr_ROL_zpg_x, 	iinstr_RLA_zpg_x, 			instr_SEC, 		instr_AND_abs_y, 			iinstr_NOP, 	iinstr_RLA_abs_y, 	iinstr_NOP_abs_x, 		instr_AND_abs_x, 		instr_ROL_abs_x,	iinstr_RLA_abs_x,

	instr_RTI, 			instr_EOR_x_ind, 		iinstr_KILL, 		iinstr_SRE_x_ind, 		iinstr_NOP_zpg, 		instr_EOR_zpg,  		instr_LSR_zpg, 		iinstr_SRE_zpg, 			instr_PHA, 		instr_EOR_imm, 				instr_LSR_A, 	iinstr_ALR_imm, 	instr_JMP_abs, 			instr_EOR_abs, 			instr_LSR_abs,		iinstr_SRE_abs,
	instr_BVC, 			instr_EOR_ind_y, 		iinstr_KILL, 		iinstr_SRE_ind_y, 		iinstr_NOP_zpg_x, 		instr_EOR_zpg_x,  		instr_LSR_zpg_x, 	iinstr_SRE_zpg_x, 			instr_CLI, 		instr_EOR_abs_y, 			iinstr_NOP, 	iinstr_SRE_abs_y, 	iinstr_NOP_abs_x, 		instr_EOR_abs_x, 		instr_LSR_abs_x, 	iinstr_SRE_abs_x,

	instr_RTS, 			instr_ADC_x_ind, 		iinstr_KILL, 		iinstr_RRA_x_ind, 		iinstr_NOP_zpg, 		instr_ADC_zpg,  		instr_ROR_zpg,		iinstr_RRA_zpg, 			instr_PLA, 		instr_ADC_imm, 				instr_ROR_A, 	iinstr_ARR_imm, 	instr_JMP_ind, 			instr_ADC_abs, 			instr_ROR_abs, 		iinstr_RRA_abs,
	instr_BVS, 			instr_ADC_ind_y, 		iinstr_KILL, 		iinstr_RRA_ind_y, 		iinstr_NOP_zpg_x, 		instr_ADC_zpg_x,  		instr_ROR_zpg_x, 	iinstr_RRA_zpg_x, 			instr_SEI, 		instr_ADC_abs_y, 			iinstr_NOP, 	iinstr_RRA_abs_y, 	iinstr_NOP_abs_x, 		instr_ADC_abs_x, 		instr_ROR_abs_x, 	iinstr_RRA_abs_x,

	iinstr_NOP_imm, 	instr_STA_x_ind, 		iinstr_NOP_imm,		iinstr_SAX_x_ind, 		instr_STY_zpg, 			instr_STA_zpg,  		instr_STX_zpg, 		iinstr_SAX_zpg, 			instr_DEY, 		iinstr_NOP_imm, 			instr_TXA, 		iinstr_XAA_imm, 	instr_STY_abs, 			instr_STA_abs, 			instr_STX_abs, 		iinstr_SAX_abs,
	instr_BCC, 			instr_STA_ind_y, 		iinstr_KILL, 		iinstr_AHX_ind_y, 		instr_STY_zpg_x, 		instr_STA_zpg_x,  		instr_STX_zpg_y, 	iinstr_SAX_zpg_x, 			instr_TYA, 		instr_STA_abs_y, 			instr_TXS, 		iinstr_TAS_abs_y, 	iinstr_SHY_abs_x, 		instr_STA_abs_x, 		iinstr_SHX_abs_y, 	iinstr_AHX_abs_y,

	instr_LDY_imm, 		instr_LDA_x_ind, 		instr_LDX_imm, 		iinstr_LAX_x_ind, 		instr_LDY_zpg,			instr_LDA_zpg,  		instr_LDX_zpg, 		iinstr_LAX_zpg, 			instr_TAY, 		instr_LDA_imm, 				instr_TAX, 		iinstr_LAX_imm, 	instr_LDY_abs, 			instr_LDA_abs, 			instr_LDX_abs, 		iinstr_LAX_abs,
	instr_BCS, 			instr_LDA_ind_y, 		iinstr_KILL, 		iinstr_LAX_ind_y, 		instr_LDY_zpg_x,   		instr_LDA_zpg_x,  		instr_LDX_zpg_y, 	iinstr_LAX_zpg_x, 			instr_CLV, 		instr_LDA_abs_y, 			instr_TSX, 		iinstr_LAS_abs_y, 	instr_LDY_abs_x, 		instr_LDA_abs_x,		instr_LDX_abs_y, 	iinstr_LAX_abs_y,

	instr_CPY_imm, 		instr_CMP_x_ind, 		iinstr_NOP_imm, 	iinstr_DCP_x_ind, 		instr_CPY_zpg,  		instr_CMP_zpg,  		instr_DEC_zpg, 		iinstr_DCP_zpg, 			instr_INY, 		instr_CMP_imm, 				instr_DEX, 		iinstr_AXS_imm, 	instr_CPY_abs, 			instr_CMP_abs, 			instr_DEC_abs, 		iinstr_DCP_abs,
	instr_BNE, 			instr_CMP_ind_y, 		iinstr_KILL, 		iinstr_DCP_ind_y, 		iinstr_NOP_zpg_x,  		instr_CMP_zpg_x,  		instr_DEC_zpg_x, 	iinstr_DCP_zpg_x, 			instr_CLD, 		instr_CMP_abs_y, 			iinstr_NOP,		iinstr_DCP_abs_y, 	iinstr_NOP_abs_x, 		instr_CMP_abs_x, 		instr_DEC_abs_x,	iinstr_DCP_abs_x,

	instr_CPX_imm, 		instr_SBC_x_ind, 		iinstr_NOP_imm, 	iinstr_ISC_x_ind, 		instr_CPX_zpg,  		instr_SBC_zpg,  		instr_INC_zpg, 		iinstr_ISC_zpg, 			instr_INX, 		instr_SBC_imm, 				instr_NOP, 		iinstr_SBC_imm, 	instr_CPX_abs, 			instr_SBC_abs, 			instr_INC_abs, 		iinstr_ISC_abs,
	instr_BEQ, 			instr_SBC_ind_y, 		iinstr_KILL, 		iinstr_ISC_ind_y, 		iinstr_NOP_zpg_x,  		instr_SBC_zpg_x,  		instr_INC_zpg_x, 	iinstr_ISC_zpg_x, 			instr_SED, 		instr_SBC_abs_y, 			iinstr_NOP, 	iinstr_ISC_abs_y, 	iinstr_NOP_abs_x, 		instr_SBC_abs_x, 		instr_INC_abs_x, 	iinstr_ISC_abs_x
};
#else
static void (*const opcode_table[256])(nes_cpu_t *, uint32_t) = {
	/* 0x00 - 0x07, 0x08 - 0xF */
	//0                 1           			2           		3           4             		5           			6           		7       8          		9          					A          		B           C          		D          				E     F
	instr_BRK,  		instr_ORA_x_ind, 		NONE, 				NONE, 		NONE, 				instr_ORA_zpg,  		instr_ASL_zpg, 		NONE, 	instr_PHP, 		instr_ORA_imm, 				instr_ASL_A, 	NONE, 		NONE, 					instr_ORA_abs, 			instr_ASL_abs, 	NONE,
	instr_BPL, 			instr_ORA_ind_y, 		NONE, 				NONE, 		NONE, 				instr_ORA_zpg_x,  		instr_ASL_zpg_x, 	NONE, 	instr_CLC, 		instr_ORA_abs_y, 			NONE, 		 	NONE, 		NONE, 					instr_ORA_abs_x, 		instr_ASL_abs_x, NONE,

	instr_JSR, 			instr_AND_x_ind, 		NONE, 				NONE, 		instr_BIT_zpg, 		instr_AND_zpg,  		instr_ROL_zpg, 		NONE, 	instr_PLP, 		instr_AND_imm, 				instr_ROL_A, 	NONE, 		instr_BIT_abs, 			instr_AND_abs, 			instr_ROL_abs, NONE,
	instr_BMI, 			instr_AND_ind_y, 		NONE, 				NONE, 		NONE, 				instr_AND_zpg_x,  		instr_ROL_zpg_x, 	NONE, 	instr_SEC, 		instr_AND_abs_y, 			NONE, 			NONE, 		NONE, 					instr_AND_abs_x, 		instr_ROL_abs_x, NONE,

	instr_RTI, 			instr_EOR_x_ind, 		NONE, 				NONE, 		NONE, 				instr_EOR_zpg,  		instr_LSR_zpg, 		NONE, 	instr_PHA, 		instr_EOR_imm, 				instr_LSR_A, 	NONE, 		instr_JMP_abs, 			instr_EOR_abs, 			instr_LSR_abs, NONE,
	instr_BVC, 			instr_EOR_ind_y, 		NONE, 				NONE, 		NONE, 				instr_EOR_zpg_x,  		instr_LSR_zpg_x, 	NONE, 	instr_CLI, 		instr_EOR_abs_y, 			NONE, 			NONE, 		NONE, 					instr_EOR_abs_x, 		instr_LSR_abs_x, NONE,

	instr_RTS, 			instr_ADC_x_ind, 		NONE, 				NONE, 		NONE, 				instr_ADC_zpg,  		instr_ROR_zpg,		NONE, 	instr_PLA, 		instr_ADC_imm, 				instr_ROR_A, 	NONE, 		instr_JMP_ind, 			instr_ADC_abs, 			instr_ROR_abs, NONE,
	instr_BVS, 			instr_ADC_ind_y, 		NONE, 				NONE, 		NONE, 				instr_ADC_zpg_x,  		instr_ROR_zpg_x, 	NONE, 	instr_SEI, 		instr_ADC_abs_y, 			NONE, 			NONE, 		NONE, 					instr_ADC_abs_x, 		instr_ROR_abs_x, NONE,

	NONE, 				instr_STA_x_ind, 		NONE,				NONE, 		instr_STY_zpg, 		instr_STA_zpg,  		instr_STX_zpg, 		NONE, 	instr_DEY, 		NONE, 						instr_TXA, 		NONE, 		instr_STY_abs, 			instr_STA_abs, 			instr_STX_abs, NONE,
	instr_BCC, 			instr_STA_ind_y, 		NONE, 				NONE, 		instr_STY_zpg_x, 	instr_STA_zpg_x,  		instr_STX_zpg_y, 	NONE, 	instr_TYA, 		instr_STA_abs_y, 			instr_TXS, 		NONE, 		NONE, 					instr_STA_abs_x, 		NONE, NONE,

	instr_LDY_imm, 		instr_LDA_x_ind, 		instr_LDX_imm, 		NONE, 		instr_LDY_zpg,		instr_LDA_zpg,  		instr_LDX_zpg, 		NONE, 	instr_TAY, 		instr_LDA_imm, 				instr_TAX, 		NONE, 		instr_LDY_abs, 			instr_LDA_abs, 			instr_LDX_abs, NONE,
	instr_BCS, 			instr_LDA_ind_y, 		NONE, 				NONE, 		instr_LDY_zpg_x,   	instr_LDA_zpg_x,  		instr_LDX_zpg_y, 	NONE, 	instr_CLV, 		instr_LDA_abs_y, 			instr_TSX, 		NONE, 		instr_LDY_abs_x, 		instr_LDA_abs_x,		instr_LDX_abs_y, NONE,

	instr_CPY_imm, 		instr_CMP_x_ind, 		NONE, 				NONE, 		instr_CPY_zpg,  	instr_CMP_zpg,  		instr_DEC_zpg, 		NONE, 	instr_INY, 		instr_CMP_imm, 				instr_DEX, 		NONE, 		instr_CPY_abs, 			instr_CMP_abs, 			instr_DEC_abs, NONE,
	instr_BNE, 			instr_CMP_ind_y, 		NONE, 				NONE, 		NONE,  				instr_CMP_zpg_x,  		instr_DEC_zpg_x, 	NONE, 	instr_CLD, 		instr_CMP_abs_y, 			NONE,			NONE, 		NONE, 					instr_CMP_abs_x, 		instr_DEC_abs_x, NONE,

	instr_CPX_imm, 		instr_SBC_x_ind, 		NONE, 				NONE, 		instr_CPX_zpg,  	instr_SBC_zpg,  		instr_INC_zpg, 		NONE, 	instr_INX, 		instr_SBC_imm, 				instr_NOP, 		NONE, 		instr_CPX_abs, 			instr_SBC_abs, 			instr_INC_abs, NONE,
	instr_BEQ, 			instr_SBC_ind_y, 		NONE, 				NONE, 		NONE,  				instr_SBC_zpg_x,  		instr_INC_zpg_x, 	NONE, 	instr_SED, 		instr_SBC_abs_y, 			NONE, 			NONE, 		NONE, 					instr_SBC_abs_x, 		instr_INC_abs_x, NONE
};
#endif


static const uint8_t size_table[256] = {
//      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
/* 00*/	2, 2, 0, 2, 1, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3, // good
/* 10*/	2, 2, 0, 2, 1, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3, // good
/* 20*/	3, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 30*/	2, 2, 0, 2, 1, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* 40*/	1, 2, 0, 2, 1, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 50*/	2, 2, 0, 2, 1, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* 60*/	1, 2, 0, 2, 1, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 70*/	2, 2, 0, 2, 1, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* 80*/	2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 0, 3, 3, 3, 3,
/* 90*/	2, 2, 0, 0, 2, 2, 2, 2, 1, 3, 1, 0, 3, 3, 3, 3,
/* A0*/	2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 0, 3, 3, 3, 3,
/* B0*/	2, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 0, 3, 3, 3, 3,
/* C0*/	2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* D0*/	2, 2, 0, 2, 1, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* E0*/ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* F0*/	2, 2, 0, 2, 1, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3
};

static const uint8_t cycle_count_table[256] = {
//      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
/*00*/	2, 6, 2, 2, 2, 3, 5, 2, 3, 2, 2, 2, 2, 4, 6, 2,
		2, 5, 2, 2, 2, 4, 6, 2, 2, 4, 2, 2, 2, 4, 7, 2,
		6, 6, 2, 2, 3, 3, 5, 2, 4, 2, 2, 2, 4, 4, 6, 2,
		2, 5, 2, 2, 2, 4, 6, 2, 2, 4, 2, 2, 2, 4, 7, 2,
/*40*/	6, 6, 2, 2, 2, 3, 5, 2, 3, 2, 2, 2, 3, 4, 6, 2,
		2, 5, 2, 2, 2, 4, 6, 2, 2, 4, 2, 2, 2, 4, 7, 2,
		6, 6, 2, 2, 2, 3, 5, 2, 4, 2, 2, 2, 5, 4, 6, 2,
		2, 5, 2, 2, 2, 4, 6, 2, 2, 4, 2, 2, 2, 4, 7, 2,
/*80*/	2, 6, 2, 2, 3, 3, 3, 2, 2, 2, 2, 2, 4, 4, 4, 2,
		2, 6, 2, 2, 4, 4, 4, 2, 2, 5, 2, 2, 2, 5, 2, 2,
		2, 6, 2, 2, 3, 3, 3, 2, 2, 2, 2, 2, 4, 4, 4, 2,
		2, 5, 2, 2, 4, 4, 4, 2, 2, 4, 2, 2, 4, 4, 4, 2,
/*C0*/	2, 6, 2, 2, 3, 3, 5, 2, 2, 2, 2, 2, 4, 4, 6, 2,
		2, 5, 2, 2, 2, 4, 6, 2, 2, 4, 2, 2, 2, 4, 7, 2,
		2, 6, 2, 2, 3, 3, 5, 2, 2, 2, 2, 2, 4, 4, 6, 2,
		2, 5, 2, 2, 2, 4, 6, 2, 2, 4, 2, 2, 2, 4, 7, 2
};


uint32_t cpu_fetch_instruction(nes_cpu_t *cpu)
{

	uint8_t op = mem_read_8(cpu, cpu->pc);
	uint8_t sz = size_table[op];
	uint32_t final_opcode = op << 16;

	if (sz == 2) {
		uint8_t imm8 = mem_read_8(cpu, cpu->pc + 1);
		final_opcode = (op << 16) | (imm8 << 8);
	} else if (sz == 3) {
		uint16_t imm16 = mem_read_16(cpu, cpu->pc + 1);
		final_opcode = (op << 16) | imm16;
	}
	return final_opcode;
}

void cpu_execute_instruction(nes_cpu_t *cpu, uint32_t instruction)
{
	opcode_table[instruction >> 16](cpu, instruction);
}

#ifdef DEBUG
void log_debug_info(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t op = (instr >> 16) & 0xff;
	uint8_t sz = size_table[op];

	static FILE *debug_file = NULL;
	static FILE *companion = NULL;
	static int NUM_INSTRUCTIONS = 0;
	if (!debug_file) {
		debug_file = fopen("debug/path.log", "w+");
	}
	if (!companion) {
		companion = fopen("debug/companion.log", "w+");
	}
	fprintf(debug_file, "pc:%04X\t", cpu->pc);

	fprintf(debug_file, "%06x", instr);
	fprintf(debug_file, "\t\t\t");
	fprintf(companion, "%08i %04x\tA:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%i\n", NUM_INSTRUCTIONS + 1, cpu->pc, cpu->a, cpu->x,
		cpu->y, cpu->sr, cpu->sp, cpu->total_cycles);
	fprintf(debug_file, "A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%i\n", cpu->a, cpu->x,
		cpu->y, cpu->sr, cpu->sp, cpu->total_cycles);
	NUM_INSTRUCTIONS++;
}
#endif

void cpu_update_registers(nes_cpu_t *cpu, uint8_t key_state)
{
	// player 1
	if (cpu->strobe_keys) {
		cpu->key_state = key_state;
	}
}


void cpu_run_cycle(nes_cpu_t *cpu)
{
	uint32_t instr = cpu_fetch_instruction(cpu);
	uint8_t op = (instr >> 16) & 0xff;
	uint8_t sz = size_table[op];

	int instr_cycle_count = cycle_count_table[op];

	#ifdef DEBUG
	log_debug_info(cpu, instr);
	#endif

	cpu_execute_instruction(cpu, instr);
	
	cpu->pc += sz;
	cpu->wait_cycles += instr_cycle_count;
	cpu->total_cycles += cpu->wait_cycles;

	cpu_check_interrupts(cpu);
}

uint8_t cpu_get_sr(nes_cpu_t *cpu) {
	return 	cpu->flags[FLAG_C] |
		   	(cpu->flags[FLAG_Z] << 1) |
			(cpu->flags[FLAG_I] << 2) |
			(cpu->flags[FLAG_D] << 3) |	
			(cpu->flags[FLAG_B] << 4) |
			(cpu->flags[FLAG_V] << 6) |
			(cpu->flags[FLAG_N] << 7);
}

void cpu_set_sr(nes_cpu_t *cpu, uint8_t new_sr) {
	for (int i = 7; i < 0; i++) {
		cpu->flags[i] = new_sr >> (7 - i);
	}
}

void cpu_cleanup(nes_cpu_t *cpu)
{
	
}