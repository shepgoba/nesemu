#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <time.h>
#include "utils.h"
#include "utils_platform.h"

bool byte_to_binary_str(char *buf, size_t buf_len, uint8_t byte)
{
    if (buf_len < 9) {
        return false;
    }
    
    for (size_t i = 0; i < 8; i++) {
        buf[i] = (byte >> (7 - i)) & 1 ? '1' : '0';
    }

    buf[buf_len - 1] = '\0';
    return true;
}

void exit_with_error(int code, const char *message, ...)
{
	va_list args;
    va_start(args, message);

	vprintf(message, args);
	va_end(args);
	fputc('\n', stdout);
	exit(code);
}

void log_event(const char *message, ...)
{
	va_list args;
	va_start(args, message);
	

	precise_time_t t = get_precise_time();
	struct tm *tm_info = localtime(&t.time);

	char buffer[26];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

	printf("[%s.%06lli] ", buffer, t.nanoseconds / 1000);

	vprintf(message, args);
	va_end(args);

	fputc('\n', stdout);
}

void handle_keypress(SDL_Event *event, uint8_t *key_state)
{
	static const SDL_Keycode keys[] = {SDLK_A, SDLK_S, SDLK_O, SDLK_P, SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
	
	SDL_Keycode pressed_key = event->key.key;
	bool key_down = event->key.type == SDL_EVENT_KEY_DOWN;

	uint8_t temp_key_state = *key_state;

	for (int idx = 0; idx < 8; idx++) {
		if (pressed_key == keys[idx]) {
			temp_key_state ^= (-key_down ^ temp_key_state) & (1UL << idx);
			break;
		}
	}

	*key_state = temp_key_state;
}

bool read_bytes(void *addr, uint32_t num_bytes, uint32_t offset, FILE *file)
{
	long int orig = ftell(file);

	fseek(file, offset, SEEK_SET);

	size_t result;
	result = fread((uint8_t *)addr, sizeof(uint8_t), num_bytes, file);

	fseek(file, orig, SEEK_SET);

	return result == (sizeof(uint8_t) * num_bytes);
}

static uint32_t bswap32(uint32_t x)
{
	return
		((x << 24) & 0xff000000) |
		((x <<  8) & 0x00ff0000) |
		((x >>  8) & 0x0000ff00) |
		((x >> 24) & 0x000000ff);
}

static inline uint8_t __get_bit_8(uint8_t byte, int bit) 
{
	return (byte >> bit) & 1;
}

#define INES_ROM_MAGIC 0x4E45531A

bool get_rom_info(FILE *handle, ines_rom_header_t *header, nes_rom_info_t *rom)
{
	if (!handle || !header || !rom)
		return false;

	int result = read_bytes(header, sizeof(header), 0, handle);
	if (!result) {
		log_event("Error getting ROM header");
		return false;
	}

	uint32_t rom_magic_swapped = bswap32(header->magic);

	bool rom_is_valid = rom_magic_swapped == INES_ROM_MAGIC;
	if (!rom_is_valid)
		return false;

	rom->prg_size = header->PRG_ROM_size * 16384;
	rom->chr_size = 0;
	rom->use_chr_ram = false;

	if (header->CHR_ROM_size != 0) {
		rom->chr_size = header->CHR_ROM_size * 8192;
	} else {
		rom->use_chr_ram = true;
	}

	rom->battery_backed_ram = __get_bit_8(header->flags6, 1);
	rom->use_trainer = __get_bit_8(header->flags6, 3);
	
	uint8_t mapper_id_lo_nibble = header->flags6 >> 4;

	// flag7 & 0xf0 is the high nibble of the mapper id
	rom->mapper_id = (header->flags7 & 0xf0) | mapper_id_lo_nibble;

	if (rom->mapper_id != 0 && rom->mapper_id != 1) {
		log_event("Unsupported ROM mapper ID! Exiting...");
		return false;
	}

	return true;
}
