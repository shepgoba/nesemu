#include "disassembler.h"
#include <stdio.h>

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
			snprintf(buf, buf_len, "%s $%04X", branch_mnemonics[opc_nib_hi >> 1], pc + simm8);
		} else {
			if (opc_nib_hi < 8) {
				if (opc_nib_hi == 0) {
					snprintf(buf, buf_len, "BRK");
				} else if (opc_nib_hi == 2) {
					snprintf(buf, buf_len, "JSR $%04X", imm16);
				} else if (opc_nib_hi == 4) {
					snprintf(buf, buf_len, "RTI");
				} else if (opc_nib_hi == 6) {
					snprintf(buf, buf_len, "RTS");
				}
			} else if (opc_nib_hi == 8) {
				unknown = true;
			} else {
				if (opc_nib_hi == 0xa) {
					snprintf(buf, buf_len, "LDY #$%02X", imm8);
				} else if (opc_nib_hi == 0xc) {
					snprintf(buf, buf_len, "CPY #$%02X", imm8);
				} else if (opc_nib_hi == 0xe) {
					snprintf(buf, buf_len, "CPX #$%02X", imm8);
				}
			}
		}
	} else if (opc_nib_lo == 1) {
		const char *mnemonic = some_mnemonics[opc_nib_hi >> 1];
		if (opc_nib_hi & 1) {
			snprintf(buf, buf_len, "%s ($%02X), Y", mnemonic, imm8);
		} else {
			snprintf(buf, buf_len, "%s ($%02X, X)", mnemonic, imm8);
		}
	} else if (opc_nib_lo == 2) {
		if (opc_nib_hi == 0xa) {
			snprintf(buf, buf_len, "LDX #$%02X", imm8);
		} else {
			unknown = true;
		}
	} else if (opc_nib_lo == 4) {
		if (opc_nib_hi == 2) {
			snprintf(buf, buf_len, "BIT $%02X", imm8);
		} else if (opc_nib_hi == 8) {
			snprintf(buf, buf_len, "STY $%02X", imm8);
		} else if (opc_nib_hi == 9) {
			snprintf(buf, buf_len, "STY $%02X, X", imm8);
		} else if (opc_nib_hi == 0xa) {
			snprintf(buf, buf_len, "LDY $%02X", imm8);
		} else if (opc_nib_hi == 0xb) {
			snprintf(buf, buf_len, "LDY $%02X, X", imm8);
		} else if (opc_nib_hi == 0xc) {
			snprintf(buf, buf_len, "CPY $%02X", imm8);
		} else if (opc_nib_hi == 0xe) {
			snprintf(buf, buf_len, "CPX $%02X", imm8);
		} else {
			unknown = true;
		}
	} else if (opc_nib_lo == 5) {
		const char *mnemonic = some_mnemonics[opc_nib_hi >> 1];
		if (opc_nib_hi & 1) {
			snprintf(buf, buf_len, "%s $%02X, X", mnemonic, imm8);
		} else {
			snprintf(buf, buf_len, "%s $%02X", mnemonic, imm8);
		}
	} else if (opc_nib_lo == 6) {
		bool is_ldx_stx = (opc_nib_hi == 9 || opc_nib_hi == 0xb);
		char index_reg = is_ldx_stx ? 'Y' : 'X';
		const char *mnemonic = other_mnemonics[opc_nib_hi >> 1];
		if (opc_nib_hi & 1) {
			snprintf(buf, buf_len, "%s $%02X, %c", mnemonic, imm8, index_reg);
		} else {
			snprintf(buf, buf_len, "%s $%02X", mnemonic, imm8);
		}
	} else if (opc_nib_lo == 8) {
		snprintf(buf, buf_len, "%s", implied_mnemonics[opc_nib_hi]);
	} else if (opc_nib_lo == 9) {
		if (opc_nib_hi == 8) {
			unknown = true;
		} else {
			const char *mnemonic = some_mnemonics[opc_nib_hi >> 1];
			if (opc_nib_hi & 1) {
				snprintf(buf, buf_len, "%s $%04X, Y", mnemonic, imm16);
			} else {
				snprintf(buf, buf_len, "%s #$%02X", mnemonic, imm8);
			}
		}
	} else if (opc_nib_lo == 0xa) {
		if (opc_nib_hi < 8) {
			if (opc_nib_hi & 1) {
				unknown = true;
			} else {
				snprintf(buf, buf_len, "%s A", A_mnemonics[opc_nib_hi >> 1]);
			}
		} else {
			if (opc_nib_hi == 8) {
				snprintf(buf, buf_len, "TXA");
			} else if (opc_nib_hi == 9) {
				snprintf(buf, buf_len, "TXS");
			} else if (opc_nib_hi == 0xa) {
				snprintf(buf, buf_len, "TAX");
			} else if (opc_nib_hi == 0xb) {
				snprintf(buf, buf_len, "TSX");
			} else if (opc_nib_hi == 0xc) {
				snprintf(buf, buf_len, "DEX");
			} else if (opc_nib_hi == 0xe) {
				snprintf(buf, buf_len, "NOP");
			} else {
				unknown = true;
			}
		}
	} else if (opc_nib_lo == 0xc)  {
		if (opc_nib_hi == 2) {
			snprintf(buf, buf_len, "BIT $%04X", imm16);
		} else if (opc_nib_hi == 4) {
			snprintf(buf, buf_len, "JMP $%04X", imm16);
		} else if (opc_nib_hi == 6) {
			snprintf(buf, buf_len, "JMP ($%04X)", imm16);
		} else if (opc_nib_hi == 8) {
			snprintf(buf, buf_len, "STY $%04X", imm16);
		} else if (opc_nib_hi == 0xa) {
			snprintf(buf, buf_len, "LDY $%04X", imm16);
		} else if (opc_nib_hi == 0xb) {
			snprintf(buf, buf_len, "LDY $%04X, X", imm16);
		} else if (opc_nib_hi == 0xc) {
			snprintf(buf, buf_len, "CPY $%04X", imm16);
		} else if (opc_nib_hi == 0xe) {
			snprintf(buf, buf_len, "CPX $%04X", imm16);
		}
	} else if (opc_nib_lo == 0xd)  {
		const char *mnemonic = some_mnemonics[opc_nib_hi >> 1];
		if (opc_nib_hi & 1) {
			snprintf(buf, buf_len, "%s $%04X, X", mnemonic, imm16);
		} else {
			snprintf(buf, buf_len, "%s $%04X", mnemonic, imm16);
		}
	} else if (opc_nib_lo == 0xe)  {
		if (opc_nib_hi == 9) {
			unknown = true;
		} else if (opc_nib_hi == 0xb) {
		 	snprintf(buf, buf_len, "LDX $%04X, Y", imm16);
		} else {
			const char *mnemonic = other_mnemonics[opc_nib_hi >> 1];
			if (opc_nib_hi & 1) {
				snprintf(buf, buf_len, "%s $%04X, X", mnemonic, imm16);
			} else {
				snprintf(buf, buf_len, "%s $%04X", mnemonic, imm16);
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
