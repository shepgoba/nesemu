#ifndef CPU_INCLUDE
#define CPU_INCLUDE
#include <stdint.h>
#include <stdbool.h>
#include "utils.h"
#include "memory.h"
#include "ppu.h"

#define CPU_IMPLEMENT_ILLEGAL_OPCODES

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


typedef uint8_t mmc_type_t;

typedef struct __nes_cpu {
	// Program counter
	uint16_t pc;

	// Registers
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t sp;
	uint8_t sr;

	// Flags (pretty self explanatory)
	uint8_t flags[CPU_NUM_FLAGS];

	// Handles to other components
	// TODO: Abstract this out into a bus probably
	nes_memory_t *mem;
	nes_ppu_t *ppu;

	uint32_t wait_cycles;
	uint32_t total_cycles;

	mmc_type_t mmc_type;
	
	// Input key state management
	// TODO: Abstract this out into a separate component
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
#endif