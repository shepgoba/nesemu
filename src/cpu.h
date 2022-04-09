#ifndef CPU_INCLUDE
#define CPU_INCLUDE
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "memory.h"
#include "ppu.h"

//#define CPU_IMPLEMENT_ILLEGAL_OPCODES

#define NMI_INTERRUPT_VECTOR_ADDR 0xFFFA
#define RESET_VECTOR_ADDR 0xFFFC
#define IRQ_INTERRUPT_VECTOR_ADDR 0xFFFE

#define MASTER_CLOCKS_PER_CPU_CLOCK 12

#define FLAG_N 7
#define FLAG_V 6
#define FLAG_B 4
#define FLAG_D 3
#define FLAG_I 2
#define FLAG_Z 1
#define FLAG_C 0



#define CPU_NUM_FLAGS 8


typedef struct __mmc1 {
	int shift_register;
	int shift_writes;
} mmc1_t;

typedef struct __nes_cpu {
	uint16_t pc;

	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t sp;
	uint8_t sr;

	uint8_t flags[CPU_NUM_FLAGS];

	bool use_mmc1;
	mmc1_t mmc1;

	nes_memory_t *mem;
	nes_ppu_t *ppu;

	uint32_t wait_cycles;
	uint32_t total_cycles;
	
	uint8_t key_state;
	bool strobe_keys;
	int strobe_keys_write_no;
} nes_cpu_t;

void cpu_init(nes_cpu_t *, nes_memory_t *, nes_ppu_t *);
void cpu_reset(nes_cpu_t *);
void cpu_run_cycle(nes_cpu_t *);
void cpu_check_interrupts(nes_cpu_t *);
uint32_t cpu_fetch_instruction(nes_cpu_t *);
void cpu_execute_instruction(nes_cpu_t *, uint32_t);
void cpu_update_registers(nes_cpu_t *, uint8_t);
void cpu_cleanup(nes_cpu_t *cpu);

uint8_t cpu_get_sr(nes_cpu_t *);
void cpu_set_sr(nes_cpu_t *, uint8_t);

uint8_t cpu_get_flag_c(nes_cpu_t *);
uint8_t cpu_get_flag_z(nes_cpu_t *);
uint8_t cpu_get_flag_i(nes_cpu_t *);
uint8_t cpu_get_flag_d(nes_cpu_t *);
uint8_t cpu_get_flag_b(nes_cpu_t *);
uint8_t cpu_get_flag_v(nes_cpu_t *);
uint8_t cpu_get_flag_n(nes_cpu_t *);

//inline uint8_t __get_imm8_from_opcode(uint32_t);
//inline uint16_t __get_imm16_from_opcode(uint32_t);

#endif