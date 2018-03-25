#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <wchar.h>
#include <locale.h>
#include <float.h>

/* config: */

//#define IRC_MODE
#define HALF_RES
#define DO_DITHER
#define COLOUR_MODE

/* end config */

#define countof(x) (sizeof(x)/sizeof(*x))
#define MAX(a, b)  ((a)>(b)?(a):(b))
#define MIN(a, b)  ((a)<=(b)?(a):(b))

typedef uint8_t v4b __attribute__((vector_size(4)));
typedef uint8_t v8b __attribute__((vector_size(8)));
typedef float   v4  __attribute__((vector_size(16)));

/*
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
*/

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

struct glyph* get_nearest_glyph(uint64_t mask, bool* out_reverse){

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

	*out_reverse = reverse;
	return glyphs + best_idx;
}

#ifdef COLOUR_MODE
#ifdef IRC_MODE
float colour_dist(uint32_t one, uint32_t two){
	long r = abs((long)((one >> 16) & 0xFF) - (long)((two >> 16) & 0xFF));
	long g = abs((long)((one >>  8) & 0xFF) - (long)((two >>  8) & 0xFF));
	long b = abs((long)((one >>  0) & 0xFF) - (long)((two >>  0) & 0xFF));
	return (r+g+b)/3.0f;
}
#endif // IRC_MODE

void print_glyph_colour(uint32_t code, v4 fg, v4 bg){
	char str[MB_LEN_MAX] = {};
	wctomb(str, code);

	uint8_t ansi_fg = 16
		+ 36 * MIN(5, (int)(fg[0] * 6.0f))
		+ 6  * MIN(5, (int)(fg[1] * 6.0f))
		+      MIN(5, (int)(fg[2] * 6.0f));

	uint8_t ansi_bg = 16
		+ 36 * MIN(5, (int)(bg[0] * 6.0f))
		+ 6  * MIN(5, (int)(bg[1] * 6.0f))
		+      MIN(5, (int)(bg[2] * 6.0f));

#ifdef IRC_MODE
	static const uint32_t irc_colours[] = {
		0xffffff, 0x000000, 0x00007f, 0x009300, 0xff0000, 0x7f0000, 0x9c009c, 0xfc7f00, 0xffff00, 0x00fc00, 0x009393, 0x00ffff,
		0x0000fc, 0xff00ff, 0x7f7f7f, 0xd2d2d2,
		0x470000, 0x472100, 0x474700, 0x324700, 0x004700, 0x00472c, 0x004747, 0x002747, 0x000047, 0x2e0047, 0x470047, 0x47002a,
		0x740000, 0x743a00, 0x747400, 0x517400, 0x007400, 0x007449, 0x007474, 0x004074, 0x000074, 0x4b0074, 0x740074, 0x740045,
		0xb50000, 0xb56300, 0xb5b500, 0x7db500, 0x00b500, 0x00b571, 0x00b5b5, 0x0063b5, 0x0000b5, 0x7500b5, 0xb500b5, 0xb5006b,
		0xff0000, 0xff8c00, 0xffff00, 0xb2ff00, 0x00ff00, 0x00ffa0, 0x00ffff, 0x008cff, 0x0000ff, 0xa500ff, 0xff00ff, 0xff0098,
		0xff5959, 0xffb459, 0xffff71, 0xcfff60, 0x6fff6f, 0x65ffc9, 0x6dffff, 0x59b4ff, 0x5959ff, 0xc459ff, 0xff66ff, 0xff59bc,
		0xff9c9c, 0xffd39c, 0xffff9c, 0xe2ff9c, 0x9cff9c, 0x9cffdb, 0x9cffff, 0x9cd3ff, 0x9c9cff, 0xdc9cff, 0xff9cff, 0xff94d3,
		0x000000, 0x131313, 0x282828, 0x363636, 0x4d4d4d, 0x656565, 0x818181, 0x9f9f9f, 0xbcbcbc, 0xe2e2e2, 0xffffff,
	};

	static const uint8_t irc_map[256] = {
		[ 52]=16, [ 94]=17, [100]=18, [ 58]=19, [ 22]=20, [ 29]=21, [ 23]=22, [ 24]=23, [ 17]=24, [ 54]=25, [ 53]=26, [ 89]=27,
		[ 88]=28, [130]=29, [142]=30, [ 64]=31, [ 28]=32, [ 35]=33, [ 30]=34, [ 25]=35, [ 18]=36, [ 91]=37, [ 90]=38, [125]=39,
		[124]=40, [166]=41, [184]=42, [106]=43, [ 34]=44, [ 49]=45, [ 37]=46, [ 33]=47, [ 19]=48, [129]=49, [127]=50, [161]=51,
		[196]=52, [208]=53, [226]=54, [154]=55, [ 46]=56, [ 86]=57, [ 51]=58, [ 75]=59, [ 21]=60, [171]=61, [201]=62, [198]=63,
		[203]=64, [215]=65, [227]=66, [191]=67, [ 83]=68, [122]=69, [ 87]=70, [111]=71, [ 63]=72, [177]=73, [207]=74, [205]=75,
		[217]=76, [223]=77, [229]=78, [193]=79, [157]=80, [158]=81, [159]=82, [153]=83, [147]=84, [183]=85, [219]=86, [212]=86,
	};

	int i_fg = irc_map[ansi_fg], i_bg = irc_map[ansi_bg];

	if(i_fg == 0){
		uint8_t a_r = MIN(5, (int)(fg[0] * 6.0f));
		uint8_t a_g = MIN(5, (int)(fg[1] * 6.0f));
		uint8_t a_b = MIN(5, (int)(fg[2] * 6.0f));

		static const int fudge[] = { 0, +1, -1 };

		for(int a = 0; a < 3; ++a){
			for(int b = 0; b < 3; ++b){
				for(int c = 0; c < 3; ++ c){
					uint8_t new_ansi = 16
						+ 36 * (a_r + fudge[a])
						+  6 * (a_g + fudge[b])
						+      (a_b + fudge[c]);

					if(irc_map[new_ansi] != 0){
						i_fg = irc_map[new_ansi];
						goto done_fg;
					}

				}
			}
		}
	}
done_fg:;
	if(i_bg == 0){
		uint8_t a_r = MIN(5, (int)(bg[0] * 6.0f));
		uint8_t a_g = MIN(5, (int)(bg[1] * 6.0f));
		uint8_t a_b = MIN(5, (int)(bg[2] * 6.0f));

		static const int fudge[] = { 0, -1, +1 };

		for(int a = 0; a < 3; ++a){
			for(int b = 0; b < 3; ++b){
				for(int c = 0; c < 3; ++ c){
					uint8_t new_ansi = 16
						+ 36 * (a_r + fudge[a])
						+  6 * (a_g + fudge[b])
						+      (a_b + fudge[c]);

					if(irc_map[new_ansi] != 0){
						i_bg = irc_map[new_ansi];
						goto done_bg;
					}

				}
			}
		}
	}
done_bg:;

	float b_fg = i_fg ? 0 : FLT_MAX, b_bg = i_bg ? 0 : FLT_MAX;

	uint32_t hex_fg = 
		(MIN(255, (int)(fg[0] * 256.0f)) << 16) |
		(MIN(255, (int)(fg[1] * 256.0f)) << 8) |
		(MIN(255, (int)(fg[2] * 256.0f)));

	uint32_t hex_bg = 
		(MIN(255, (int)(bg[0] * 256.0f)) << 16) |
		(MIN(255, (int)(bg[1] * 256.0f)) << 8) |
		(MIN(255, (int)(bg[2] * 256.0f)));

	for(int i = 0; i < countof(irc_colours); ++i){
		uint32_t c = irc_colours[i];
		float fg_diff = colour_dist(c, hex_fg);
		float bg_diff = colour_dist(c, hex_bg);

		if(fg_diff < b_fg){
			b_fg = fg_diff;
			i_fg = i;
		}

		if(bg_diff < b_bg){
			b_bg = bg_diff;
			i_bg = i;
		}
	}

	printf("\003%d,%d%s", i_fg, i_bg, str);
#else
	printf("\e[38;5;%d;48;5;%dm%s\e[0m", ansi_fg, ansi_bg, str);
#endif // IRC_MODE
}
#endif // COLOUR_MODE

void print_glyph(uint32_t code, bool reverse){
	char str[MB_LEN_MAX] = {};
	wctomb(str, code);

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
	assert(setlocale(LC_CTYPE, "C.UTF-8") || setlocale(LC_CTYPE, "en_US.utf8") || setlocale(LC_CTYPE, ""));

	if(argc < 2){
		fprintf(stderr, "Usage: %s [file]\n", argv[0]);
		return 1;
	}

	nglyphs = countof(_glyphs) + 255;
	glyphs = calloc(nglyphs, sizeof(struct glyph));
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

#ifdef HALF_RES
	for(int y = 0; y + 1 < h; y+=2){
		for(int x = 0; x < w*4; ++x){
			in[y/2][x] = (int)(in[y][x] + in[y+1][x]) / 2;
		}
	}

	h /= 2;
#endif

	uint8_t (*out)[w*4] = malloc(w*h*4);
	uint8_t gs_min = 255, gs_maw = 0;

	for(int y = 0; y < h; ++y){
		for(int x = 0; x < w*4; x+=4){
			float gs
				= (in[y][x+0] / 255.0) * 0.2126
				+ (in[y][x+1] / 255.0) * 0.7152
				+ (in[y][x+2] / 255.0) * 0.0722;

			uint8_t v = (gs*255);

			memset(&out[y][x], v, 3);
			out[y][x+3] = 255;

			if(v < gs_min) gs_min = v;
			if(v > gs_maw) gs_maw = v;
		}
	}

	float scale = 255.0 / (float)(gs_maw - gs_min);

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

	//printf("min: %d, maw: %d\n", gs_min, gs_maw);

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

			bool reverse;
			struct glyph* g = get_nearest_glyph(mask, &reverse);

#ifdef COLOUR_MODE
			v4 fg = {}, bg = {};
			int fgcnt = 0;

			for(int i = 0; i < 8; ++i){
				for(int j = 0; j < 8; ++j){
					v8b c = *(v8b*)&in[y+i][x+j*4];
					v4 color = (v4)_mm_cvtpu16_ps((__m64)__builtin_ia32_punpcklbw((__v8qi)c, (__v8qi){}));

					if(g->mask & (UINT64_C(1) << (i*8+j))){
						fg += color;
						fgcnt++;
					} else {
						bg += color;
					}
				}
			}

			if(fgcnt) fg /= (float)fgcnt;
			if(fgcnt != 64)	bg /= (64.0f - fgcnt);

			fg /= 255.0f;
			bg /= 255.0f;

			print_glyph_colour(g->code, fg, bg);
#else
			print_glyph(g->code, reverse);
#endif
		}

		puts("");
	}

	//stbi_write_png("debug.png", w, h, 4, out, w*4);

	return 0;
}
