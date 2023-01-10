#ifndef UTILS_INCLUDE
#define UTILS_INCLUDE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "memory.h"
#include <SDL2/SDL.h>

typedef struct {
    uint32_t magic;
    uint8_t PRG_ROM_size;
    uint8_t CHR_ROM_size;
    uint8_t flags6;
    uint8_t flags7;
    uint8_t flags8;
    uint8_t flags9;
    uint8_t flags10;
	char padding[5];
} ines_rom_header_t;


typedef struct {
	int rom_size;
	int chr_size;
	int mapper_id;
	bool use_chr_ram;
	bool use_trainer;
} nes_rom_info_t;

uint32_t bswap32(uint32_t);
bool get_bit(uint8_t, int);
bool read_bytes(void *, uint32_t, uint32_t, FILE *);
bool get_rom_info(FILE *, ines_rom_header_t *, nes_rom_info_t *);
void handle_keypress(SDL_Event *, uint8_t *);
void exit_with_error(int, const char *, ...);

#endif