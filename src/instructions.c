#include "cpu.h"
#include "memory.h"
#include "instructions.h"

#if defined(__GNUC__) || defined(__clang__)
	#define NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
	#define NOINLINE __declspec(noinline)
#else
	#define NOINLINE
#endif

/*

Common function name terminologies:
__* = internal wrapper function around common small operations
(eg, __has_page_overflow, __is_negative)

oper_* = operation
(eg. push, pop, branch)

instr_* = instruction implementation used in opcode table
(eg. ADC, AND)

_instr_* = internal instruction implementation with shared code
(eg. _instr_adc, _instr_and)

iinstr_* = illegal instruction

*/


static inline bool __has_page_overflow(uint16_t addr, uint8_t change)
{
	uint8_t addr_lo = addr & 0xff;
	uint16_t result = addr_lo + change;
	bool has_carry = (result & 0x100) == 0x100;
	
	return has_carry;
}

static inline bool __is_negative(uint8_t num)
{
	return (num & 0b10000000) >> 7;
}

static inline void oper_branch_offset(nes_cpu_t *cpu, int8_t offset)
{
	if (__has_page_overflow(cpu->pc, offset)) {
		cpu->wait_cycles += 2;
	} else {
		cpu->wait_cycles++;
	}
	cpu->pc += offset;
}

inline void set_flag(nes_cpu_t *cpu, uint8_t flag, int enable)
{
	cpu->flags[flag] = enable;
}

inline bool get_flag(nes_cpu_t *cpu, int flag)
{
	return cpu->flags[flag];
}

void oper_push_16(nes_cpu_t *cpu, uint16_t value)
{
	mem_write_16(cpu, (cpu->sp - 1) + 0x100, value);
	cpu->sp -= 2;
}

void oper_push_8(nes_cpu_t *cpu, uint8_t value)
{
	mem_write_8(cpu, cpu->sp + 0x100, value);
	cpu->sp--;
}

uint8_t oper_pop_8(nes_cpu_t *cpu)
{
	cpu->sp++;
	uint8_t result = mem_read_8(cpu, cpu->sp + 0x100);

	return result;
}

uint16_t oper_pop_16(nes_cpu_t *cpu)
{
	cpu->sp += 2;
	uint16_t result = mem_read_16(cpu, (cpu->sp - 1) + 0x100);

	return result;
}

static inline void __add_cpu_cycle_if_page_overflow(nes_cpu_t *cpu, uint16_t address, uint8_t offset)
{
	if (__has_page_overflow(address, offset)) {
		cpu->wait_cycles++;
	}
}

static inline void __set_value_abs(nes_cpu_t *cpu, uint16_t address, uint8_t value)
{
	mem_write_8(cpu, address, value);
}

static inline void __set_value_abs_x(nes_cpu_t *cpu, uint16_t address, uint8_t value)
{
	__add_cpu_cycle_if_page_overflow(cpu, address, cpu->x);
	mem_write_8(cpu, address + cpu->x, value);
}

static inline void __set_value_abs_y(nes_cpu_t *cpu, uint16_t address, uint8_t value)
{
	__add_cpu_cycle_if_page_overflow(cpu, address, cpu->y);
	mem_write_8(cpu, address + cpu->y, value);
}

static inline void __set_value_zpg(nes_cpu_t *cpu, uint8_t address, uint8_t value)
{
	mem_write_8(cpu, address, value);
}

static inline void __set_value_zpg_x(nes_cpu_t *cpu, uint8_t address, uint8_t value)
{
	__add_cpu_cycle_if_page_overflow(cpu, address, cpu->x);
	mem_write_8(cpu, (uint8_t)(address + cpu->x), value);
}

static inline void __set_value_zpg_y(nes_cpu_t *cpu, uint8_t address, uint8_t value)
{
	__add_cpu_cycle_if_page_overflow(cpu, address, cpu->y);
	mem_write_8(cpu, (uint8_t)(address + cpu->y), value);
}

static inline void __set_value_ind_y(nes_cpu_t *cpu, uint16_t address, uint8_t value)
{
	// relies on 8 bit overflow
	uint8_t lo_addr = mem_read_8(cpu, (uint8_t)address);
	uint8_t hi_addr = mem_read_8(cpu, (uint8_t)(address + 1));

	uint16_t addr = (hi_addr << 8) | lo_addr;
	
	mem_write_8(cpu, addr + cpu->y, value);
}

static inline void __set_value_x_ind(nes_cpu_t *cpu, uint16_t address, uint8_t value)
{
	// relies on 8 bit overflow
	uint8_t lo_addr = mem_read_8(cpu, (uint8_t)(address + cpu->x));
	uint8_t hi_addr = mem_read_8(cpu, (uint8_t)(address + cpu->x + 1));

	uint16_t addr = (hi_addr << 8) | lo_addr;

	mem_write_8(cpu, addr, value);
}

static inline void __set_value_ind(nes_cpu_t *cpu, uint16_t address, uint8_t value)
{
	uint16_t addr = mem_read_16(cpu, address);
	mem_write_8(cpu, addr, value);
}

static inline uint8_t __get_value_abs(nes_cpu_t *cpu, uint16_t address)
{
	return mem_read_8(cpu, address);
}

static inline uint8_t __get_value_abs_x(nes_cpu_t *cpu, uint16_t address, bool add_cycle_if_page_crossed)
{
	if (add_cycle_if_page_crossed) {
		if (__has_page_overflow(address, cpu->x)) {
			cpu->wait_cycles++;
		}
	}
	return mem_read_8(cpu, address + cpu->x);
}

static inline uint8_t __get_value_abs_y(nes_cpu_t *cpu, uint16_t address, bool add_cycle_if_page_crossed)
{
	if (add_cycle_if_page_crossed) {
		if (__has_page_overflow(address, cpu->y)) {
			cpu->wait_cycles++;
		}
	}
	return mem_read_8(cpu, address + cpu->y);
}

static inline uint8_t __get_value_zpg(nes_cpu_t *cpu, uint8_t address)
{
	return mem_read_8(cpu, address);
}

static inline uint8_t __get_value_zpg_x(nes_cpu_t *cpu, uint8_t address)
{
	// relies on 8 bit overflow
	return mem_read_8(cpu, (uint8_t)(address + cpu->x));
}

static inline uint8_t __get_value_zpg_y(nes_cpu_t *cpu, uint8_t address)
{
	// relies on 8 bit overflow
	return mem_read_8(cpu, (uint8_t)(address + cpu->y));
}

static inline uint8_t __get_value_ind_y(nes_cpu_t *cpu, uint8_t address, bool add_cycle_if_page_crossed)
{
	uint8_t lo_addr = mem_read_8(cpu, (uint8_t)address);
	uint8_t hi_addr = mem_read_8(cpu, (uint8_t)(address + 1));

	uint16_t addr = (hi_addr << 8) | lo_addr;

	uint16_t result = addr + cpu->y;

	if (add_cycle_if_page_crossed) {
		if (__has_page_overflow(addr, cpu->y)) {
			cpu->wait_cycles++;
		}
	}

	return mem_read_8(cpu, result);
}

static inline uint8_t __get_value_x_ind(nes_cpu_t *cpu, uint8_t address)
{
	uint8_t lo_addr = mem_read_8(cpu, (uint8_t)(address + cpu->x));
	uint8_t hi_addr = mem_read_8(cpu, (uint8_t)(address + cpu->x + 1));

	uint16_t addr = (hi_addr << 8) | lo_addr;
	return mem_read_8(cpu, addr);
}

static inline uint16_t __get_value_ind(nes_cpu_t *cpu, uint16_t address)
{
	uint8_t lo_addr = mem_read_8(cpu, address);
	uint8_t hi_addr = mem_read_8(cpu, (address & 0xff00) | (uint8_t)((address & 0xff) + 1));

	uint16_t addr = (hi_addr << 8) | lo_addr;

	return addr;
}

static inline uint8_t __get_imm8_from_opcode(uint32_t opcode)
{
	return (opcode >> 8) & 0xff;
}

static inline uint16_t __get_imm16_from_opcode(uint32_t opcode)
{
	return opcode & 0xffff;
}

static inline uint8_t __get_bit_8(uint8_t byte, int bit) 
{
	return (byte >> bit) & 1;
}

static inline uint8_t __get_bit_16(uint16_t word, int bit) 
{
	return (word >> bit) & 1;
}

/*
Generic implementations of many instructions
*/
NOINLINE
void _instr_ADC(nes_cpu_t *cpu, uint8_t num)
{
	int c_flag = get_flag(cpu, FLAG_C);

	uint16_t result = cpu->a + num + c_flag;

	set_flag(cpu, FLAG_Z, (uint8_t)result == 0);
	set_flag(cpu, FLAG_C, (result & 0x100) == 0x100);
	set_flag(cpu, FLAG_N, __is_negative(result));
	set_flag(cpu, FLAG_V, ((cpu->a ^ (uint8_t)result) & (num ^ (uint8_t)result) & 0x80) == 0x80);
	cpu->a = (uint8_t)result;
}

NOINLINE
void _instr_AND(nes_cpu_t *cpu, uint8_t num)
{
	cpu->a &= num;

	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}

NOINLINE
void _instr_CMP(nes_cpu_t *cpu, uint8_t num)
{
	uint8_t result = cpu->a - num;

	set_flag(cpu, FLAG_C, cpu->a >= num);
	set_flag(cpu, FLAG_Z, result == 0);
	set_flag(cpu, FLAG_N, __is_negative(result));
}

NOINLINE
void _instr_CPX(nes_cpu_t *cpu, uint8_t num)
{
	uint8_t result = cpu->x - num;

	set_flag(cpu, FLAG_C, cpu->x >= num);
	set_flag(cpu, FLAG_Z, result == 0);
	set_flag(cpu, FLAG_N, __is_negative(result));
}

NOINLINE
void _instr_CPY(nes_cpu_t *cpu, uint8_t num)
{
	uint8_t result = cpu->y - num;

	set_flag(cpu, FLAG_C, cpu->y >= num);
	set_flag(cpu, FLAG_Z, result == 0);
	set_flag(cpu, FLAG_N, __is_negative(result));
}

NOINLINE
void _instr_EOR(nes_cpu_t *cpu, uint8_t num)
{
	cpu->a ^= num;

	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}

NOINLINE
void _instr_LDA(nes_cpu_t *cpu, uint8_t num)
{
	cpu->a = num;

	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}

NOINLINE
void _instr_LDX(nes_cpu_t *cpu, uint8_t num)
{
	cpu->x = num;

	set_flag(cpu, FLAG_Z, cpu->x == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->x));
}

NOINLINE
void _instr_LDY(nes_cpu_t *cpu, uint8_t num)
{
	cpu->y = num;

	set_flag(cpu, FLAG_Z, cpu->y == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->y));
}

NOINLINE
void _instr_ORA(nes_cpu_t *cpu, uint8_t num)
{
	cpu->a |= num;

	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}

/*
Generic instructions end
*/

void instr_ADC_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	_instr_ADC(cpu, num);
}

void instr_ADC_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, true);
	_instr_ADC(cpu, num);
}

void instr_ADC_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, true);
	_instr_ADC(cpu, num);
}

void instr_ADC_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t imm8 = __get_imm8_from_opcode(instr);
	_instr_ADC(cpu, imm8);
}

void instr_ADC_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, true);
	_instr_ADC(cpu, num);
}

void instr_ADC_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);
	_instr_ADC(cpu, num);
}

void instr_ADC_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	_instr_ADC(cpu, num);
}

void instr_ADC_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);
	_instr_ADC(cpu, num);
}

void instr_AND_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	int num = __get_value_abs(cpu, addr);
	_instr_AND(cpu, num);
}

void instr_AND_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	int num = __get_value_abs_x(cpu, addr, true);
	_instr_AND(cpu, num);
}

void instr_AND_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	int num = __get_value_abs_y(cpu, addr, true);
	_instr_AND(cpu, num);
}

void instr_AND_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t num = __get_imm8_from_opcode(instr);
	_instr_AND(cpu, num);
}

void instr_AND_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	int num = __get_value_ind_y(cpu, addr, true);
	_instr_AND(cpu, num);
}

void instr_AND_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	int num = __get_value_x_ind(cpu, addr);
	_instr_AND(cpu, num);
}

void instr_AND_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	int num = __get_value_zpg(cpu, addr);
	_instr_AND(cpu, num);
}

void instr_AND_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	int num = __get_value_zpg_x(cpu, addr);
	_instr_AND(cpu, num);
}

void instr_ASL_A(nes_cpu_t *cpu, uint32_t instr)
{
	int status = __get_bit_8(cpu->a, 7);
	set_flag(cpu, FLAG_C, status);

	cpu->a <<= 1;
	
	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}

void instr_ASL_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	int status = __get_bit_8(num, 7);

	set_flag(cpu, FLAG_C, status);
	num <<= 1;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs(cpu, addr, num);
}

void instr_ASL_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);
	int status = __get_bit_8(num, 7);

	set_flag(cpu, FLAG_C, status);
	num <<= 1;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs_x(cpu, addr, num);
}

void instr_ASL_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	int status = __get_bit_8(num, 7);

	set_flag(cpu, FLAG_C, status);
	num <<= 1;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg(cpu, addr, num);
}

void instr_ASL_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);
	int status = __get_bit_8(num, 7);

	set_flag(cpu, FLAG_C, status);
	num <<= 1;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg_x(cpu, addr, num);
}

void instr_BCC(nes_cpu_t *cpu, uint32_t instr)
{
	bool flag = get_flag(cpu, FLAG_C);

	if (!flag) {
		oper_branch_offset(cpu, (int8_t)__get_imm8_from_opcode(instr));
	}
}

void instr_BCS(nes_cpu_t *cpu, uint32_t instr)
{
	bool flag = get_flag(cpu, FLAG_C);

	if (flag) {
		oper_branch_offset(cpu, (int8_t)__get_imm8_from_opcode(instr));
	}
}

void instr_BEQ(nes_cpu_t *cpu, uint32_t instr)
{
	bool flag = get_flag(cpu, FLAG_Z);

	if (flag) {
		oper_branch_offset(cpu, (int8_t)__get_imm8_from_opcode(instr));
	}
}


void instr_BIT_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	set_flag(cpu, FLAG_Z, (cpu->a & num) == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));
	set_flag(cpu, FLAG_V, __get_bit_8(num, 6));
}

void instr_BIT_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	set_flag(cpu, FLAG_Z, (cpu->a & num) == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));
	set_flag(cpu, FLAG_V, __get_bit_8(num, 6));
}

void instr_BMI(nes_cpu_t *cpu, uint32_t instr)
{
	bool flag = get_flag(cpu, FLAG_N);

	if (flag) {
		oper_branch_offset(cpu, (int8_t)__get_imm8_from_opcode(instr));
	}
}

void instr_BNE(nes_cpu_t *cpu, uint32_t instr)
{
	bool flag = get_flag(cpu, FLAG_Z);

	if (!flag) {
		oper_branch_offset(cpu, (int8_t)__get_imm8_from_opcode(instr));
	}
}

void instr_BPL(nes_cpu_t *cpu, uint32_t instr)
{
	bool flag = get_flag(cpu, FLAG_N);

	if (!flag) {
		oper_branch_offset(cpu, (int8_t)__get_imm8_from_opcode(instr));
	}
}

void instr_BRK(nes_cpu_t *cpu, uint32_t instr)
{
	oper_push_16(cpu, cpu->pc);

	/*
	always set bit 5 in the SR copy, 
	bit 4 if from an instruction (which it is in this case)
	*/
	uint8_t copy = cpu_get_sr(cpu) | 0b00110000;
	oper_push_8(cpu, copy);

	set_flag(cpu, FLAG_I, 1);
}


void instr_BVC(nes_cpu_t *cpu, uint32_t instr)
{
	bool flag = get_flag(cpu, FLAG_V);

	if (!flag) {
		oper_branch_offset(cpu, (int8_t)__get_imm8_from_opcode(instr));
	}
}

void instr_BVS(nes_cpu_t *cpu, uint32_t instr)
{
	bool flag = get_flag(cpu, FLAG_V);

	if (flag) {
		oper_branch_offset(cpu, (int8_t)__get_imm8_from_opcode(instr));
	}
}

void instr_CLC(nes_cpu_t *cpu, uint32_t instr)
{
	set_flag(cpu, FLAG_C, 0);
}

void instr_CLD(nes_cpu_t *cpu, uint32_t instr)
{
	set_flag(cpu, FLAG_D, 0);
}

void instr_CLI(nes_cpu_t *cpu, uint32_t instr)
{
	set_flag(cpu, FLAG_I, 0);
}

void instr_CLV(nes_cpu_t *cpu, uint32_t instr)
{
	set_flag(cpu, FLAG_V, 0);
}

void instr_CMP_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	_instr_CMP(cpu, num);
}

void instr_CMP_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, true);
	_instr_CMP(cpu, num);
}

void instr_CMP_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, true);
	_instr_CMP(cpu, num);
}

void instr_CMP_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t num = __get_imm8_from_opcode(instr);
	_instr_CMP(cpu, num);
}

void instr_CMP_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, true);
	_instr_CMP(cpu, num);
}

void instr_CMP_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);
	_instr_CMP(cpu, num);
}

void instr_CMP_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	_instr_CMP(cpu, num);
}

void instr_CMP_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);
	_instr_CMP(cpu, num);
}

void instr_CPX_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	_instr_CPX(cpu, num);
}

void instr_CPX_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t num = __get_imm8_from_opcode(instr);
	_instr_CPX(cpu, num);
}

void instr_CPX_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	_instr_CPX(cpu, num);
}

void instr_CPY_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	_instr_CPY(cpu, num);
}

void instr_CPY_imm(nes_cpu_t *cpu, uint32_t instr)
{	
	uint8_t num = __get_imm8_from_opcode(instr);
	_instr_CPY(cpu, num);
}

void instr_CPY_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	_instr_CPY(cpu, num);
}

void instr_DEC_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	num--;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs(cpu, addr, num);
}

void instr_DEC_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	num--;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs_x(cpu, addr, num);
}

void instr_DEC_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	num--;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg(cpu, addr, num);
}

void instr_DEC_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	num--;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg_x(cpu, addr, num);
}

void instr_DEX(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->x--;
	set_flag(cpu, FLAG_Z, cpu->x == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->x));
}

void instr_DEY(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->y--;
	set_flag(cpu, FLAG_Z, cpu->y == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->y));
}

void instr_EOR_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	_instr_EOR(cpu, num);
}

void instr_EOR_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, true);
	_instr_EOR(cpu, num);
}

void instr_EOR_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, true);
	_instr_EOR(cpu, num);
}

void instr_EOR_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t imm8 = __get_imm8_from_opcode(instr);
	_instr_EOR(cpu, imm8);
}

void instr_EOR_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, true);
	_instr_EOR(cpu, num);
}

void instr_EOR_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);
	_instr_EOR(cpu, num);
}

void instr_EOR_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	_instr_EOR(cpu, num);
}

void instr_EOR_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);
	_instr_EOR(cpu, num);
}

void instr_INC_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	num++;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs(cpu, addr, num);
}

void instr_INC_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	num++;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs_x(cpu, addr, num);
}

void instr_INC_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	num++;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg(cpu, addr, num);
}

void instr_INC_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	num++;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg_x(cpu, addr, num);
}

void instr_INX(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->x++;
	set_flag(cpu, FLAG_Z, cpu->x == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->x));
}

void instr_INY(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->y++;
	set_flag(cpu, FLAG_Z, cpu->y == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->y));
}

void instr_JMP_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	cpu->pc = addr;
}

void instr_JMP_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint16_t deref_addr = __get_value_ind(cpu, addr);
	cpu->pc = deref_addr;
}

void instr_JSR(nes_cpu_t *cpu, uint32_t instr)
{
	oper_push_16(cpu, cpu->pc - 1);
	uint16_t addr = __get_imm16_from_opcode(instr);
	cpu->pc = addr;
}


void instr_LDA_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	_instr_LDA(cpu, num);
}

void instr_LDA_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, true);
	_instr_LDA(cpu, num);
}

void instr_LDA_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, true);
	_instr_LDA(cpu, num);
}

void instr_LDA_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t imm8 = __get_imm8_from_opcode(instr);
	_instr_LDA(cpu, imm8);
}

void instr_LDA_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, true);
	_instr_LDA(cpu, num);
}

void instr_LDA_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);
	_instr_LDA(cpu, num);
}

void instr_LDA_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	_instr_LDA(cpu, num);
}

void instr_LDA_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);
	_instr_LDA(cpu, num);
}

void instr_LDX_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	_instr_LDX(cpu, num);
}

void instr_LDX_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, true);
	_instr_LDX(cpu, num);
}

void instr_LDX_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t imm8 = __get_imm8_from_opcode(instr);
	_instr_LDX(cpu, imm8);
}

void instr_LDX_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	_instr_LDX(cpu, num);
}

void instr_LDX_zpg_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_y(cpu, addr);
	_instr_LDX(cpu, num);
}

void instr_LDY_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	_instr_LDY(cpu, num);
}

void instr_LDY_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, true);
	_instr_LDY(cpu, num);
}

void instr_LDY_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t imm8 = __get_imm8_from_opcode(instr);
	_instr_LDY(cpu, imm8);
}

void instr_LDY_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	_instr_LDY(cpu, num);
}

void instr_LDY_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);
	_instr_LDY(cpu, num);
}

void instr_LSR_A(nes_cpu_t *cpu, uint32_t instr)
{
	set_flag(cpu, FLAG_C, cpu->a & 1);

	cpu->a >>= 1;

	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, 0);
}

void instr_LSR_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, 0);

	__set_value_abs(cpu, addr, num);
}

void instr_LSR_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, 0);

	__set_value_abs_x(cpu, addr, num);
}

void instr_LSR_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, 0);

	__set_value_zpg(cpu, addr, num);
}

void instr_LSR_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, 0);

	__set_value_zpg_x(cpu, addr, num);
}

void instr_NOP(nes_cpu_t *cpu, uint32_t instr)
{
	// do nothing
}

void instr_ORA_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);
	_instr_ORA(cpu, num);
}

void instr_ORA_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, true);
	_instr_ORA(cpu, num);
}

void instr_ORA_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, true);
	_instr_ORA(cpu, num);
}


void instr_ORA_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t imm8 = __get_imm8_from_opcode(instr);
	_instr_ORA(cpu, imm8);
}

void instr_ORA_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, true);
	_instr_ORA(cpu, num);
}

void instr_ORA_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);
	_instr_ORA(cpu, num);
}

void instr_ORA_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);
	_instr_ORA(cpu, num);
}

void instr_ORA_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);
	_instr_ORA(cpu, num);
}

void instr_PHA(nes_cpu_t *cpu, uint32_t instr)
{
	oper_push_8(cpu, cpu->a);
}

void instr_PHP(nes_cpu_t *cpu, uint32_t instr)
{
	// always set bit 5 in the SR copy, bit 4 if from an instruction
	uint8_t copy = cpu_get_sr(cpu) | 0b00110000;
	oper_push_8(cpu, copy);
}

void instr_PLA(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->a = oper_pop_8(cpu);
	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}

void instr_PLP(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t dat = oper_pop_8(cpu);
	cpu_set_sr(cpu, dat);
}

void instr_ROL_A(nes_cpu_t *cpu, uint32_t instr)
{
	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, __get_bit_8(cpu->a, 7));

	cpu->a = (cpu->a << 1) | c_flag;

	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}

void instr_ROL_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs(cpu, addr, num);
}

void instr_ROL_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C,  __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs_x(cpu, addr, num);
}

void instr_ROL_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C,  __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg(cpu, addr, num);
}

void instr_ROL_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C,  __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg_x(cpu, addr, num);
}

void instr_ROR_A(nes_cpu_t *cpu, uint32_t instr)
{
	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, cpu->a & 1);

	cpu->a = (cpu->a >> 1) | (c_flag << 7);

	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}

void instr_ROR_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs(cpu, addr, num);
}

void instr_ROR_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_abs_x(cpu, addr, num);
}

void instr_ROR_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg(cpu, addr, num);
}

void instr_ROR_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	set_flag(cpu, FLAG_Z, num == 0);
	set_flag(cpu, FLAG_N, __is_negative(num));

	__set_value_zpg_x(cpu, addr, num);
}

void instr_RTI(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t new_sr = oper_pop_8(cpu);
	cpu_set_sr(cpu, new_sr);

	cpu->pc = oper_pop_16(cpu);
}

void instr_RTS(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->pc = oper_pop_16(cpu) + 1;
}

void instr_SBC_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = ~__get_value_abs(cpu, addr);
	_instr_ADC(cpu, num);
}

void instr_SBC_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = ~__get_value_abs_x(cpu, addr, true);
	_instr_ADC(cpu, num);
}
 
void instr_SBC_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = ~__get_value_abs_y(cpu, addr, true);
	_instr_ADC(cpu, num);
}

void instr_SBC_imm(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t imm8 = ~__get_imm8_from_opcode(instr);
	_instr_ADC(cpu, imm8);
}

void instr_SBC_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = ~__get_value_ind_y(cpu, addr, true);
	_instr_ADC(cpu, num);
}

void instr_SBC_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = ~__get_value_x_ind(cpu, addr);
	_instr_ADC(cpu, num);
}

void instr_SBC_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = ~__get_value_zpg(cpu, addr);
	_instr_ADC(cpu, num);
}

void instr_SBC_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = ~__get_value_zpg_x(cpu, addr);
	_instr_ADC(cpu, num);
}

void instr_SEC(nes_cpu_t *cpu, uint32_t instr)
{
	set_flag(cpu, FLAG_C, 1);
}

void instr_SED(nes_cpu_t *cpu, uint32_t instr)
{
	set_flag(cpu, FLAG_D, 1);
}

void instr_SEI(nes_cpu_t *cpu, uint32_t instr)
{
	set_flag(cpu, FLAG_I, 1);
}

void instr_STA_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	__set_value_abs(cpu, addr, cpu->a);
}

void instr_STA_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	__set_value_abs_x(cpu, addr, cpu->a);
}

void instr_STA_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	__set_value_abs_y(cpu, addr, cpu->a);
}

void instr_STA_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	__set_value_ind_y(cpu, addr, cpu->a);
}

void instr_STA_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	__set_value_x_ind(cpu, addr, cpu->a);
}

void instr_STA_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	__set_value_zpg(cpu, addr, cpu->a);
}

void instr_STA_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	__set_value_zpg_x(cpu, addr, cpu->a);
}

void instr_STX_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	__set_value_abs(cpu, addr, cpu->x);
}

void instr_STX_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	__set_value_zpg(cpu, addr, cpu->x);
}

void instr_STX_zpg_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	__set_value_zpg_y(cpu, addr, cpu->x);
}

void instr_STY_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	__set_value_abs(cpu, addr, cpu->y);
}

void instr_STY_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	__set_value_zpg(cpu, addr, cpu->y);
}

void instr_STY_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	__set_value_zpg_x(cpu, addr, cpu->y);
}

void instr_TAX(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->x = cpu->a;
	set_flag(cpu, FLAG_Z, cpu->x == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->x));
}

void instr_TAY(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->y = cpu->a;
	set_flag(cpu, FLAG_Z, cpu->y == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->y));
}

void instr_TSX(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->x = cpu->sp;
	set_flag(cpu, FLAG_Z, cpu->x == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->x));
}

void instr_TXA(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->a = cpu->x;
	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}

void instr_TXS(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->sp = cpu->x;
}

void instr_TYA(nes_cpu_t *cpu, uint32_t instr)
{
	cpu->a = cpu->y;
	set_flag(cpu, FLAG_Z, cpu->a == 0);
	set_flag(cpu, FLAG_N, __is_negative(cpu->a));
}


#ifdef CPU_IMPLEMENT_ILLEGAL_OPCODES
//illegal opcodes
void iinstr_AHX_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	// unstable
}

void iinstr_AHX_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	// unstable
}

void iinstr_ALR_imm(nes_cpu_t *cpu, uint32_t instr)
{
	// TODO
}

void iinstr_ANC_imm(nes_cpu_t *cpu, uint32_t instr)
{
	instr_AND_imm(cpu, instr);
	set_flag(cpu, FLAG_C, __is_negative(cpu->a));
}

void iinstr_ARR_imm(nes_cpu_t *cpu, uint32_t instr)
{
	// TODO
}

void iinstr_AXS_imm(nes_cpu_t *cpu, uint32_t instr)
{
	// TODO
}

void iinstr_DCP_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	num--;
	__set_value_abs(cpu, addr, num);

	_instr_CMP(cpu, num);
}

void iinstr_DCP_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	num--;
	__set_value_abs_x(cpu, addr, num);

	_instr_CMP(cpu, num);
}

void iinstr_DCP_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, false);

	num--;
	__set_value_abs_y(cpu, addr, num);

	_instr_CMP(cpu, num);
}

void iinstr_DCP_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, false);

	num--;
	__set_value_ind_y(cpu, addr, num);

	_instr_CMP(cpu, num);
}

void iinstr_DCP_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);

	num--;
	__set_value_x_ind(cpu, addr, num);

	_instr_CMP(cpu, num);
}

void iinstr_DCP_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	num--;
	__set_value_zpg(cpu, addr, num);

	_instr_CMP(cpu, num);
}

void iinstr_DCP_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	num--;
	__set_value_zpg_x(cpu, addr, num);

	_instr_CMP(cpu, num);
}

void iinstr_ISC_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	num++;
	__set_value_abs(cpu, addr, num);

	_instr_ADC(cpu, ~num);
}

void iinstr_ISC_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	num++;
	__set_value_abs_x(cpu, addr, num);

	_instr_ADC(cpu, ~num);
}

void iinstr_ISC_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, false);

	num++;
	__set_value_abs_y(cpu, addr, num);

	_instr_ADC(cpu, ~num);
}

void iinstr_ISC_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, false);

	num++;
	__set_value_ind_y(cpu, addr, num);

	_instr_ADC(cpu, ~num);
}

void iinstr_ISC_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);

	num++;
	__set_value_x_ind(cpu, addr, num);

	_instr_ADC(cpu, ~num);
}

void iinstr_ISC_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	num++;
	__set_value_zpg(cpu, addr, num);

	_instr_ADC(cpu, ~num);
}

void iinstr_ISC_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	num++;
	__set_value_zpg_x(cpu, addr, num);

	_instr_ADC(cpu, ~num);
}

void iinstr_KILL(nes_cpu_t *cpu, uint32_t instr)
{
	// freezes cpu so just do nothing
}

void iinstr_LAS_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	// TODO
}

void iinstr_LAX_abs(nes_cpu_t *cpu, uint32_t instr)
{
	instr_LDA_abs(cpu, instr);
	cpu->x = cpu->a;
}

void iinstr_LAX_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	instr_LDA_abs_y(cpu, instr);
	cpu->x = cpu->a;
}

void iinstr_LAX_imm(nes_cpu_t *cpu, uint32_t instr)
{
	instr_LDA_imm(cpu, instr);
	cpu->x = cpu->a;
}

void iinstr_LAX_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	instr_LDA_ind_y(cpu, instr);
	cpu->x = cpu->a;
}

void iinstr_LAX_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	instr_LDA_x_ind(cpu, instr);
	cpu->x = cpu->a;
}

void iinstr_LAX_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	instr_LDA_zpg(cpu, instr);
	cpu->x = cpu->a;
}

void iinstr_LAX_zpg_y(nes_cpu_t *cpu, uint32_t instr)
{
	// copy to X first to prevent code duplication
	instr_LDX_zpg_y(cpu, instr);
	cpu->a = cpu->x;
}

void iinstr_NOP(nes_cpu_t *cpu, uint32_t instr)
{
	// do nothing
}

void iinstr_NOP_abs(nes_cpu_t *cpu, uint32_t instr)
{
	// do nothing
}

void iinstr_NOP_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	// do nothing
}

void iinstr_NOP_imm(nes_cpu_t *cpu, uint32_t instr)
{
	// do nothing
}

void iinstr_NOP_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	// do nothing
}

void iinstr_NOP_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	// do nothing
}

void iinstr_RLA_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	__set_value_abs(cpu, addr, num);
	_instr_AND(cpu, num);
}

void iinstr_RLA_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	__set_value_abs_x(cpu, addr, num);
	_instr_AND(cpu, num);
}

void iinstr_RLA_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, false);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	__set_value_abs_y(cpu, addr, num);
	_instr_AND(cpu, num);
}

void iinstr_RLA_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, false);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	__set_value_ind_y(cpu, addr, num);
	_instr_AND(cpu, num);
}

void iinstr_RLA_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	__set_value_x_ind(cpu, addr, num);
	_instr_AND(cpu, num);
}

void iinstr_RLA_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	__set_value_zpg(cpu, addr, num);
	_instr_AND(cpu, num);
}

void iinstr_RLA_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, __get_bit_8(num, 7));

	num = (num << 1) | c_flag;

	__set_value_zpg_x(cpu, addr, num);
	_instr_AND(cpu, num);
}

void iinstr_RRA_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	__set_value_abs(cpu, addr, num);
	_instr_ADC(cpu, num);
}

void iinstr_RRA_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	__set_value_abs_x(cpu, addr, num);
	_instr_ADC(cpu, num);
}

void iinstr_RRA_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, false);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	__set_value_abs_y(cpu, addr, num);
	_instr_ADC(cpu, num);
}

void iinstr_RRA_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, false);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	__set_value_ind_y(cpu, addr, num);
	_instr_ADC(cpu, num);
}

void iinstr_RRA_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	__set_value_x_ind(cpu, addr, num);
	_instr_ADC(cpu, num);
}

void iinstr_RRA_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	__set_value_zpg(cpu, addr, num);
	_instr_ADC(cpu, num);
}

void iinstr_RRA_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	int c_flag = get_flag(cpu, FLAG_C);
	set_flag(cpu, FLAG_C, num & 1);

	num = (num >> 1) | (c_flag << 7);

	__set_value_zpg_x(cpu, addr, num);
	_instr_ADC(cpu, num);
}

void iinstr_SAX_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = cpu->a & cpu->x;

	__set_value_abs(cpu, addr, num);
}

void iinstr_SAX_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = cpu->a & cpu->x;

	__set_value_x_ind(cpu, addr, num);
}

void iinstr_SAX_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = cpu->a & cpu->x;

	__set_value_zpg(cpu, addr, num);
}

void iinstr_SAX_zpg_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = cpu->a & cpu->x;

	__set_value_zpg_y(cpu, addr, num);
}

void iinstr_SBC_imm(nes_cpu_t *cpu, uint32_t instr)
{
	instr_SBC_imm(cpu, instr);
}

void iinstr_SHX_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	// unstable
}

void iinstr_SHY_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	// unstable
}

void iinstr_SLO_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	int status = __get_bit_8(num, 7);
	set_flag(cpu, FLAG_C, status);

	num <<= 1;
	__set_value_abs(cpu, addr, num);
	
	_instr_ORA(cpu, num);
}

void iinstr_SLO_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	int status = __get_bit_8(num, 7);
	set_flag(cpu, FLAG_C, status);

	num <<= 1;
	__set_value_abs_x(cpu, addr, num);
	_instr_ORA(cpu, num);
}

void iinstr_SLO_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, false);

	int status = __get_bit_8(num, 7);
	set_flag(cpu, FLAG_C, status);

	num <<= 1;
	__set_value_abs_y(cpu, addr, num);
	_instr_ORA(cpu, num);
}

void iinstr_SLO_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, false);

	int status = __get_bit_8(num, 7);
	set_flag(cpu, FLAG_C, status);

	num <<= 1;
	__set_value_ind_y(cpu, addr, num);
	_instr_ORA(cpu, num);
}

void iinstr_SLO_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);

	int status = __get_bit_8(num, 7);
	set_flag(cpu, FLAG_C, status);

	num <<= 1;
	__set_value_x_ind(cpu, addr, num);
	_instr_ORA(cpu, num);
}

void iinstr_SLO_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	int status = __get_bit_8(num, 7);
	set_flag(cpu, FLAG_C, status);

	num <<= 1;
	__set_value_zpg(cpu, addr, num);
	_instr_ORA(cpu, num);
}

void iinstr_SLO_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	int status = __get_bit_8(num, 7);
	set_flag(cpu, FLAG_C, status);

	num <<= 1;
	__set_value_zpg_x(cpu, addr, num);
	_instr_ORA(cpu, num);
}

void iinstr_SRE_abs(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs(cpu, addr);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;
	__set_value_abs(cpu, addr, num);
	_instr_EOR(cpu, num);
}

void iinstr_SRE_abs_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_x(cpu, addr, false);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;
	__set_value_abs_x(cpu, addr, num);
	_instr_EOR(cpu, num);
}

void iinstr_SRE_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint16_t addr = __get_imm16_from_opcode(instr);
	uint8_t num = __get_value_abs_y(cpu, addr, false);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;
	__set_value_abs_y(cpu, addr, num);
	_instr_EOR(cpu, num);
}

void iinstr_SRE_ind_y(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_ind_y(cpu, addr, false);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;
	__set_value_ind_y(cpu, addr, num);
	_instr_EOR(cpu, num);
}

void iinstr_SRE_x_ind(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_x_ind(cpu, addr);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;
	__set_value_x_ind(cpu, addr, num);
	_instr_EOR(cpu, num);
}

void iinstr_SRE_zpg(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg(cpu, addr);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;
	__set_value_zpg(cpu, addr, num);
	_instr_EOR(cpu, num);
}

void iinstr_SRE_zpg_x(nes_cpu_t *cpu, uint32_t instr)
{
	uint8_t addr = __get_imm8_from_opcode(instr);
	uint8_t num = __get_value_zpg_x(cpu, addr);

	set_flag(cpu, FLAG_C, num & 1);

	num >>= 1;
	__set_value_zpg_x(cpu, addr, num);
	_instr_EOR(cpu, num);
}

void iinstr_TAS_abs_y(nes_cpu_t *cpu, uint32_t instr)
{
	// unstable
}

void iinstr_XAA_imm(nes_cpu_t *cpu, uint32_t instr)
{
	// unstable
}
#endif
