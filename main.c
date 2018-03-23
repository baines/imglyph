#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <wchar.h>
#include <locale.h>

//#define IRC_MODE
#define HALF_RES
#define DO_DITHER

#define countof(x) (sizeof(x)/sizeof(*x))

static const uint8_t dither[8][8] = {
	{ 0 , 48, 12, 60, 3 , 51, 15, 63 },
	{ 32, 16, 44, 28, 35, 19, 47, 31 },
	{ 8 , 56, 4 , 52, 11, 59, 7 , 55 },
	{ 40, 24, 36, 20, 43, 27, 39, 23 },
	{ 2 , 50, 14, 62, 1 , 49, 13, 61 },
	{ 34, 18, 46, 30, 33, 17, 45, 29 },
	{ 10, 58, 6 , 54, 9 , 57, 5 , 53 },
	{ 42, 26, 38, 22, 41, 25, 37, 21 },
};

// "braille dither"
static const uint8_t dither2[8][8] = {
	{ 0 , 0 , 16, 0 , 0 , 16, 0 , 0 },
	{ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },
	{ 0 , 0 , 16, 0 , 0 , 16, 0 , 0 },
	{ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },
	{ 0 , 0 , 16, 0 , 0 , 16, 0 , 0 },
	{ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },
	{ 0 , 0 , 16, 0 , 0 , 16, 0 , 0 },
	{ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },
};

static const struct glyph {
	uint32_t code;
	uint64_t mask;
} _glyphs[] = {

	// use mkmask.c to add new ones

	{ 0x2580, UINT64_C(0xffffffff) },
	{ 0x2581, UINT64_C(0xff00000000000000) },
	{ 0x2582, UINT64_C(0xffff000000000000) },
	{ 0x2583, UINT64_C(0xffffff0000000000) },
	{ 0x2584, UINT64_C(0xffffffff00000000) },
	{ 0x2585, UINT64_C(0xffffffffff000000) },
	{ 0x2586, UINT64_C(0xffffffffffff0000) },
	{ 0x2587, UINT64_C(0xffffffffffffff00) },
	
	{ 0x2588, UINT64_C(0xffffffffffffffff) },


	{ 0x2589, UINT64_C(0x7f7f7f7f7f7f7f7f) },
	{ 0x258a, UINT64_C(0x3f3f3f3f3f3f3f3f) },
	{ 0x258b, UINT64_C(0x1f1f1f1f1f1f1f1f) },
	{ 0x258c, UINT64_C(0x0f0f0f0f0f0f0f0f) },
	{ 0x258d, UINT64_C(0x0707070707070707) },
	{ 0x258e, UINT64_C(0x0303030303030303) },
	{ 0x258f, UINT64_C(0x0101010101010101) },
	{ 0x2590, UINT64_C(0xf0f0f0f0f0f0f0f0) },
	{ 0x2591, UINT64_C(0x8822008822008822) }, //
	{ 0x2592, UINT64_C(0xaa55aa55aa55aa55) }, //
	{ 0x2593, UINT64_C(0xbbeeffbbeeffbbee) }, //
	{ 0x2594, UINT64_C(0xff) },
	{ 0x2595, UINT64_C(0x8080808080808080) },
	{ 0x2596, UINT64_C(0x0f0f0f0f00000000) },
	{ 0x2597, UINT64_C(0xf0f0f0f000000000) },
	{ 0x2598, UINT64_C(0x0f0f0f0f) },
	{ 0x2599, UINT64_C(0xffffffff0f0f0f0f) },
	{ 0x259a, UINT64_C(0xf0f0f0f00f0f0f0f) },
	{ 0x259b, UINT64_C(0x0f0f0f0fffffffff) },
	{ 0x259c, UINT64_C(0xf0f0f0f0ffffffff) },
	{ 0x259d, UINT64_C(0x0f0f0f0f0) },
	{ 0x259e, UINT64_C(0xf0f0f0ff0f0f0f0) },
	{ 0x259f, UINT64_C(0xfffffffff0f0f0f0) },

	/*
	{ 0x25a0, UINT64_C(0x007e7e7e7e7e7e00) },
	{ 0x25a1, UINT64_C(0x007e424242427e00) },
	{ 0x25a2, UINT64_C(0x003c424242423c00) },
	{ 0x25a3, UINT64_C(0x007e425a5a427e00) },
	{ 0x25aa, UINT64_C(0x3c3c3c3c0000) },
	{ 0x25ab, UINT64_C(0x3c24243c0000) },
	{ 0x25ac, UINT64_C(0x7e7e7e7e0000) },
	{ 0x25ad, UINT64_C(0x7e42427e0000) },
	{ 0x25ae, UINT64_C(0x3c3c3c3c3c3c00) },
	{ 0x25af, UINT64_C(0x3c242424243c00) },
	{ 0x25b0, UINT64_C(0x1e3e7c780000) },
	{ 0x25b1, UINT64_C(0x1e2244780000) },
	{ 0x25b2, UINT64_C(0xffffff7e7e3c3c18) },
	{ 0x25b3, UINT64_C(0xff81814242242418) },
	{ 0x25b4, UINT64_C(0x7e7e3c180000) },
	{ 0x25b5, UINT64_C(0x7e4224180000) },
	{ 0x25b6, UINT64_C(0x71f7fffff7f1f07) },
	{ 0x25b7, UINT64_C(0x719618181611907) },
	{ 0x25b8, UINT64_C(0xc1c3c3c1c0c00) },
	{ 0x25b9, UINT64_C(0xc142424140c00) },*/


	{ 0x2501, UINT64_C(0xffff000000) },
	{ 0x2503, UINT64_C(0x1818181818181818) },
	{ 0x250f, UINT64_C(0x181818f8f8000000) },
	{ 0x2513, UINT64_C(0x1818181f1f000000) },
	{ 0x2517, UINT64_C(0xf8f8181818) },
	{ 0x251b, UINT64_C(0x1f1f181818) },
	{ 0x2523, UINT64_C(0x181818f8f8181818) },
	{ 0x252b, UINT64_C(0x1818181f1f181818) },
	{ 0x2533, UINT64_C(0x181818ffff000000) },
	{ 0x253b, UINT64_C(0xffff181818) },
	{ 0x254b, UINT64_C(0x181818ffff181818) },

	{ 0x2571, UINT64_C(0x102040810204080) },  //
	{ 0x2572, UINT64_C(0x8040201008040201) }, //
	{ 0x2573, UINT64_C(0x8142241818244281) }, //

	{ 0x2578, UINT64_C(0xf0f000000) },
	{ 0x2579, UINT64_C(0x18181818) },
	{ 0x257a, UINT64_C(0xf0f0000000) },
	{ 0x257b, UINT64_C(0x1818181800000000) },

	/*
	{ 0x25cf, UINT64_C(0x3c7e7e7e7e3c00) },
	{ 0x25d6, UINT64_C(0xc0e0e0e0e0c00) },
	{ 0x25d7, UINT64_C(0x30707070703000) },*/

	{ 0x2895, UINT64_C(0x2024040020240400) },
};

void gen_braille_glyphs(struct glyph* g){
	for(int i = 0; i < 0xff; ++i){
		g[i].code = 0x2801 + i;

		uint8_t m = i+1;
		uint64_t mask = 0;

		if(m & 0x01) mask |= UINT64_C(0x4);
		if(m & 0x02) mask |= UINT64_C(0x40000);
		if(m & 0x04) mask |= UINT64_C(0x400000000);
		if(m & 0x08) mask |= UINT64_C(0x20);
		if(m & 0x10) mask |= UINT64_C(0x200000);
		if(m & 0x20) mask |= UINT64_C(0x2000000000);
		if(m & 0x40) mask |= UINT64_C(0x4000000000000);
		if(m & 0x80) mask |= UINT64_C(0x20000000000000);

		g[i].mask = mask;
	}
}

struct glyph* glyphs;
size_t nglyphs;

void print_nearest(uint64_t mask){

	int best_idx = 0;
	int best_pop = INT_MAX;
	bool reverse = false;

	for(int i = 0; i < nglyphs; ++i){
		const struct glyph* g = glyphs + i;

		uint64_t diff = mask ^ g->mask;
		int pop = __builtin_popcountll(diff);

		if(pop < best_pop){
			best_pop = pop;
			best_idx = i;
		}
	}

	for(int i = 0; i < nglyphs; ++i){
		const struct glyph* g = glyphs + i;

		uint64_t diff = mask ^ (g->mask ^ UINT64_MAX);
		int pop = __builtin_popcountll(diff);

		if(pop < best_pop){
			best_pop = pop;
			best_idx = i;
			reverse = true;
		}
	}

	char str[MB_LEN_MAX];
	wctomb(str, glyphs[best_idx].code);

#ifdef IRC_MODE
	if(reverse){
		printf("\0031,0%s", str);
	} else {
		printf("\0030,1%s", str);
	}
#else
	if(reverse){
		printf("\e[7m%s\e[0m", str);
	} else {
		printf("%s", str);
	}
#endif
}

int main(int argc, char** argv){

	// hope you've got a utf-8 locale
	assert(setlocale(LC_CTYPE, "C.UTF-8") || setlocale(LC_CTYPE, "en_US.utf8"));

	if(argc < 2){
		fprintf(stderr, "Usage: %s [file]\n", argv[0]);
		return 1;
	}

	nglyphs = countof(_glyphs) + 255;
	glyphs = malloc(sizeof(_glyphs) + (255 * sizeof(struct glyph)));
	memcpy(glyphs, _glyphs, sizeof(_glyphs));
	gen_braille_glyphs(glyphs + countof(_glyphs));

	int w, h;
	void* img = stbi_load(argv[1], &w, &h, NULL, 4);
	if(!img){
		fprintf(stderr, "stbi_load returns null...\n");
		return 1;
	}

	//printf("w: %d, h: %d\n", w, h);

	uint8_t (*in)[w*4] = img;
	uint8_t (*out)[w*4] = malloc(w*h*4);
	uint8_t gs_min = 255, gs_maw = 0;

	for(int i = 0; i < h; ++i){
		for(int j = 0; j < w*4; j+=4){
			float gs
				= (in[i][j+0] / 255.0) * 0.2126
				+ (in[i][j+1] / 255.0) * 0.7152
				+ (in[i][j+2] / 255.0) * 0.0722;

			uint8_t v = (gs*255);

			memset(&out[i][j], v, 3);
			out[i][j+3] = 255;

			if(v < gs_min) gs_min = v;
			if(v > gs_maw) gs_maw = v;
		}
	}

	float scale = 255.0 / (float)(gs_maw - gs_min);

#ifdef HALF_RES
	uint8_t (*half)[w*4] = malloc(w*h*2);
	for(int y = 0; y + 1 < h; y+=2){
		for(int x = 0; x < w*4; x+=4){
			uint32_t v
				= (int)((out[y+0][x] - gs_min) * scale
				+  (out[y+1][x] - gs_min) * scale) >> 1;

#ifdef DO_DITHER
			//if((v >> 2) > dither[(y/2)%8][(x/4)%8]){
			v >>= 2;
			uint8_t d = dither2[(y/2)%8][(x/4)%8];
			if( (v >= 32 && (v - 32) >= d) ||
				(v <= 31 && (31 - v) <= d)){
#else
			if(v >= 127){
#endif
				v = 255;
			} else {
				v = 0;
			}

			memset(&half[y/2][x], v, 3);
			half[y/2][x+3] = 255;
		}
	}

	free(out);
	out = half;

	h /= 2;
#else
	for(int y = 0; y < h; ++y){
		for(int x = 0; x < w*4; x+=4){
			uint32_t v = (int)((out[y][x] - gs_min) * scale);

#ifdef DO_DITHER
			//if((v >> 2) > dither[y%8][(x/4)%8]){
			v >>= 2;
			uint8_t d = dither2[y%8][(x/4)%8];
			if( (v >= 32 && (v - 32) >= d) ||
				(v <= 31 && (31 - v) <= d)){
#else
			if(v >= 127){
#endif
				v = 255;
			} else {
				v = 0;
			}

			memset(&out[y][x], v, 3);
		}
	}

#endif

	//printf("min: %d, maw: %d\n", gs_min, gs_maw);

	int cnt = 0;

	for(int y = 0; y + 8 < h; y += 8){
		for(int x = 0; x + 32 < (w*4); x += (8*4)){
			uint64_t mask = 0;

			for(int i = 0; i < 8; ++i){
				for(int j = 0; j < 8; ++j){
					if(out[y+i][x+j*4] == 255){
						mask |= (UINT64_C(1) << (i*8+j));
					}
				}
			}

			//fprintf(stderr, "mask %d = %#" PRIx64 "\n", cnt++, mask);
			print_nearest(mask);
		}

		puts("");
	}

	//stbi_write_png("debug.png", w, h, 4, out, w*4);

	return 0;
}
