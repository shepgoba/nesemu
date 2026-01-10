#include "apu.h"
#include <SDL3/SDL_audio.h>
#include <stdio.h>

#define CPU_FREQUENCY 1789773

static SDL_AudioDeviceID device;
static SDL_AudioStream *stream;

bool apu_init(nes_apu_t *apu)
{
	
    SDL_AudioSpec dev_spec;
    dev_spec.freq = 44100;
    dev_spec.format = SDL_AUDIO_F32;
    dev_spec.channels = 2;

    device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &dev_spec);
    if (!device) {
        return false;
    }

    SDL_AudioSpec src_spec;
    src_spec.freq = 44100;
    src_spec.format = SDL_AUDIO_F32;
    src_spec.channels = 1;

    stream = SDL_CreateAudioStream(&src_spec, NULL);
    if (!stream) {
        SDL_CloseAudioDevice(device);
        return false;
    }

	apu->status = 0;

	return true;
}

void apu_pulse1_play(nes_apu_t *apu)
{

}
