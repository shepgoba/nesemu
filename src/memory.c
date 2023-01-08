#include "memory.h"
#include "cpu.h"
#include "ppu.h"

int memory_init(nes_memory_t *memory)
{
	memory->data = malloc(ADDRESS_SPACE_SIZE_6502);
	if (!memory->data) {
		printf("error allocating memory!\n");
		return 0;
	}
	memset(memory->data, 0, ADDRESS_SPACE_SIZE_6502);

	return 1;
}


void memory_cleanup(nes_memory_t *memory)
{
	free(memory->data);
}

int vmemory_init(nes_vmemory_t *vmemory)
{
	vmemory->data = malloc(ADDRESS_SPACE_SIZE_2C02);
	if (!vmemory->data) {
		printf("error allocating video memory!\n");
		return 0;
	}
	memset(vmemory->data, 0, ADDRESS_SPACE_SIZE_2C02);
	return 1;
}


void vmemory_cleanup(nes_vmemory_t *vmemory)
{
	free(vmemory->data);
}


void trol_set_bit(uint8_t *byte, int bit, int status)
{
	*byte ^= (-status ^ *byte) & (1UL << bit);
}

__attribute__((noinline))
void mem_write_8(nes_cpu_t *cpu, uint16_t address, uint8_t value)
{
	uint8_t *mem = cpu->mem->data;
	nes_ppu_t *ppu = cpu->ppu;

	if (cpu->use_mmc1) {
		if (address >= 0x8000 && address <= 0xffff) {
			if (value & 0b10000000) {
				cpu->mmc1.shift_register = 0;
				cpu->mmc1.shift_writes = 0;
			} else {
				if (cpu->mmc1.shift_writes == 4) {
					int bit = value & 1;
					//trol_set_bit(&cpu->mmc1.shift_register, 3 - cpu->mmc1.shift_writes, bit);


					int idx = cpu->mmc1.shift_register & 0xf;
					printf("idx is %x\n", idx);

					//memcpy(cpu->mem->data + 0xC000, (uint8_t *)(cpu->rom_ptr + idx % 8 * 0x4000), 0x4000);

					cpu->mmc1.shift_register = 0;
					cpu->mmc1.shift_writes = 0;
				} else {
					int bit = value & 1;

					//trol_set_bit(&cpu->mmc1.shift_register, 3 - cpu->mmc1.shift_writes, bit);// <<= (value & 1);

					cpu->mmc1.shift_writes++;
					printf("pc:%04x, %i\n", cpu->pc, bit);
				}

			}
			return;
		}
	}

	switch (address) {
		case PPUCTRL_ADDR: {
			//printf("we writing to PPUCTRL!\n");
			ppu->PPUCTRL = value;

			ppu->nametable_base = 0x2000 + (value & 3) * 0x400;
			ppu->sprites8x16 = get_bit(value, 5);
			ppu->background_tiledata_base = get_bit(value, 4) ? 0x1000 : 0x0000;
			ppu->sprite_tiledata_base = get_bit(value, 3) ? 0x1000 : 0x0000;
			ppu->PPUADDR_increment_amount = get_bit(value, 2) ? 0x20 : 0x1;
			ppu->NMI_output = get_bit(value, 7);
			break;
		}
		case PPUMASK_ADDR: {
			/*
			source: https://wiki.nesdev.com/w/index.php?title=PPU_registers

			7  bit  0
			---- ----
			BGRs bMmG
			|||| ||||
			|||| |||+- Greyscale (0: normal color, 1: produce a greyscale display)
			|||| ||+-- 1: Show background in leftmost 8 pixels of screen, 0: Hide
			|||| |+--- 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
			|||| +---- 1: Show background
			|||+------ 1: Show sprites
			||+------- Emphasize red (green on PAL/Dendy)
			|+-------- Emphasize green (red on PAL/Dendy)
			+--------- Emphasize blue
			*/

			ppu->PPUMASK = value;

			ppu->should_render_background = get_bit(value, 3);
			ppu->should_render_sprites = get_bit(value, 4);

			break;
		}
		case PPUSTATUS_ADDR: {

			break;
		}
		case OAMADDR_ADDR: {
			ppu->OAMADDR = value;
			break;
		}
		case OAMDATA_ADDR: {
			printf("we writing to OAMDATA!\n");
			ppu->oam[ppu->OAMADDR] = value;
			ppu->OAMADDR++;
			break;
		}
		case PPUADDR_ADDR: {
			if (ppu->PPUADDR_2nd_write) {
				ppu->PPUADDR |= value;
			} else {
				ppu->PPUADDR = (value << 8);
			}			
			ppu->PPUADDR_2nd_write = !ppu->PPUADDR_2nd_write;
			//printf("ppuaddr: %04x\n", ppu->PPUADDR);
			break;
		}
		case PPUDATA_ADDR: {
			ppu->vmem->data[ppu->PPUADDR] = value;
			ppu->PPUADDR += ppu->PPUADDR_increment_amount;
			break;
		}
		case OAMDMA_ADDR: {
			//printf("writing 256 bytes to OAM DMA from %04x!\n", value * 0x100);
			memcpy(ppu->oam, &mem[value * 0x100], 0x100);
			cpu->wait_cycles = 513;

			break;
		}
		case CONTROLLER_IO_ADDR: {
			cpu->strobe_keys = value & 1;
			if (cpu->strobe_keys) {
				cpu->strobe_keys_write_no = 0;
			}
			//printf("strobe_keys: %i\n", cpu->strobe_keys);
			break;
		}

		default:
			mem[address] = value;
	}
}

void mem_write_16(nes_cpu_t *cpu, uint16_t address, uint16_t value)
{
	mem_write_8(cpu, address, value & 0xff);
   	mem_write_8(cpu, address + 1, (value & 0xff00) >> 8);
}

__attribute__((noinline))
uint8_t mem_read_8(nes_cpu_t *cpu, uint16_t address)
{

	uint8_t *mem = cpu->mem->data;
	nes_ppu_t *ppu = cpu->ppu;
	
	switch (address) {
		case PPUCTRL_ADDR: {
			break;
		}
		case PPUMASK_ADDR: {
			break;
		}
		case PPUSTATUS_ADDR: {
			uint8_t copy = (ppu->PPUSTATUS & 0b01111111) | (ppu->NMI_occurred << 7) | (ppu->sprite0hit << 6);

			ppu->NMI_occurred = false;

			return copy;
			break;
		}
		case OAMADDR_ADDR: {

			break;
		}
		case OAMDATA_ADDR: {
			uint8_t copy = ppu->oam[ppu->OAMADDR];
			if (!ppu->in_vblank)
				ppu->OAMADDR++;
			return copy;
			break;
		}
		case OAMDMA_ADDR: {
			return 0;
			break;
		}
		case CONTROLLER_IO_ADDR: {
			if (!cpu->strobe_keys) {
				bool bit = (cpu->key_state >> cpu->strobe_keys_write_no) & 1;
				cpu->strobe_keys_write_no++;
				return bit;
			}
			return cpu->key_state & 1;
			break;
		}

		default:
			return mem[address];
	}

	return 0;
}

uint16_t mem_read_16(nes_cpu_t *cpu, uint16_t address)
{
	uint8_t *mem = cpu->mem->data;

	uint8_t lo_byte = mem_read_8(cpu, address);
	uint8_t hi_byte = mem_read_8(cpu, address + 1);
	
	return (hi_byte << 8) | (lo_byte);
}