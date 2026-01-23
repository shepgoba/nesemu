#include "ppu.h"
#include <string.h>

void ppu_init(nes_ppu_t *ppu, nes_vmemory_t *vmem) 
{
	memset(ppu, 0, sizeof(*ppu));

	ppu->vmem = vmem;
	ppu->dot_clock_scanline = 0;
	ppu->scanline = 0;
	ppu->NMI_output = false;
	ppu->W_toggle = false;
	ppu->nametable_base_offset = 0;
	ppu->background_tiledata_base_offset = 0;
	ppu->sprite_tiledata_base_offset = 0;
	ppu->PPUADDR_increment_amount = 1;
    ppu->ppudata_buf = 0;
}

void ppu_update_registers(nes_ppu_t *ppu, bool *should_update_frame, uint32_t *video_data)
{
	if (ppu->scanline < 240) {
		// normal operation
		if (ppu->dot_clock_scanline == 340)
			ppu_draw_scanline(ppu, video_data);
	} else if (ppu->scanline == 241) {
		// start of vblank
		if (ppu->dot_clock_scanline == 1) {
			ppu->in_vblank = true;
			ppu->triggered_NMI = false;
		}
	} else if (ppu->scanline == 262) {
		// end of vblank
		ppu->scanline = 0;
		if (ppu->dot_clock_scanline == 1) {
			ppu->sprite0hit = false;
			ppu->in_vblank = false;
			*should_update_frame = true;
		}
	}
	if (ppu->dot_clock_scanline == 341) {
		ppu->scanline++;
		ppu->dot_clock_scanline = 0;
	}

}

// Stored in RGB 8-bit format (0xRRGGBB)
static const uint32_t ntsc_rgb_table[64] = {
	0x464646, 0x00065a, 0x000678, 0x020673, 0x35034c, 0x57000e, 0x5a0000, 0x410000, 0x120200, 0x001400, 0x001e00, 0x001e00, 0x001521, 0x000000, 0x000000, 0x000000, 
	0x9d9d9d, 0x004ab9, 0x0530e1, 0x5718da, 0x9f07a7, 0xcc0255, 0xcf0b00, 0xa42300, 0x5c3f00, 0x0b5800, 0x006600, 0x006713, 0x005e6e, 0x000000, 0x000000, 0x000000, 
	0xfeffff, 0x1f9eff, 0x5376ff, 0x9865ff, 0xfc67ff, 0xff6cb3, 0xff7466, 0xff8014, 0xc49a00, 0x71b300, 0x28c421, 0x00c874, 0x00bfd0, 0x2b2b2b, 0x000000, 0x000000, 
	0xfeffff, 0x9ed5ff, 0xafc0ff, 0xd0b8ff, 0xfebfff, 0xffc0e0, 0xffc3bd, 0xffca9c, 0xe7d58b, 0xc5df8e, 0xa6e6a3, 0x94e8c5, 0x92e4eb, 0xa7a7a7, 0x000000, 0x000000
};

static inline uint8_t __get_bit_8(uint8_t byte, int bit) 
{
	return (byte >> bit) & 1;
}

static inline uint8_t __mirror_bits(uint8_t num) 
{
	uint8_t val = ((num & 0x01) << 7)
				| ((num & 0x02) << 5)
				| ((num & 0x04) << 3)
				| ((num & 0x08) << 1)
				| ((num & 0x10) >> 1)
				| ((num & 0x20) >> 3)
				| ((num & 0x40) >> 5)
				| ((num & 0x80) >> 7);
	return val;
}


void ppu_draw_background_scanline(nes_ppu_t *ppu, uint32_t *video_data)
{
	uint8_t *bg_tiledata_ptr = &ppu->vmem->data[ppu->background_tiledata_base_offset];
	uint8_t *attributedata_ptr = &ppu->vmem->data[ppu->nametable_base_offset + 0x3c0];
	uint8_t *nametable_ptr = &ppu->vmem->data[ppu->nametable_base_offset];

	int scx = ppu->PPUSCROLLX;
	int scy = ppu->PPUSCROLLY;

	for (int cur_tile_idx = 0; cur_tile_idx < HORIZONTAL_TILE_COUNT; cur_tile_idx++) {
		int palette_offset = (ppu->scanline / 32) * 8 + cur_tile_idx / 4;
		uint8_t palette_data = attributedata_ptr[palette_offset];

		bool horizontal_odd = (cur_tile_idx / 2) & 1;
		bool vertical_odd = (ppu->scanline / 16) & 1;

		uint8_t specific_palette_data = palette_data >> (((vertical_odd << 1) | horizontal_odd) << 1) & 0b11;
		int nametable_offset = (ppu->scanline / 8) * 32 + cur_tile_idx;

		uint8_t tile_data_offset = nametable_ptr[nametable_offset];
		int offset = (tile_data_offset * 16) + (ppu->scanline % 8);
		
		uint8_t lo_bits = bg_tiledata_ptr[offset];
		uint8_t hi_bits = bg_tiledata_ptr[offset + 8];

		for (int pixel_x = 0; pixel_x < 8; pixel_x++) {
			uint8_t pixel_data = (((hi_bits >> (7 - pixel_x)) & 1) << 1) | 
								  ((lo_bits >> (7 - pixel_x)) & 1);

			uint16_t bg_palette_offset = pixel_data == 0 ? 0x3f00 :
				0x3f00 + specific_palette_data * 4;

			uint8_t *bg_palette_addr = ppu->vmem->data + bg_palette_offset;

			uint32_t final_color = ntsc_rgb_table[bg_palette_addr[pixel_data]];

			uint16_t pixel_addr = ppu->scanline * INTERNAL_VIDEO_WIDTH + cur_tile_idx * 0x8 + pixel_x;
			video_data[pixel_addr] = final_color;
		}
	}
}

void ppu_draw_sprite_scanline(nes_ppu_t *ppu, uint32_t *video_data)
{
	uint8_t *sprite_tiledata = ppu->vmem->data + ppu->sprite_tiledata_base_offset;
 
	for (int oam_idx = 0; oam_idx < 0x40; oam_idx += 4) {
		uint8_t sprite_y = ppu->oam[oam_idx];

		// 0xef to 0xff is off screen and will be invisible, so continue
		if (sprite_y >= 0xef) {
			continue;
		}

		uint8_t sprite_tile_idx = ppu->oam[oam_idx + 1];
		uint8_t sprite_attributes = ppu->oam[oam_idx + 2];
		uint8_t sprite_x = ppu->oam[oam_idx + 3];
		
		int sprite_palette = sprite_attributes & 0b11;


		if (ppu->scanline >= sprite_y + 1 && ppu->scanline < sprite_y + 9) {
			int slice_offset = (sprite_tile_idx * 16) + ((ppu->scanline - sprite_y - 1) % 8) % 8;//+ (((8 - (sprite_y - 1)) % 8) + (ppu->scanline % 8)) % 8;

			uint8_t lo_bits = sprite_tiledata[slice_offset];
			uint8_t hi_bits = sprite_tiledata[slice_offset + 8];

			// flip horizontally
			if (__get_bit_8(sprite_attributes, 6)) {
				lo_bits = __mirror_bits(lo_bits);
				hi_bits = __mirror_bits(hi_bits);
			}

			for (int pixel_x = 0; pixel_x < 8; pixel_x++) {
				uint8_t pixel_data = (((hi_bits >> (7 - pixel_x)) & 1) << 1) | 
									  ((lo_bits >> (7 - pixel_x)) & 1);

				if (pixel_data == 0)
					continue;

				uint8_t *sprite_palette_addr = ppu->vmem->data + 0x3f10 + sprite_palette * 4;
				uint32_t final_color = ntsc_rgb_table[sprite_palette_addr[pixel_data]];
				int pixel_addr = ppu->scanline * INTERNAL_VIDEO_WIDTH + (sprite_x + pixel_x);

				// sprite 0 hit
				if (video_data[pixel_addr]) {
					ppu->sprite0hit = true;
				}

				video_data[pixel_addr] = final_color;
			}
		}
	}
}
/*
TODO: Not assume background pixels are always first
*/
void ppu_draw_scanline(nes_ppu_t *ppu, uint32_t *video_data)
{
	if (ppu->should_render_background) {
		ppu_draw_background_scanline(ppu, video_data);
	}

	if (ppu->should_render_sprites) {
		ppu_draw_sprite_scanline(ppu, video_data);
	}
}

void ppu_cleanup(nes_ppu_t *ppu)
{
	// not needed (yet)	
}
