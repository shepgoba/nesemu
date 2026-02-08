#ifndef APU_INCLUDE
#define APU_INCLUDE
#include <stdbool.h>
#include <stdint.h>

#define PULSE1_DLCV 0x4000
#define PULSE1_SWEEP 0x4001
#define PULSE1_TIMER_LO 0x4002
#define PULSE1_LENGTH_CTR_TIMER_HI 0x4003

#define PULSE2_DLCV 0x4004
#define PULSE2_SWEEP 0x4005
#define PULSE2_TIMER_LO 0x4006
#define PULSE2_LENGTH_CTR_TIMER_HI 0x4007

#define TRIANGLE_CR 0x4008
#define TRIANGLE_TIMER_LO 0x400A
#define TRIANGLE_LENGTH_CTR_TIMER_HI 0x400B

#define APU_STATUS 0x4015

#define APU_FRAME_COUNTER 0x4017

#define MASTER_CLOCKS_PER_APU_CLOCK 24

typedef struct {
	int duty : 2;
	bool loop : 1;
	bool constant_volume : 1;
	int volume_envelope : 4;

	bool sweep_enabled : 1;
	int period : 3;
	bool negate : 1;
	int shift : 3;

	int timer : 11;
	int length_counter : 5;
} nes_apu_pulse_t;

typedef struct {
	nes_apu_pulse_t pulse1;
	nes_apu_pulse_t pulse2;
	uint8_t status;
} nes_apu_t;


bool apu_init(nes_apu_t *);
void apu_pulse_play(nes_apu_t *, nes_apu_pulse_t *pulse);
#endif // APU_INCLUDE
