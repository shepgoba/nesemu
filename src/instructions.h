#ifndef INSTRUCTIONS_INCLUDE
#define INSTRUCTIONS_INCLUDE
#include "cpu.h"
#include "memory.h"

void oper_push_8(nes_cpu_t *cpu, uint8_t value);
void oper_push_16(nes_cpu_t *cpu, uint16_t value);

uint8_t oper_pop_8(nes_cpu_t *cpu);
uint16_t oper_pop_16(nes_cpu_t *cpu);

void set_flag(nes_cpu_t *cpu, uint8_t flag, int enable);
bool get_flag(nes_cpu_t *cpu, int flag);

void instr_ADC_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_ADC_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_ADC_abs_y(nes_cpu_t *cpu, uint32_t instr);
void instr_ADC_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_ADC_ind_y(nes_cpu_t *cpu, uint32_t instr);
void instr_ADC_x_ind(nes_cpu_t *cpu, uint32_t instr);
void instr_ADC_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_ADC_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_AND_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_AND_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_AND_abs_y(nes_cpu_t *cpu, uint32_t instr);
void instr_AND_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_AND_ind_y(nes_cpu_t *cpu, uint32_t instr);
void instr_AND_x_ind(nes_cpu_t *cpu, uint32_t instr);
void instr_AND_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_AND_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_ASL_A(nes_cpu_t *cpu, uint32_t instr);
void instr_ASL_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_ASL_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_ASL_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_ASL_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_BCC(nes_cpu_t *cpu, uint32_t instr);
void instr_BCS(nes_cpu_t *cpu, uint32_t instr);
void instr_BEQ(nes_cpu_t *cpu, uint32_t instr);
void instr_BIT_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_BIT_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_BMI(nes_cpu_t *cpu, uint32_t instr);
void instr_BNE(nes_cpu_t *cpu, uint32_t instr);
void instr_BPL(nes_cpu_t *cpu, uint32_t instr);
void instr_BRK(nes_cpu_t *cpu, uint32_t instr);
void instr_BVC(nes_cpu_t *cpu, uint32_t instr);
void instr_BVS(nes_cpu_t *cpu, uint32_t instr);
void instr_CLC(nes_cpu_t *cpu, uint32_t instr);
void instr_CLD(nes_cpu_t *cpu, uint32_t instr);
void instr_CLI(nes_cpu_t *cpu, uint32_t instr);
void instr_CLV(nes_cpu_t *cpu, uint32_t instr);
void instr_CMP_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_CMP_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_CMP_abs_y(nes_cpu_t *cpu, uint32_t instr);
void instr_CMP_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_CMP_ind_y(nes_cpu_t *cpu, uint32_t instr);
void instr_CMP_x_ind(nes_cpu_t *cpu, uint32_t instr);
void instr_CMP_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_CMP_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_CPX_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_CPX_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_CPX_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_CPY_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_CPY_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_CPY_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_DEC_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_DEC_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_DEC_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_DEC_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_DEX(nes_cpu_t *cpu, uint32_t instr);
void instr_DEY(nes_cpu_t *cpu, uint32_t instr);
void instr_EOR_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_EOR_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_EOR_abs_y(nes_cpu_t *cpu, uint32_t instr);
void instr_EOR_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_EOR_ind_y(nes_cpu_t *cpu, uint32_t instr);
void instr_EOR_x_ind(nes_cpu_t *cpu, uint32_t instr);
void instr_EOR_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_EOR_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_INC_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_INC_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_INC_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_INC_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_INX(nes_cpu_t *cpu, uint32_t instr);
void instr_INY(nes_cpu_t *cpu, uint32_t instr);
void instr_JMP_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_JMP_ind(nes_cpu_t *cpu, uint32_t instr);
void instr_JSR(nes_cpu_t *cpu, uint32_t instr);
void instr_LDA_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_LDA_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_LDA_abs_y(nes_cpu_t *cpu, uint32_t instr);
void instr_LDA_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_LDA_ind_y(nes_cpu_t *cpu, uint32_t instr);
void instr_LDA_x_ind(nes_cpu_t *cpu, uint32_t instr);
void instr_LDA_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_LDA_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_LDX_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_LDX_abs_y(nes_cpu_t *cpu, uint32_t instr);
void instr_LDX_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_LDX_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_LDX_zpg_y(nes_cpu_t *cpu, uint32_t instr);
void instr_LDY_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_LDY_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_LDY_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_LDY_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_LDY_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_LSR_A(nes_cpu_t *cpu, uint32_t instr);
void instr_LSR_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_LSR_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_LSR_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_LSR_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_NOP(nes_cpu_t *cpu, uint32_t instr);
void instr_ORA_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_ORA_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_ORA_abs_y(nes_cpu_t *cpu, uint32_t instr);
void instr_ORA_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_ORA_ind_y(nes_cpu_t *cpu, uint32_t instr);
void instr_ORA_x_ind(nes_cpu_t *cpu, uint32_t instr);
void instr_ORA_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_ORA_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_PHA(nes_cpu_t *cpu, uint32_t instr);
void instr_PHP(nes_cpu_t *cpu, uint32_t instr);
void instr_PLA(nes_cpu_t *cpu, uint32_t instr);
void instr_PLP(nes_cpu_t *cpu, uint32_t instr);
void instr_ROL_A(nes_cpu_t *cpu, uint32_t instr);
void instr_ROL_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_ROL_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_ROL_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_ROL_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_ROR_A(nes_cpu_t *cpu, uint32_t instr);
void instr_ROR_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_ROR_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_ROR_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_ROR_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_RTI(nes_cpu_t *cpu, uint32_t instr);
void instr_RTS(nes_cpu_t *cpu, uint32_t instr);
void instr_SBC_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_SBC_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_SBC_abs_y(nes_cpu_t *cpu, uint32_t instr);
void instr_SBC_imm(nes_cpu_t *cpu, uint32_t instr);
void instr_SBC_ind_y(nes_cpu_t *cpu, uint32_t instr);
void instr_SBC_x_ind(nes_cpu_t *cpu, uint32_t instr);
void instr_SBC_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_SBC_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_SEC(nes_cpu_t *cpu, uint32_t instr);
void instr_SED(nes_cpu_t *cpu, uint32_t instr);
void instr_SEI(nes_cpu_t *cpu, uint32_t instr);
void instr_STA_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_STA_abs_x(nes_cpu_t *cpu, uint32_t instr);
void instr_STA_abs_y(nes_cpu_t *cpu, uint32_t instr);
void instr_STA_ind_y(nes_cpu_t *cpu, uint32_t instr);
void instr_STA_x_ind(nes_cpu_t *cpu, uint32_t instr);
void instr_STA_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_STA_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_STX_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_STX_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_STX_zpg_y(nes_cpu_t *cpu, uint32_t instr);
void instr_STY_abs(nes_cpu_t *cpu, uint32_t instr);
void instr_STY_zpg(nes_cpu_t *cpu, uint32_t instr);
void instr_STY_zpg_x(nes_cpu_t *cpu, uint32_t instr);
void instr_TAX(nes_cpu_t *cpu, uint32_t instr);
void instr_TAY(nes_cpu_t *cpu, uint32_t instr);
void instr_TSX(nes_cpu_t *cpu, uint32_t instr);
void instr_TXA(nes_cpu_t *cpu, uint32_t instr);
void instr_TXS(nes_cpu_t *cpu, uint32_t instr);
void instr_TYA(nes_cpu_t *cpu, uint32_t instr);

#ifdef CPU_IMPLEMENT_ILLEGAL_OPCODES
// illegal/undefined instructions
void iinstr_AHX_abs_y(nes_cpu_t *, uint32_t);
void iinstr_AHX_ind_y(nes_cpu_t *, uint32_t);
void iinstr_ALR_imm(nes_cpu_t *, uint32_t);
void iinstr_ANC_imm(nes_cpu_t *, uint32_t);
void iinstr_ARR_imm(nes_cpu_t *, uint32_t);
void iinstr_AXS_imm(nes_cpu_t *, uint32_t);
void iinstr_DCP_abs(nes_cpu_t *, uint32_t);
void iinstr_DCP_abs_x(nes_cpu_t *, uint32_t);
void iinstr_DCP_abs_y(nes_cpu_t *, uint32_t);
void iinstr_DCP_ind_y(nes_cpu_t *, uint32_t);
void iinstr_DCP_x_ind(nes_cpu_t *, uint32_t);
void iinstr_DCP_zpg(nes_cpu_t *, uint32_t);
void iinstr_DCP_zpg_x(nes_cpu_t *, uint32_t);
void iinstr_ISC_abs(nes_cpu_t *, uint32_t);
void iinstr_ISC_abs_x(nes_cpu_t *, uint32_t);
void iinstr_ISC_abs_y(nes_cpu_t *, uint32_t);
void iinstr_ISC_ind_y(nes_cpu_t *, uint32_t);
void iinstr_ISC_x_ind(nes_cpu_t *, uint32_t);
void iinstr_ISC_zpg(nes_cpu_t *, uint32_t);
void iinstr_ISC_zpg_x(nes_cpu_t *, uint32_t);
void iinstr_KILL(nes_cpu_t *, uint32_t);
void iinstr_LAS_abs_y(nes_cpu_t *, uint32_t);
void iinstr_LAX_abs(nes_cpu_t *, uint32_t);
void iinstr_LAX_abs_y(nes_cpu_t *, uint32_t);
void iinstr_LAX_imm(nes_cpu_t *, uint32_t);
void iinstr_LAX_ind_y(nes_cpu_t *, uint32_t);
void iinstr_LAX_x_ind(nes_cpu_t *, uint32_t);
void iinstr_LAX_zpg(nes_cpu_t *, uint32_t);
void iinstr_LAX_zpg_x(nes_cpu_t *, uint32_t);
void iinstr_NOP(nes_cpu_t *, uint32_t);
void iinstr_NOP_abs(nes_cpu_t *, uint32_t);
void iinstr_NOP_abs_x(nes_cpu_t *, uint32_t);
void iinstr_NOP_imm(nes_cpu_t *, uint32_t);
void iinstr_NOP_zpg(nes_cpu_t *, uint32_t);
void iinstr_NOP_zpg_x(nes_cpu_t *, uint32_t);
void iinstr_RLA_abs(nes_cpu_t *, uint32_t);
void iinstr_RLA_abs_x(nes_cpu_t *, uint32_t);
void iinstr_RLA_abs_y(nes_cpu_t *, uint32_t);
void iinstr_RLA_ind_y(nes_cpu_t *, uint32_t);
void iinstr_RLA_x_ind(nes_cpu_t *, uint32_t);
void iinstr_RLA_zpg(nes_cpu_t *, uint32_t);
void iinstr_RLA_zpg_x(nes_cpu_t *, uint32_t);
void iinstr_RRA_abs(nes_cpu_t *, uint32_t);
void iinstr_RRA_abs_x(nes_cpu_t *, uint32_t);
void iinstr_RRA_abs_y(nes_cpu_t *, uint32_t);
void iinstr_RRA_ind_y(nes_cpu_t *, uint32_t);
void iinstr_RRA_x_ind(nes_cpu_t *, uint32_t);
void iinstr_RRA_zpg(nes_cpu_t *, uint32_t);
void iinstr_RRA_zpg_x(nes_cpu_t *, uint32_t);
void iinstr_SAX_abs(nes_cpu_t *, uint32_t);
void iinstr_SAX_x_ind(nes_cpu_t *, uint32_t);
void iinstr_SAX_zpg(nes_cpu_t *, uint32_t);
void iinstr_SAX_zpg_x(nes_cpu_t *, uint32_t);
void iinstr_SBC_imm(nes_cpu_t *, uint32_t);
void iinstr_SHX_abs_y(nes_cpu_t *, uint32_t);
void iinstr_SHY_abs_x(nes_cpu_t *, uint32_t);
void iinstr_SLO_abs(nes_cpu_t *, uint32_t);
void iinstr_SLO_abs_x(nes_cpu_t *, uint32_t);
void iinstr_SLO_abs_y(nes_cpu_t *, uint32_t);
void iinstr_SLO_ind_y(nes_cpu_t *, uint32_t);
void iinstr_SLO_x_ind(nes_cpu_t *, uint32_t);
void iinstr_SLO_zpg(nes_cpu_t *, uint32_t);
void iinstr_SLO_zpg_x(nes_cpu_t *, uint32_t);
void iinstr_SRE_abs(nes_cpu_t *, uint32_t);
void iinstr_SRE_abs_x(nes_cpu_t *, uint32_t);
void iinstr_SRE_abs_y(nes_cpu_t *, uint32_t);
void iinstr_SRE_ind_y(nes_cpu_t *, uint32_t);
void iinstr_SRE_x_ind(nes_cpu_t *, uint32_t);
void iinstr_SRE_zpg(nes_cpu_t *, uint32_t);
void iinstr_SRE_zpg_x(nes_cpu_t *, uint32_t);
void iinstr_TAS_abs_y(nes_cpu_t *, uint32_t);
void iinstr_XAA_imm(nes_cpu_t *, uint32_t);
#endif

#endif