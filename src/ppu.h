#ifndef PPU_INCLUDE
#define PPU_INCLUDE
#include <stdint.h>
#include "memory.h"
#include "utils.h"

#define PPUCTRL_ADDR 0x2000
#define PPUMASK_ADDR 0x2001
#define PPUSTATUS_ADDR 0x2002
#define OAMADDR_ADDR 0x2003
#define OAMDATA_ADDR 0x2004
#define PPUSCROLL_ADDR 0x2005
#define PPUADDR_ADDR 0x2006
#define PPUDATA_ADDR 0x2007
#define OAMDMA_ADDR 0x4014

#define DOTS_PER_SCANLINE 341
#define SCANLINES_PER_FRAME 262

#define HORIZONTAL_TILE_COUNT 32
#define VERTICAL_TILE_COUNT 30

#define INTERNAL_VIDEO_WIDTH 256
#define INTERNAL_VIDEO_HEIGHT 240

#define MASTER_CLOCKS_PER_PPU_CLOCK 4

#define VIDEO_SCALE 2


typedef struct {
	nes_vmemory_t *vmem;

	uint8_t PPUCTRL;
	uint8_t PPUMASK;
	uint8_t PPUSTATUS;
	uint8_t OAMADDR;
	uint8_t OAMDATA;
	uint8_t PPUSCROLL;
	uint16_t PPUADDR;
	uint8_t PPUDATA;

	uint8_t oam[256];

	bool NMI_output;
	bool NMI_occurred;
	bool in_vblank;
	bool PPUADDR_2nd_write;

	bool should_render_background;
	bool should_render_sprites;
	bool sprites8x16;
	bool sprite0hit;

	int nametable_base;
	int background_tiledata_base;
	int sprite_tiledata_base;

	int PPUADDR_increment_amount;
	
	int dot_clock_scanline;
	int scanline;
} nes_ppu_t;

void ppu_init(nes_ppu_t *, nes_vmemory_t *);
void ppu_draw_scanline(nes_ppu_t *ppu, uint32_t *);
void ppu_update_registers(nes_ppu_t *, bool *, uint32_t *);
void ppu_cleanup(nes_ppu_t *);
#endif