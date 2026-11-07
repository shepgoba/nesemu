#include "disassembler.h"
#include <stdio.h>

#ifndef LOWERCASE_MNEMONICS
#define LOWERCASE_MNEMONICS 1
#endif

#if LOWERCASE_MNEMONICS
#include <ctype.h>

static const char *lower_filter(const char *mn)
{
	static char lower[8];
	for (size_t i = 0; mn[i] != '\0' && i < sizeof(lower) - 1; i++) {
		lower[i] = (char)tolower((unsigned char)mn[i]);
	}
	lower[7] = '\0';
	return lower;
}
#else
#define lower_filter(mn) (mn)
#endif

static const char *some_mnemonics[8] = {"ORA", "AND", "EOR", "ADC", "STA", "LDA", "CMP", "SBC"};
static const char *other_mnemonics[8] = {"ASL", "ROL", "LSR", "ROR", "STX", "LDX", "DEC", "INC"};
static const char *implied_mnemonics[16] = {
	"PHP", "CLC", "PLP", "SEC",
	"PHA", "CLI", "PLA", "SEI",
	"DEY", "TYA", "TAY", "CLV",
	"INY", "CLD", "INX", "SED"
};
static const char *branch_mnemonics[8] = {"BPL", "BMI", "BVC", "BVS", "BCC", "BCS", "BNE", "BEQ"};
static const char *A_mnemonics[4] = {"ASL", "ROL", "LSR", "ROR"};

static const char *mn_brk = "BRK";
static const char *mn_jsr = "JSR";
static const char *mn_rti = "RTI";
static const char *mn_rts = "RTS";
static const char *mn_ldy = "LDY";
static const char *mn_cpy = "CPY";
static const char *mn_cpx = "CPX";
static const char *mn_ldx = "LDX";
static const char *mn_bit = "BIT";
static const char *mn_sty = "STY";
static const char *mn_txa = "TXA";
static const char *mn_txs = "TXS";
static const char *mn_tax = "TAX";
static const char *mn_tsx = "TSX";
static const char *mn_dex = "DEX";
static const char *mn_nop = "NOP";
static const char *mn_jmp = "JMP";

bool disasm_instr(uint32_t instr, char *buf, size_t buf_len, uint16_t pc)
{
	bool unknown = false;
	uint8_t opc = (instr >> 16) & 0xff;
	uint8_t opc_nib_hi = (opc >> 4) & 0xf;
	uint8_t opc_nib_lo = opc & 0xf;

	uint8_t imm8 = (instr >> 8) & 0xff;
	uint16_t imm16 = instr & 0xffff;

	if (opc_nib_lo == 0) {
		if (opc_nib_hi & 1) {
			int8_t simm8 = (instr >> 8) & 0xff;
			snprintf(buf, buf_len, "%s $%04X", lower_filter(branch_mnemonics[opc_nib_hi >> 1]), pc + simm8);
		} else {
			if (opc_nib_hi < 8) {
				if (opc_nib_hi == 0) {
					snprintf(buf, buf_len, "%s", lower_filter(mn_brk));
				} else if (opc_nib_hi == 2) {
					snprintf(buf, buf_len, "%s $%04X", lower_filter(mn_jsr), imm16);
				} else if (opc_nib_hi == 4) {
					snprintf(buf, buf_len, "%s", lower_filter(mn_rti));
				} else if (opc_nib_hi == 6) {
					snprintf(buf, buf_len, "%s", lower_filter(mn_rts));
				}
			} else if (opc_nib_hi == 8) {
				unknown = true;
			} else {
				if (opc_nib_hi == 0xa) {
					snprintf(buf, buf_len, "%s #$%02X", lower_filter(mn_ldy), imm8);
				} else if (opc_nib_hi == 0xc) {
					snprintf(buf, buf_len, "%s #$%02X", lower_filter(mn_cpy), imm8);
				} else if (opc_nib_hi == 0xe) {
					snprintf(buf, buf_len, "%s #$%02X", lower_filter(mn_cpx), imm8);
				}
			}
		}
	} else if (opc_nib_lo == 1) {
		const char *mnemonic = some_mnemonics[opc_nib_hi >> 1];
		if (opc_nib_hi & 1) {
			snprintf(buf, buf_len, "%s ($%02X), Y", lower_filter(mnemonic), imm8);
		} else {
			snprintf(buf, buf_len, "%s ($%02X, X)", lower_filter(mnemonic), imm8);
		}
	} else if (opc_nib_lo == 2) {
		if (opc_nib_hi == 0xa) {
			snprintf(buf, buf_len, "%s #$%02X", lower_filter(mn_ldx), imm8);
		} else {
			unknown = true;
		}
	} else if (opc_nib_lo == 4) {
		if (opc_nib_hi == 2) {
			snprintf(buf, buf_len, "%s $%02X", lower_filter(mn_bit), imm8);
		} else if (opc_nib_hi == 8) {
			snprintf(buf, buf_len, "%s $%02X", lower_filter(mn_sty), imm8);
		} else if (opc_nib_hi == 9) {
			snprintf(buf, buf_len, "%s $%02X, X", lower_filter(mn_sty), imm8);
		} else if (opc_nib_hi == 0xa) {
			snprintf(buf, buf_len, "%s $%02X", lower_filter(mn_ldy), imm8);
		} else if (opc_nib_hi == 0xb) {
			snprintf(buf, buf_len, "%s $%02X, X", lower_filter(mn_ldy), imm8);
		} else if (opc_nib_hi == 0xc) {
			snprintf(buf, buf_len, "%s $%02X", lower_filter(mn_cpy), imm8);
		} else if (opc_nib_hi == 0xe) {
			snprintf(buf, buf_len, "%s $%02X", lower_filter(mn_cpx), imm8);
		} else {
			unknown = true;
		}
	} else if (opc_nib_lo == 5) {
		const char *mnemonic = some_mnemonics[opc_nib_hi >> 1];
		if (opc_nib_hi & 1) {
			snprintf(buf, buf_len, "%s $%02X, X", lower_filter(mnemonic), imm8);
		} else {
			snprintf(buf, buf_len, "%s $%02X", lower_filter(mnemonic), imm8);
		}
	} else if (opc_nib_lo == 6) {
		bool is_ldx_stx = (opc_nib_hi == 9 || opc_nib_hi == 0xb);
		char index_reg = is_ldx_stx ? 'Y' : 'X';
		const char *mnemonic = other_mnemonics[opc_nib_hi >> 1];
		if (opc_nib_hi & 1) {
			snprintf(buf, buf_len, "%s $%02X, %c", lower_filter(mnemonic), imm8, index_reg);
		} else {
			snprintf(buf, buf_len, "%s $%02X", lower_filter(mnemonic), imm8);
		}
	} else if (opc_nib_lo == 8) {
		snprintf(buf, buf_len, "%s", lower_filter(implied_mnemonics[opc_nib_hi]));
	} else if (opc_nib_lo == 9) {
		if (opc_nib_hi == 8) {
			unknown = true;
		} else {
			const char *mnemonic = some_mnemonics[opc_nib_hi >> 1];
			if (opc_nib_hi & 1) {
				snprintf(buf, buf_len, "%s $%04X, Y", lower_filter(mnemonic), imm16);
			} else {
				snprintf(buf, buf_len, "%s #$%02X", lower_filter(mnemonic), imm8);
			}
		}
	} else if (opc_nib_lo == 0xa) {
		if (opc_nib_hi < 8) {
			if (opc_nib_hi & 1) {
				unknown = true;
			} else {
				snprintf(buf, buf_len, "%s A", lower_filter(A_mnemonics[opc_nib_hi >> 1]));
			}
		} else {
			if (opc_nib_hi == 8) {
				snprintf(buf, buf_len, "%s", lower_filter(mn_txa));
			} else if (opc_nib_hi == 9) {
				snprintf(buf, buf_len, "%s", lower_filter(mn_txs));
			} else if (opc_nib_hi == 0xa) {
				snprintf(buf, buf_len, "%s", lower_filter(mn_tax));
			} else if (opc_nib_hi == 0xb) {
				snprintf(buf, buf_len, "%s", lower_filter(mn_tsx));
			} else if (opc_nib_hi == 0xc) {
				snprintf(buf, buf_len, "%s", lower_filter(mn_dex));
			} else if (opc_nib_hi == 0xe) {
				snprintf(buf, buf_len, "%s", lower_filter(mn_nop));
			} else {
				unknown = true;
			}
		}
	} else if (opc_nib_lo == 0xc)  {
		if (opc_nib_hi == 2) {
			snprintf(buf, buf_len, "%s $%04X", lower_filter(mn_bit), imm16);
		} else if (opc_nib_hi == 4) {
			snprintf(buf, buf_len, "%s $%04X", lower_filter(mn_jmp), imm16);
		} else if (opc_nib_hi == 6) {
			snprintf(buf, buf_len, "%s ($%04X)", lower_filter(mn_jmp), imm16);
		} else if (opc_nib_hi == 8) {
			snprintf(buf, buf_len, "%s $%04X", lower_filter(mn_sty), imm16);
		} else if (opc_nib_hi == 0xa) {
			snprintf(buf, buf_len, "%s $%04X", lower_filter(mn_ldy), imm16);
		} else if (opc_nib_hi == 0xb) {
			snprintf(buf, buf_len, "%s $%04X, X", lower_filter(mn_ldy), imm16);
		} else if (opc_nib_hi == 0xc) {
			snprintf(buf, buf_len, "%s $%04X", lower_filter(mn_cpy), imm16);
		} else if (opc_nib_hi == 0xe) {
			snprintf(buf, buf_len, "%s $%04X", lower_filter(mn_cpx), imm16);
		}
	} else if (opc_nib_lo == 0xd)  {
		const char *mnemonic = some_mnemonics[opc_nib_hi >> 1];
		if (opc_nib_hi & 1) {
			snprintf(buf, buf_len, "%s $%04X, X", lower_filter(mnemonic), imm16);
		} else {
			snprintf(buf, buf_len, "%s $%04X", lower_filter(mnemonic), imm16);
		}
	} else if (opc_nib_lo == 0xe)  {
		if (opc_nib_hi == 9) {
			unknown = true;
		} else if (opc_nib_hi == 0xb) {
		 	snprintf(buf, buf_len, "%s $%04X, Y", lower_filter(mn_ldx), imm16);
		} else {
			const char *mnemonic = other_mnemonics[opc_nib_hi >> 1];
			if (opc_nib_hi & 1) {
				snprintf(buf, buf_len, "%s $%04X, X", lower_filter(mnemonic), imm16);
			} else {
				snprintf(buf, buf_len, "%s $%04X", lower_filter(mnemonic), imm16);
			}
		}
	} else {
		// should catch lo nibble in {3, 7, 0xb, 0xf}
		unknown = true;
	}

	if (unknown) {
		snprintf(buf, buf_len, "<illegal instruction (%08x)>", instr);
	}

	return true;
}