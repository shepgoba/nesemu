#ifndef NES_INCLUDE
#define NES_INCLUDE
#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"
#include "memory.h"
#include "ppu.h"


struct SDL_Renderer;
struct SDL_Texture;

#define SPEED_MODIFIER 1.0

#define MILLISECONDS_PER_SECOND 1000.0
#define NES_FRAMES_PER_SECOND 60.0988138974

#define MASTER_CLOCK_PER_SEC 21477272
#define MASTER_CLOCK_CYCLES_PER_FRAME 357366

struct nes_render_context {
	struct SDL_Renderer *renderer;
	struct SDL_Texture *video_texture;
};

typedef struct nes_render_context nes_render_context_t;


struct nes {
	nes_cpu_t cpu;
	nes_memory_t memory;
	nes_ppu_t ppu;
	nes_vmemory_t vmemory;
	
	ines_rom_header_t rom_header;
	nes_rom_info_t rom_info;

	uint8_t *rom_data;
	uint32_t *video_data;

	nes_render_context_t render_ctx;

	uint8_t key_state;

	uint32_t frame_start;
	uint64_t frames;

	uint64_t master_clock_cycles;
};

typedef struct nes nes_t;

bool nes_init(nes_t *, nes_render_context_t *);
void nes_cleanup(nes_t *);

void nes_do_master_cycle(nes_t *, uint32_t);
bool nes_load_rom(nes_t *, const char *);
void nes_delay_if_necessary(nes_t *);
void nes_do_frame_cycle(nes_t *);

// utilities
void nes_dump_memory(nes_t *, const char *);
void nes_dump_vmemory(nes_t *, const char *);

#endif