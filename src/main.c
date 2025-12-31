#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "utils.h"
#include "nes.h"

#define WINDOW_NAME "NESEMU"

int main(int argc, char **argv)
{
	if (argc < 2) {
		exit_with_error(2, "Usage: nesemu <rom path>");
	}

	printf("NESEMU v0.1\n");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		exit_with_error(1, "Couldn't initialize SDL: %s", SDL_GetError());
	}

	SDL_Window *window = SDL_CreateWindow(
		WINDOW_NAME,
		INTERNAL_VIDEO_WIDTH * VIDEO_SCALE,
		INTERNAL_VIDEO_HEIGHT * VIDEO_SCALE,
		0
	);
	if (!window) {
		exit_with_error(5, "Could not create window!");
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(
		window, 
		NULL
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
	SDL_SetTextureScaleMode(video_texture, SDL_SCALEMODE_NEAREST);

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
				case SDL_EVENT_QUIT: {
					goto main_cleanup;
					break;
				}
				case SDL_EVENT_KEY_DOWN:
				case SDL_EVENT_KEY_UP: {
					if (event.key.key == SDLK_D) {
						nes_dump_memory(&nes, "debug/mem.bin");
						nes_dump_vmemory(&nes, "debug/vmem.bin");
						log_event("Dumping RAM / VRAM. Exiting...");

						goto main_cleanup;
					} else {
						handle_keypress(&event, &nes.key_state);
					}
					break;
				}
				default: break;
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