#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "utils.h"

void exit_with_error(int code, const char *message, ...)
{
	va_list args;
    va_start(args, message);

	printf(message, args);
	va_end(args);
	fputc('\n', stdout);
	exit(code);
}

void handle_keypress(SDL_Event *event, uint8_t *key_state)
{
	static const SDL_KeyCode keys[] = {SDLK_a, SDLK_s, SDLK_o, SDLK_p, SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
	
	SDL_KeyCode pressed_key = event->key.keysym.sym;
	bool key_down = event->key.type == SDL_KEYDOWN;

	uint8_t temp_key_state = *key_state;

	for (int idx = 0; idx < 8; idx++) {
		if (pressed_key == keys[idx]) {
			temp_key_state ^= (-key_down ^ temp_key_state) & (1UL << idx);
			break;
		}
	}

	*key_state = temp_key_state;
}

bool get_bit(uint8_t value, int bit) 
{
	return (value >> bit) & 1;
}

bool read_bytes(void *addr, uint32_t num_bytes, uint32_t offset, FILE *file)
{
	int orig = ftell(file);

	fseek(file, offset, SEEK_SET);

	int result;
	result = fread((uint8_t *)addr, sizeof(uint8_t), num_bytes, file);

	fseek(file, orig, SEEK_SET);

	return result == (sizeof(uint8_t) * num_bytes);
}

uint32_t bswap32(uint32_t x)
{
	return
		((x << 24) & 0xff000000) |
		((x <<  8) & 0x00ff0000) |
		((x >>  8) & 0x0000ff00) |
		((x >> 24) & 0x000000ff);
}

#define INES_ROM_MAGIC 0x4E45531A

bool get_rom_info(FILE *handle, ines_rom_header_t *header, nes_rom_info_t *rom)
{
	if (!handle || !header || !rom)
		return false;

	int result = read_bytes(header, sizeof(header), 0, handle);
	if (!result) {
		printf("Error getting rom header\n");
		return false;
	}

	uint32_t rom_magic_swapped = bswap32(header->magic);

	bool rom_is_valid = rom_magic_swapped == INES_ROM_MAGIC;

	if (rom_is_valid) {
		rom->prg_size = header->PRG_ROM_size * 16384;
		rom->chr_size = 0;
		rom->use_chr_ram = false;

		if (header->CHR_ROM_size != 0) {
			rom->chr_size = header->CHR_ROM_size * 8192;
		} else {
			rom->use_chr_ram = true;
		}

		rom->use_trainer = (header->flags6 & 0b00001000) >> 3;
		rom->mapper_id = (header->flags7 & 0xf0) | ((header->flags6 & 0xf0) >> 4);

		if (rom->mapper_id != 0 && rom->mapper_id != 1) {
			printf("unsupported mapper id! exiting...\n");
			return false;
		}
	}
	return rom_is_valid;
}