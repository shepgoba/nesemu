#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "utils.h"
#include "nes.h"

#define WINDOW_NAME "NESEMU"

int main(int argc, char **argv)
{
	if (argc < 2) {
		exit_with_error(2, "Usage: nesemu <rom path>");
	}

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		exit_with_error(1, "Couldn't initialize SDL");
	}

	SDL_Window *window = SDL_CreateWindow(
		WINDOW_NAME,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		INTERNAL_VIDEO_WIDTH * VIDEO_SCALE,
		INTERNAL_VIDEO_HEIGHT * VIDEO_SCALE,
		0
	);
	if (!window) {
		exit_with_error(5, "Could not create window!");
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(
		window, 
		-1, 
		SDL_RENDERER_ACCELERATED
	);
	if (!renderer) {
		exit_with_error(6, "Could not create renderer!");
	}

	SDL_Texture *video_texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888, 
		SDL_TEXTUREACCESS_STREAMING, 
		INTERNAL_VIDEO_WIDTH, 
		INTERNAL_VIDEO_HEIGHT
	);
	if (!video_texture) {
		exit_with_error(7, "Could not create video texture!");
	}


	nes_render_context_t render_ctx = {
		renderer,
		video_texture
	};

	nes_t nes;
	if (!nes_init(&nes, &render_ctx)) {
		exit_with_error(3, "Could not create main NES data!");
	}

	if (!nes_load_rom(&nes, argv[1])) {
		exit_with_error(4, "Could not load NES rom!");
	}

	int pitch;
	SDL_LockTexture(video_texture, NULL, (void **)&nes.video_data, &pitch);

	while (true) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: {
					goto main_cleanup;
				}
				case SDL_KEYDOWN:
				case SDL_KEYUP: {
					if (event.key.keysym.sym == SDLK_d) {
						dump_memory(&nes.memory, "mem.bin");
						dump_vmemory(&nes.vmemory, "vmem.bin");
						exit(0);
					} else {
						handle_keypress(&event, &nes.key_state);
					}
					break;
				}
			}
		}

		nes_do_frame_cycle(&nes);
		nes_delay_if_necessary(&nes);
	}

main_cleanup:
	nes_cleanup(&nes);

	SDL_DestroyTexture(video_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	
	SDL_Quit();
	return 0;
}