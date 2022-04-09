#include "ppu.h"

void ppu_init(nes_ppu_t *ppu, nes_vmemory_t *vmem) 
{
	if (ppu && vmem) {
		ppu->vmem = vmem;
		ppu->dot_clock_scanline = 0;
		ppu->scanline = 0;
		ppu->NMI_output = false;
		ppu->NMI_occurred = false;
		ppu->PPUADDR_2nd_write = false;
		ppu->nametable_base = 0;
		ppu->PPUADDR_increment_amount = 1;
	}
}

void ppu_update_registers(nes_ppu_t *ppu, bool *should_update_frame, uint32_t *video_data)
{
	if (ppu->dot_clock_scanline == 341) {
		ppu->scanline++;
		ppu->dot_clock_scanline = 0;
	}

	if (ppu->scanline == 241) {
		if (ppu->dot_clock_scanline == 1) {
			ppu->PPUSTATUS |= 0b01111111;
			ppu->NMI_occurred = true;
			ppu->in_vblank = true;
		}
	} else if (ppu->scanline == 262) {
		ppu->scanline = 0;
		if (ppu->dot_clock_scanline == 0) {
			ppu->NMI_occurred = false;
			ppu->in_vblank = false;

			if (should_update_frame) {
				*should_update_frame = true;
			}
		} else if (ppu->dot_clock_scanline == 1) {
			ppu->PPUSTATUS &= 0b01111111;
			ppu->sprite0hit = false;
		}
	}
	if (ppu->scanline < 240 && ppu->dot_clock_scanline == 340) {
		ppu_draw_scanline(ppu, video_data);
	}
}

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

static inline uint8_t reverse(uint8_t num) 
{
	uint8_t pog = ((num & 0x01) << 7)
				| ((num & 0x02) << 5)
				| ((num & 0x04) << 3)
				| ((num & 0x08) << 1)
				| ((num & 0x20) >> 3)
				| ((num & 0x10) >> 1)
				| ((num & 0x40) >> 5)
				| ((num & 0x80) >> 7);
	return pog;
}

void ppu_draw_scanline(nes_ppu_t *ppu, uint32_t *video_data)
{
	if (ppu->should_render_background) {
		uint8_t *bg_tiledata = ppu->vmem->data + ppu->background_tiledata_base;	
		uint8_t *attributedata = ppu->vmem->data + ppu->nametable_base + 0x3c0;

		for (int tile = 0; tile < HORIZONTAL_TILE_COUNT; tile++) {
			int palette_offset = (ppu->scanline / 32) * 8 + tile / 4;
			uint8_t palette_data = attributedata[palette_offset];
			uint8_t specific_palette_data;

			bool horizontal_odd = (tile / 2) & 1;
			bool vertical_odd = (ppu->scanline / 16) & 1;
			if (horizontal_odd && vertical_odd) {
				specific_palette_data = palette_data >> 6;
			} else if (vertical_odd) {
				specific_palette_data = (palette_data >> 4) & 0b11;
			} else if (horizontal_odd) {
				specific_palette_data = (palette_data >> 2) & 0b11;
			} else {
				specific_palette_data = palette_data & 0b11;
			}

			int tile_index = (ppu->scanline / 8) * 32 + tile;
			uint8_t tile_data_offset = (ppu->vmem->data + ppu->nametable_base)[tile_index];
			int offset = (tile_data_offset * 16) + (ppu->scanline % 8);
			
			uint8_t lo_bits = bg_tiledata[offset];
			uint8_t hi_bits = bg_tiledata[offset + 8];

			for (int pixel_x = 0; pixel_x < 8; pixel_x++) {
				uint8_t pixel_data = (((hi_bits >> (7 - pixel_x)) & 1) << 1) | 
									 (lo_bits >> (7 - pixel_x)) & 1;
				
				uint8_t *sprite_palette_addr = ppu->vmem->data + 0x3f00 + specific_palette_data * 4;
				uint32_t final_color = 0xff000000 | ntsc_rgb_table[sprite_palette_addr[pixel_data]];

				video_data[ppu->scanline * INTERNAL_VIDEO_WIDTH + tile * 0x8 + pixel_x] = final_color;
			}
		}
	}

	if (ppu->should_render_sprites) {
		uint8_t *sprite_tiledata = ppu->vmem->data + ppu->sprite_tiledata_base;
 
		for (int oam_idx = 0; oam_idx < 0x40; oam_idx += 4) {
			uint8_t sprite_y = ppu->oam[oam_idx];

			// 0xef to 0xff is off screen and will be invisible, so continue
			if (sprite_y >= 0xef && sprite_y <= 0xff) {
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
				if (__get_bit_8(sprite_attributes, 6)) {
					lo_bits = reverse(lo_bits);
					hi_bits = reverse(hi_bits);
				}
				for (int pixel_x = 0; pixel_x < 8; pixel_x++) {
					uint8_t pixel_data = (((hi_bits >> (7 - pixel_x)) & 1) << 1) | 
											(lo_bits >> (7 - pixel_x)) & 1;

					uint8_t *sprite_palette_addr = ppu->vmem->data + 0x3f10 + sprite_palette * 4;
					uint32_t final_color = 0xff000000 | ntsc_rgb_table[sprite_palette_addr[pixel_data]];
					int pixel_addr = ppu->scanline * INTERNAL_VIDEO_WIDTH + (sprite_x + pixel_x);

					// sprite 0 hit
					if (pixel_data && video_data[pixel_addr]) {
						ppu->sprite0hit = true;
					}

					if (pixel_data != 0)
						video_data[pixel_addr] = final_color;
				}
			}
		}
	}
}

void ppu_cleanup(nes_ppu_t *ppu)
{
	
}