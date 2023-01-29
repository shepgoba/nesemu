#ifndef MEMORY_INCLUDE
#define MEMORY_INCLUDE
#include <stdint.h>
#include <stdbool.h>

#define ADDRESS_SPACE_SIZE_6502 0x10000
#define ADDRESS_SPACE_SIZE_2C02 0x4000

#define MASTER_CLOCK_PER_SEC 21477272

#define RESET_VECTOR_ADDR 0xFFFC

#define CONTROLLER_IO_ADDR 0x4016

typedef struct __nes_cpu nes_cpu_t;

typedef struct __nes_memory {
    uint8_t *data;
} nes_memory_t;

typedef struct __nes_vmemory {
    uint8_t *data;
} nes_vmemory_t;

bool memory_init(nes_memory_t *);
void memory_cleanup(nes_memory_t *);

bool vmemory_init(nes_vmemory_t *);
void vmemory_cleanup(nes_vmemory_t *);


uint8_t mem_read_8(nes_cpu_t *, uint16_t);
uint16_t mem_read_16(nes_cpu_t *, uint16_t);
void mem_write_8(nes_cpu_t *cpu, uint16_t address, uint8_t value);
void mem_write_16(nes_cpu_t *cpu, uint16_t address, uint16_t value);
#endif