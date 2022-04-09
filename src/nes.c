#include "nes.h"

bool nes_init(nes_t *nes, nes_render_context_t *render_ctx)
{

	if (!memory_init(&nes->memory)) {
		printf("Could not allocate memory!\n");
		return false;
	}

	if (!vmemory_init(&nes->vmemory)) {
		printf("Could not allocate vmemory!\n");
		return false;
	}

	ppu_init(&nes->ppu, &nes->vmemory);
	cpu_init(&nes->cpu, &nes->memory, &nes->ppu);

	nes->render_ctx = render_ctx;

	nes->use_mmc1 = false;

	nes->frames = 0;
	nes->frame_start = 0;
	nes->key_state = 0;
	nes->master_clock_cycles = 0;

	nes->rom_data = NULL;
	nes->video_data = NULL;

	return true;
}

bool nes_load_rom(nes_t *nes, const char *path) 
{
	FILE *rom_handle = fopen(path, "rb");
	if (!rom_handle) {
		return false;
	}

	if (!get_rom_info(rom_handle, &nes->rom_header, &nes->rom_info)) {
		return false;
	}

	if (nes->rom_info.mapper_id == 1) {
		nes->cpu.use_mmc1 = true;
		printf("setting mmc1 to true\n");
	}

	bool copy_prg_rom_result = 
		read_bytes(nes->memory.data + 0xc000, nes->rom_info.rom_size, 0x10, rom_handle);
		
	if (!copy_prg_rom_result) {
		printf("couldn't copy PRG ROM!\n");
		return false;
	}

	bool copy_chr_rom_result = read_bytes(
		nes->vmemory.data, 
		nes->rom_info.chr_size, 
		0x10 + nes->rom_info.rom_size, 
		rom_handle
	);

	if (!copy_chr_rom_result) {
		printf("couldn't copy CHR ROM!\n");
		return false;
	}

	nes->rom_data = malloc(nes->rom_info.rom_size);
	if (!nes->rom_data) {
		printf("couldn't allocate rom data\n");
		return false;
	}
	
	read_bytes(nes->rom_data, nes->rom_info.rom_size, 0x10, rom_handle);	
	
	cpu_reset(&nes->cpu);

	fclose(rom_handle);

	return true;
}

static void render_frame(nes_render_context_t *render_ctx, uint32_t **video_data) 
{
	static const SDL_Rect video_display_rect = {
		0,
		0, 
		INTERNAL_VIDEO_WIDTH * VIDEO_SCALE, 
		INTERNAL_VIDEO_HEIGHT * VIDEO_SCALE
	};

	SDL_UnlockTexture(render_ctx->video_texture);

	SDL_RenderCopy(render_ctx->renderer, render_ctx->video_texture, NULL, &video_display_rect);
	SDL_RenderPresent(render_ctx->renderer);

	int texture_pitch;
	SDL_LockTexture(render_ctx->video_texture, NULL, (void **)video_data, &texture_pitch);
}

static void nes_do_ppu_cycle(nes_t *nes, nes_render_context_t *render_ctx)
{
	bool should_update_frame = false;
	ppu_update_registers(&nes->ppu, &should_update_frame, nes->video_data);

	if (should_update_frame) {
		render_frame(render_ctx, &nes->video_data);
	}

	nes->ppu.dot_clock_scanline++;
}

static void nes_do_cpu_cycle(nes_t *nes)
{
	cpu_update_registers(&nes->cpu, nes->key_state);

	if (nes->cpu.wait_cycles == 0) {
		cpu_run_cycle(&nes->cpu);
	}

	nes->cpu.wait_cycles--;
}

void nes_do_master_cycle(nes_t *nes, uint32_t master_clock_frame)
{
	if ((master_clock_frame % MASTER_CLOCKS_PER_PPU_CLOCK) == 0) {
		nes_do_ppu_cycle(nes, nes->render_ctx);
	}

	if ((master_clock_frame % MASTER_CLOCKS_PER_CPU_CLOCK) == 0) {
		nes_do_cpu_cycle(nes);
	}
}


void nes_cleanup(nes_t *nes)
{
	free(nes->rom_data);

	cpu_cleanup(&nes->cpu);
	ppu_cleanup(&nes->ppu);
	vmemory_cleanup(&nes->vmemory);
	memory_cleanup(&nes->memory);
}


static void nes_delay(nes_t *nes, uint32_t time)
{
	SDL_Delay(time);
}

void nes_delay_if_necessary(nes_t *nes)
{
	static const double TARGET_DELAY_TIME = 
		MILLISECONDS_PER_SECOND / (NES_FRAMES_PER_SECOND * SPEED_MODIFIER);
	
	uint32_t frame_time = SDL_GetTicks() - nes->frame_start;

	if (TARGET_DELAY_TIME > frame_time) {
		uint32_t time_delta = TARGET_DELAY_TIME - frame_time;
		nes_delay(nes, time_delta);
	}
}

