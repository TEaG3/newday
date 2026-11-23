// The themes' table and the scenes of ten of them (v1.3; the prototype "Temas NULD en vivo"). What the console
// taught with Torres (2026-11-18) holds for all: a full-screen picture sent every frame costs 3 ms, so the dithered
// backdrops are a 64-px strip repeated (vertical gradients) or a left half mirrored (radial ones); libm's sinf is
// software on the EE, so what moves uses fsin, a parabola; and the lights are added (gfx_add), never sorted.
// ponytail: only Torres and Classic was timed on a console; the others are sized by the same rules and PCSX2.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <kernel.h>
#include "gfx.h"
#include "ui_data.h"
#include "splash.h"
#include "torres.h"
#include "theme.h"
#include "ini.h"
#include "cover.h"
#include <dirent.h>

#define TAU 6.2831853f
#define W GFX_W
#define H GFX_H
#define WHITE 0xFFFFFF

static float now; // the scenes' clock, seconds
static int cur;   // the theme in use (themes_enter)

static void tick(void) { now += 1 / 60.f; }
static float clampf(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
static float ease(float k) { k = clampf(k); return k * k * (3 - 2 * k); }
static float mix(float a, float b, float k) { return a + (b - a) * k; }
static float frac(float x) { x -= (int)x; return x < 0 ? x + 1 : x; }
static float fsin(float x) { x = frac(x * (1 / TAU)); return x < 0.5f ? 16 * x * (0.5f - x) : -16 * (x - 0.5f) * (1 - x); }
static float fcos(float x) { return fsin(x + TAU / 4); }
static float saw(float x) { return 2 * (frac(x + 0.5f) - 0.5f); }
static float rnd(int i, int j) // 0..1, fixed per (i, j)
{
	unsigned h = (unsigned)i * 374761393u + (unsigned)j * 668265263u;
	h = (h ^ h >> 13) * 1274126177u;
	return ((h ^ h >> 16) & 0xFFFF) / 65536.f;
}
static unsigned rgb3(float r, float g, float b)
{
	r = r < 0 ? 0 : r > 255 ? 255 : r, g = g < 0 ? 0 : g > 255 ? 255 : g, b = b < 0 ? 0 : b > 255 ? 255 : b;
	return (unsigned)r << 16 | (unsigned)g << 8 | (unsigned)b;
}
static unsigned scale(unsigned c, float k) { k = clampf(k); return rgb3((c >> 16 & 255) * k, (c >> 8 & 255) * k, (c & 255) * k); }
static unsigned blend(unsigned a, unsigned b, float k)
{
	k = clampf(k);
	return rgb3(mix(a >> 16 & 255, b >> 16 & 255, k), mix(a >> 8 & 255, b >> 8 & 255, k), mix(a & 255, b & 255, k));
}
// drum and bass at 174 BPM: kick on beats 0 and 2.5, snare on 1 and 3; 1 at the hit, falling off
static float hit(float b, float r) { float v = 1 / (1 + r * b * 0.4f); v *= v; return v * v; }
static float beat4(float t) { return 4 * frac(t * 174 / 240); }
static float kick(float t)
{
	float b = beat4(t), k = hit(b, 5), k2 = hit(4 * frac((b + 1.5f) / 4), 5), s = 0.6f * hit(4 * frac((b + 3) / 4), 7), s2 = 0.6f * hit(4 * frac((b + 1) / 4), 7);
	k = k > k2 ? k : k2, s = s > s2 ? s : s2;
	return k > s ? k : s;
}

static void lights_on(void) { gfx_dither(1), gfx_add(1), gfx_alpha(0x80); }
static void lights_off(void) { gfx_add(0), gfx_dither(0), gfx_alpha(0x80); }
static void light(float x, float y, float s, float a, unsigned rgb) // a soft light with a white heart, radius s
{
	int al = (int)(0x80 * clampf(a)), r = (int)s, c = (int)(s * 0.45f);
	if (al < 1 || r < 1) return;
	gfx_alpha(al);
	gfx_glow((int)x - r, (int)y - r, 2 * r, 2 * r, rgb, rgb);
	if (c > 0) gfx_glow((int)x - c, (int)y - c, 2 * c, 2 * c, WHITE, WHITE);
}
static void halo(float x, float y, float w, float h, float a, unsigned rgb, float grow) // a glow (or a shadow) behind a box
{
	gfx_dither(1), gfx_alpha((int)(0x80 * clampf(a)));
	gfx_glow((int)(x - grow), (int)(y - grow), (int)(w + 2 * grow), (int)(h + 2 * grow), rgb, rgb);
	gfx_dither(0), gfx_alpha(0x80);
}
static void frame(float x, float y, float w, float h, int t, unsigned rgb, float a) // a t-px frame, inwards from the box
{
	int X = (int)x, Y = (int)y, Wd = (int)w, Hg = (int)h;
	gfx_alpha((int)(0x80 * clampf(a)));
	gfx_rect(X, Y, Wd, t, rgb), gfx_rect(X, Y + Hg - t, Wd, t, rgb);
	gfx_rect(X, Y + t, t, Hg - 2 * t, rgb), gfx_rect(X + Wd - t, Y + t, t, Hg - 2 * t, rgb);
	gfx_alpha(0x80);
}
static void wordmark(float cx, float cy, float cap, unsigned ink, unsigned slash, float a) // the mark and TEa.G3 in Michroma
{
	float r = cap * 0.6875f, gap = cap * 0.42f, x = cx - (2 * r + gap + splash_vtext_w(&vfont_michroma, cap, 0.06f, "TEa.G3")) / 2;
	if (a <= 0.01f) return;
	gfx_alpha((int)(0x80 * clampf(a)));
	splash_mark(x + r, cy, r, ink, slash);
	splash_vtext(&vfont_michroma, x + 2 * r + gap, cy + cap / 2, cap, 0.06f, "TEa.G3", ink);
	gfx_alpha(0x80);
}

// ---- a dithered backdrop, one at a time: a left half to mirror (1) or a 64-px strip to repeat (2) ----
static unsigned short *pic;
static int pic_of = -1, pic_kind;
static void picture(int theme, int kind, gfx_color_fn col)
{
	if (pic_of == theme) return;
	pic_of = -1;
	if (!pic && !(pic = memalign(64, W / 2 * H * 2))) return; // no memory: the theme's flat colour
	if (kind == 1) {
		unsigned short *lin = malloc(W / 2 * H * 2);
		if (!lin) return;
		gfx_fs_dither(lin, W / 2, H, col, NULL);
		gfx_tiles_from(pic, lin, W / 2, H);
		free(lin);
	} else gfx_fs_dither(pic, 64, H, col, NULL);
	SyncDCache(pic, pic + W / 2 * H);
	pic_of = theme, pic_kind = kind;
}
static void backdrop(int theme, unsigned flat)
{
	gfx_alpha(0x80), gfx_dither(0);
	if (pic_of != theme) gfx_rect(0, 0, W, H, flat);
	else if (pic_kind == 1) gfx_image_mirrored(pic, W / 2, H);
	else gfx_hstrip(pic, 64, H, 0);
}
static void stops(const float (*s)[4], int n, float k, float *c) // a gradient: rows of {position, r, g, b}
{
	int i = 1;
	while (i < n - 1 && k > s[i][0]) i++;
	float t = clampf((k - s[i - 1][0]) / (s[i][0] - s[i - 1][0]));
	for (int j = 0; j < 3; j++) c[j] = mix(s[i - 1][j + 1], s[i][j + 1], t);
}

// ---- the selected cover, for the themes that take from it: a tiny copy to enlarge (out of focus) and its colour ----
#define BW 32
#define BH 46
static unsigned short *blur[2]; // [0] the cover, [1] the one before it (they cross-fade)
static int has_blur[2];
static unsigned cov_id;
static float blur_t = -9, acc[3] = {255, 180, 84}, acc_goal[3] = {255, 180, 84};

void themes_cover(const unsigned short *pix, int w, int h)
{
	unsigned id = (unsigned)pix ^ pix[w * h / 2] << 16 ^ pix[w * h / 3] ^ (unsigned)w << 8;
	if (id == cov_id || w < BW || h < BH) return;
	if (!blur[0] && (!(blur[0] = memalign(64, BW * BH * 2)) || !(blur[1] = memalign(64, BW * BH * 2)))) return;
	cov_id = id;
	unsigned short *t = blur[0];
	blur[0] = blur[1], blur[1] = t, has_blur[1] = has_blur[0], has_blur[0] = 1, blur_t = now;
	float sr = 0, sg = 0, sb = 0, sw = 0;
	for (int y = 0; y < BH; y++)
		for (int x = 0; x < BW; x++) {
			int x0 = x * w / BW, x1 = (x + 1) * w / BW, y0 = y * h / BH, y1 = (y + 1) * h / BH, r = 0, g = 0, b = 0, n = 0;
			for (int v = y0; v < y1; v++)
				for (int u = x0; u < x1; u++) { unsigned p = pix[v * w + u]; r += p & 31, g += p >> 5 & 31, b += p >> 10 & 31, n++; }
			r /= n, g /= n, b /= n;
			blur[0][y * BW + x] = 0x8000 | b << 10 | g << 5 | r;
			int hi = r > g ? (r > b ? r : b) : (g > b ? g : b), lo = r < g ? (r < b ? r : b) : (g < b ? g : b);
			float s = hi ? (hi - lo) / (float)hi : 0, k = s * s * hi / 31 + 1e-4f; // the saturated, bright cells count
			sr += r * 8 * k, sg += g * 8 * k, sb += b * 8 * k, sw += k;
		}
	SyncDCache(blur[0], blur[0] + BW * BH);
	sr /= sw, sg /= sw, sb /= sw; // the cover's dominant colour, pushed to a light, vivid tone
	float hi = sr > sg ? (sr > sb ? sr : sb) : (sg > sb ? sg : sb), lo = sr < sg ? (sr < sb ? sr : sb) : (sg < sb ? sg : sb);
	if (hi < 1) hi = 1;
	float sat = (hi - lo) / hi, push = sat < 0.5f ? (0.5f / (sat + 0.01f) > 3 ? 3 : 0.5f / (sat + 0.01f)) : 1, c[3] = {sr, sg, sb};
	for (int j = 0; j < 3; j++) {
		float v = (hi - (hi - c[j]) * push) * 235 / hi;
		v = v < 0 ? 0 : v > 255 ? 255 : v;
		acc_goal[j] = v + (255 - v) * 0.12f;
	}
}
unsigned themes_accent(void)
{
	for (int j = 0; j < 3; j++) acc[j] += (acc_goal[j] - acc[j]) * 0.08f;
	return rgb3(acc[0], acc[1], acc[2]);
}
static void blurred(float a, float zoom) // the cover out of focus over the whole screen, drifting; the old one fades under it
{
	float w = W * 1.25f * zoom, h = w * BH / BW, x = (W - w) / 2 + fsin(now * 0.07f) * 50, y = (H - h) / 2 + fcos(now * 0.05f) * 70;
	float k = ease((now - blur_t) / 0.5f);
	gfx_dither(1);
	if (has_blur[1] && k < 1) gfx_alpha((int)(0x80 * a)), gfx_image(blur[1], BW, BH, (int)x, (int)y, (int)w, (int)h);
	if (has_blur[0]) gfx_alpha((int)(0x80 * a * (has_blur[1] ? k : 1))), gfx_image(blur[0], BW, BH, (int)x, (int)y, (int)w, (int)h);
	gfx_dither(0), gfx_alpha(0x80);
}

// ---- Classic (former NULD theme): splash.c's floor, corners and scanlines ----
static void nuld_bg(float turn) { (void)turn; tick(); splash_floor(now * 0.5f, NULD_FG, 0x12); }
static void nuld_sel(float x, float y, float w, float h, float f, int front)
{
	int g = (int)(12 * w / 256);
	if (front) return;
	gfx_alpha((int)(0x80 * f));
	splash_hud((int)x - g, (int)y - g, (int)w + 2 * g, (int)h + 2 * g, (int)(30 * w / 256), themes[cur].pal.accent);
	gfx_alpha(0x80);
}
static void nuld_launch(float u) { splash_floor(u, NULD_FG, 0x12); }

// ---- Cristal: the game's cover, out of focus, is the ground; panes of glass; the accent comes from the cover ----
static void cristal_bg(float turn)
{
	(void)turn;
	tick();
	gfx_alpha(0x80), gfx_rect(0, 0, W, H, 0x0B0A14);
	blurred(1, 1);
	gfx_alpha(0x38), gfx_rect(0, 0, W, H, 0x090812);
	gfx_dither(1);
	gfx_fade(0, 0, W, 216, 0x06050E, 0x46, 0), gfx_fade(0, 490, W, 230, 0x06050E, 0, 0x5A);
	gfx_dither(0), gfx_alpha(0x80);
}
static void cristal_sel(float x, float y, float w, float h, float f, int front)
{
	unsigned a = rgb3(acc[0], acc[1], acc[2]);
	if (!front) halo(x, y, w, h, f * 0.6f * (0.75f + 0.25f * fsin(now * 2)), a, 90 * w / 256);
	else frame(x - 3, y - 3, w + 6, h + 6, 2, a, f);
}
static float cristal_splash(float u)
{
	static const unsigned col[5] = {0xFFB454, 0x7C5CFF, 0x2EC4FF, 0xFF5C8A, 0x4BE3A0};
	float k = ease(u / 0.7f);
	int y = 204 + (int)(14 * (1 - k));
	tick();
	gfx_alpha(0x80), gfx_rect(0, 0, W, H, 0x0B0A14);
	lights_on();
	gfx_alpha((int)(0x34 * ease(u / 1.5f)));
	for (int i = 0; i < 5; i++) // lights of the library's colours drifting behind the glass
		gfx_glow(380 + (int)(fsin(now * 0.13f + i * 1.7f) * 520), 100 + (int)(fcos(now * 0.11f + i * 2.3f) * 260), 520, 520, col[i], col[i]);
	lights_off();
	gfx_alpha((int)(0x58 * k)), gfx_rect(350, y, 580, 200, 0x15122A);
	frame(350, y, 580, 200, 1, 0x8A88A0, k);
	gfx_scissor(350, y, 929, y + 199);
	lights_on(); // a light passing over the plate
	gfx_alpha((int)(0x1C * k)), gfx_glow(100 + (int)(frac(u * 0.25f) * 1300) - 100, y - 60, 200, 320, WHITE, WHITE);
	lights_off();
	gfx_scissor(0, 0, W - 1, H - 1);
	wordmark(640, y + 100, 60, WHITE, 0xFFB454, k);
	return k;
}
static void cristal_launch(float u)
{
	tick();
	gfx_alpha(0x80), gfx_rect(0, 0, W, H, 0x0B0A14);
	blurred(1, 1 + u * 0.05f);
	gfx_alpha(0x48), gfx_rect(0, 0, W, H, 0x090812), gfx_alpha(0x80);
}

// ---- Aurora: curtains of light waving over a sky with stars ----
static void aurora_col(int x, int y, float *c, void *u)
{
	(void)x, (void)u;
	c[0] = mix(3, 10, y / 719.f), c[1] = mix(5, 18, y / 719.f), c[2] = mix(15, 38, y / 719.f);
}
static void aurora_init(void) { picture(TH_AURORA, 2, aurora_col); }
static void aurora_sky(float fx) // fx: how much of the curtains there is (0: none yet, over 1: a flare)
{
	static const struct { unsigned c, hot; float y, h, a; } cur[3] = {{0x1EF0AA, 0x78FFE6, 606, 470, 0.72f}, {0x7850FF, 0xD28CFF, 566, 430, 0.62f},
	                                                                 {0xFF50A0, 0xFF8CDC, 624, 360, 0.34f}};
	backdrop(TH_AURORA, 0x060B1A);
	for (int i = 0; i < 60; i++) { // stars
		gfx_alpha((int)(0x80 * (0.25f + 0.45f * fabsf(fsin(now * (0.4f + rnd(i, 7)) + i)))));
		gfx_rect((int)(rnd(i, 8) * W), (int)(rnd(i, 9) * 520), 1 + (i % 7 == 0), 1 + (i % 7 == 0), 0xDCE6FF);
	}
	lights_on();
	for (int ci = 0; ci < 3; ci++) { // each curtain: three bands between four lines that wave together
		static float ys[4][65], xy[260];
		static unsigned cs[4][65], sc[130];
		for (int n = 0; n < 65; n++) {
			float x = n * 20, wob = fsin(x * 0.0041f + now * 0.21f + ci * 2.1f) + 0.5f * fsin(x * 0.0113f - now * 0.17f + ci) + 0.25f * fsin(x * 0.031f + now * 0.4f);
			float hh = cur[ci].h * (0.75f + 0.25f * fsin(x * 0.007f + now * 0.13f + ci * 4)), fold = 0.5f + 0.5f * fsin(x * 0.017f + now * 0.5f + ci * 3);
			float a = cur[ci].a * fx * (0.3f + 0.7f * fold * fold), yb = cur[ci].y + wob * 46;
			ys[0][n] = yb - hh, ys[1][n] = yb - hh * 0.28f, ys[2][n] = yb - hh * 0.07f, ys[3][n] = yb;
			cs[0][n] = cs[3][n] = 0, cs[1][n] = scale(cur[ci].c, a * 0.5f), cs[2][n] = scale(cur[ci].hot, a * 0.95f);
		}
		for (int b = 0; b < 3; b++) {
			for (int n = 0; n < 65; n++)
				xy[4 * n] = xy[4 * n + 2] = n * 20, xy[4 * n + 1] = ys[b][n], xy[4 * n + 3] = ys[b + 1][n], sc[2 * n] = cs[b][n], sc[2 * n + 1] = cs[b + 1][n];
			gfx_strip(xy, sc, 130);
		}
	}
	lights_off();
}
static void aurora_bg(float turn) { (void)turn; tick(); aurora_sky(1); }
static void aurora_sel(float x, float y, float w, float h, float f, int front)
{
	if (!front) halo(x, y, w, h, f * 0.4f * (0.8f + 0.2f * fsin(now * 1.6f)), 0x7CF5D6, 80 * w / 256);
	else frame(x - 1, y - 1, w + 2, h + 2, 1, WHITE, f * 0.75f);
}
static float aurora_splash(float u)
{
	float k = ease((u - 1) / 1.2f);
	tick();
	aurora_sky(ease(u / 3.5f)); // the curtains come up out of a dark sky
	wordmark(640, 300, 72, 0xEEF2FF, 0x7CF5D6, k);
	return k;
}
static void aurora_launch(float u)
{
	tick();
	aurora_sky(1 + ease(u / 0.8f) * 0.8f); // they flare
	gfx_alpha((int)(0x80 * 0.35f * ease(u))), gfx_rect(0, 0, W, H, 0x03050F), gfx_alpha(0x80);
}

// ---- Liquid: a huge sun setting over the water, behind the game you are on ----
#define HZ 522
static float sun_x = 640;
static void liquid_col(int x, int y, float *c, void *u)
{
	static const float sky[6][4] = {{0, 5, 14, 36}, {0.3f, 14, 47, 96}, {0.55f, 91, 63, 143}, {0.78f, 217, 88, 138}, {0.93f, 255, 158, 110}, {1, 255, 208, 154}};
	static const float sea[3][4] = {{0, 107, 58, 134}, {0.25f, 42, 31, 102}, {1, 6, 9, 32}};
	(void)x, (void)u;
	if (y < HZ) stops(sky, 6, y / (float)HZ, c);
	else stops(sea, 3, (y - HZ) / (float)(H - HZ), c);
}
static void liquid_init(void) { picture(TH_LIQUID, 2, liquid_col); }
static void liquid_scene(float sy) // sy: the sun's centre
{
	int sx = (int)mix(640, sun_x, 0.75f);
	float up = clampf((HZ + 262 - sy) / 262);
	backdrop(TH_LIQUID, 0x0E2F60);
	gfx_scissor(0, 0, W - 1, HZ - 1);
	lights_on();
	gfx_alpha(0x40), gfx_glow(sx - 520, (int)sy - 520, 1040, 1040, 0xFFBE82, 0xFF788C);
	lights_off();
	static float xy[132], rx[67], ry[67];
	static unsigned sc[66];
	for (int j = 0; j <= 32; j++) { // the sun: a disc in slices, cream at the top to coral at the bottom, and a soft rim
		float c = fcos(TAU / 2 * j / 32), s = sqrtf(1 - c * c) * 262, y = sy - c * 262;
		xy[4 * j] = sx - s, xy[4 * j + 2] = sx + s, xy[4 * j + 1] = xy[4 * j + 3] = y;
		sc[2 * j] = sc[2 * j + 1] = blend(0xFFF6D0, 0xFF6F7D, clampf((y - 100) / (HZ - 100)));
		rx[j] = sx + s, ry[j] = y, rx[65 - j] = sx - s, ry[65 - j] = y;
	}
	rx[66] = rx[0], ry[66] = ry[0];
	gfx_dither(1), gfx_alpha(0x80), gfx_strip(xy, sc, 66), gfx_ribbon(rx, ry, 67, 5, 0xFFC890, 0x60), gfx_dither(0);
	gfx_scissor(0, 0, W - 1, H - 1);
	for (int y = HZ + 4; y < 706 && up > 0; y += 5) { // the sun's road on the water
		float d = (y - HZ) / 184.f, w = (230 - d * 120) * (0.62f + 0.38f * fsin(y * 0.7f + now * 2.2f)) + 20;
		gfx_alpha((int)(0x80 * 0.5f * (1 - d * 0.8f) * up));
		gfx_rect(sx - (int)w, y, 2 * (int)w, 2, rgb3(255, 200 - d * 60, 150 + d * 20));
	}
	for (int i = 0; i < 6; i++) { // swell
		static float px[47], py[47];
		float y0 = HZ + 14 + i * 30;
		for (int n = 0; n < 47; n++) {
			px[n] = -10 + n * 28;
			py[n] = y0 + fsin(px[n] * (0.012f - i * 0.001f) + now * (0.9f + i * 0.13f) + i * 1.9f) * (4 + i * 1.6f) + fsin(px[n] * 0.031f - now * 0.6f + i) * 2;
		}
		gfx_ribbon(px, py, 47, 3.5f + i * 0.7f, rgb3(255 - i * 18, 170 + i * 6, 200 + i * 9), (int)(0x80 * (0.34f - i * 0.04f)));
	}
	gfx_alpha(0x40), gfx_rect(0, HZ, W, 1, 0xFFE1C8), gfx_alpha(0x80);
}
static void liquid_bg(float turn) { (void)turn; tick(); liquid_scene(360); }
static void liquid_sel(float x, float y, float w, float h, float f, int front)
{
	if (!front) return;
	sun_x += (x + w / 2 - sun_x) * 0.04f;
	frame(x - 2, y - 2, w + 4, h + 4, 2, WHITE, f);
}
static float liquid_splash(float u) // sunrise: the sun comes up out of the water and the night lifts
{
	float k = ease(u / 4.5f), w = ease((u - 1.4f) / 1.2f);
	tick();
	liquid_scene(mix(790, 360, k));
	gfx_alpha((int)(0x80 * 0.7f * (1 - k))), gfx_rect(0, 0, W, H, 0x04081A), gfx_alpha(0x80);
	wordmark(640, 596, 52, WHITE, 0x7FF0E0, w);
	return w;
}
static void liquid_launch(float u) // sunset
{
	float d = ease(u / 0.7f);
	tick();
	liquid_scene(mix(360, 800, d));
	gfx_alpha((int)(0x80 * 0.6f * d)), gfx_rect(0, 0, W, H, 0x04081A), gfx_alpha(0x80);
}

// ---- Onda: a ribbon of light flowing, over a ground of the cover's colour ----
static void onda_scene(const float *c)
{
	unsigned c1 = rgb3(c[0] * 0.16f, c[1] * 0.16f, c[2] * 0.16f), c2 = rgb3(c[0] * 0.5f, c[1] * 0.5f, c[2] * 0.5f), c3 = rgb3(c[0] * 0.22f, c[1] * 0.22f, c[2] * 0.22f);
	gfx_alpha(0x80), gfx_dither(1);
	gfx_grad(0, 0, W, 396, c1, c2, 1), gfx_grad(0, 396, W, 324, c2, c3, 1);
	lights_on();
	for (int i = 0; i < 6; i++) { // the ribbon: bands hanging from curves that drift, each with its bright edge
		static float px[65], py[65], xy[260];
		static unsigned sc[130];
		unsigned top = scale(WHITE, 0.16f + i * 0.015f);
		for (int n = 0; n < 65; n++) {
			float x = n * 20;
			px[n] = x, py[n] = 470 + i * 9 + fsin(x * 0.0042f + now * (0.32f + i * 0.03f) + i * 0.5f) * (70 - i * 6) + fsin(x * 0.0105f - now * 0.24f + i * 1.3f) * 26;
			xy[4 * n] = xy[4 * n + 2] = x, xy[4 * n + 1] = py[n], xy[4 * n + 3] = py[n] + 46 + i * 12 + fsin(x * 0.006f + now * 0.5f + i) * (26 + i * 5);
			sc[2 * n] = top, sc[2 * n + 1] = 0;
		}
		gfx_alpha(0x80), gfx_strip(xy, sc, 130);
		gfx_ribbon(px, py, 65, 2.5f, WHITE, (int)(0x80 * (0.3f - i * 0.035f)));
	}
	for (int i = 0; i < 26; i++) { // sparks riding it
		float x = W * frac(rnd(i, 2) + now * (18 + rnd(i, 3) * 30) / W);
		light(x, 480 + fsin(x * 0.0042f + now * 0.32f) * 70 + (rnd(i, 4) - 0.5f) * 90, 4 + rnd(i, 5) * 8, 0.25f + 0.5f * fabsf(fsin(now * 1.4f + i)), 0xBFD4FF);
	}
	lights_off();
}
static void onda_bg(float turn) { (void)turn; tick(); themes_accent(); onda_scene(acc); }
static void onda_sel(float x, float y, float w, float h, float f, int front)
{
	if (!front) halo(x, y, w, h, f * 0.55f * (0.7f + 0.3f * fsin(now * 2.2f)), WHITE, 60 * w / 256);
	else frame(x - 1, y - 1, w + 2, h + 2, 1, WHITE, f);
}
static float onda_splash(float u) // the ground drifts through every colour
{
	float hue = frac(now * 0.125f) * 12, c[3], k = ease((u - 0.6f) / 1);
	for (int j = 0; j < 3; j++) {
		float q = frac((j * 8 % 12 + hue) / 12) * 12, m = q - 3 < 9 - q ? q - 3 : 9 - q;
		c[j] = 255 * (0.5f - 0.5f * (m < -1 ? -1 : m > 1 ? 1 : m));
	}
	tick();
	onda_scene(c);
	wordmark(640, 280, 70, WHITE, 0xC8D2E6, k);
	return k;
}
static void onda_launch(float u) // a thread of light runs across
{
	int x = (int)(W * ease(u / 0.8f));
	tick(), themes_accent();
	onda_scene(acc);
	gfx_alpha(0x66), gfx_rect(0, 470, x, 1, WHITE);
	lights_on(), light(x, 470, 22, 1, 0xBFD4FF), lights_off();
}

// ---- Orbit: the original Y2K design: a blue night, ice and iris, orbits and sparkles ----
#define ICE 0x7FE7FF
#define IRIS 0xC08BFF
static float circ[65][2]; // a circle, once
static void orbit_col(int x, int y, float *c, void *u)
{
	static const float st[3][4] = {{0, 19, 35, 90}, {0.5f, 10, 18, 50}, {1, 4, 6, 14}};
	float dx = x - 640, dy = y - 420;
	(void)u;
	stops(st, 3, clampf(sqrtf(dx * dx + dy * dy) / 760), c);
}
static void orbit_init(void)
{
	for (int j = 0; j < 65; j++) circ[j][0] = cosf(-TAU / 4 + TAU * j / 64), circ[j][1] = sinf(-TAU / 4 + TAU * j / 64);
	picture(TH_ORBIT, 1, orbit_col);
}
static void orbit_scene(float cy, float u) // u < 0: all there; else the orbits are being drawn, u seconds in
{
	static const struct { float rx, ry, rot, sp; unsigned c; } o[3] = {{560, 150, -0.14f, 0.1f, ICE}, {690, 205, 0.1f, -0.07f, IRIS}, {460, 95, -0.05f, 0.16f, ICE}};
	backdrop(TH_ORBIT, 0x0A1232);
	lights_on();
	for (int i = 0; i < 3; i++) {
		static float px[65], py[65];
		float k = u < 0 ? 1 : ease((u - i * 0.45f) / 1.8f), rot = o[i].rot + fsin(now * 0.1f + i) * 0.03f, cr = fcos(rot), sr = fsin(rot);
		int n = 1 + (int)(64 * k);
		if (k <= 0) continue;
		for (int j = 0; j < n; j++)
			px[j] = 640 + circ[j][0] * o[i].rx * cr - circ[j][1] * o[i].ry * sr, py[j] = cy + circ[j][0] * o[i].rx * sr + circ[j][1] * o[i].ry * cr;
		gfx_ribbon(px, py, n, 9, o[i].c, 0x0D), gfx_ribbon(px, py, n, 2.5f, o[i].c, 0x34);
		for (int t = 0; t < 9; t++) { // its body, with a tail
			float a = u < 0 ? now * o[i].sp * 3 + i * 2 - t * 0.03f * (o[i].sp < 0 ? -1 : 1) : -TAU / 4 + TAU * k - t * 0.03f, ex = fcos(a) * o[i].rx, ey = fsin(a) * o[i].ry;
			light(640 + ex * cr - ey * sr, cy + ex * sr + ey * cr, (13 - t) * 1.6f, t ? 0.3f - t * 0.03f : 1, 0x5A96FF);
		}
	}
	float in = u < 0 ? 1 : ease(u / 2);
	for (int i = 0; i < 26; i++) { // four-point sparkles
		float x = (int)(rnd(i, 21) * W) + 0.5f, y = (int)(rnd(i, 22) * 600) + 0.5f, tw = fabsf(fsin(now * (0.5f + rnd(i, 23)) + i * 2));
		tw = tw * tw * tw, tw *= tw * in;
		unsigned c = scale(i % 3 ? ICE : IRIS, (0.25f + tw * 0.75f) * in);
		float s = 2 + tw * 9;
		gfx_line(x - s, y, x + s, y, c, 0x80, 0x80), gfx_line(x, y - s, x, y + s, c, 0x80, 0x80);
	}
	lights_off();
}
static void orbit_bg(float turn) { (void)turn; tick(); orbit_scene(420, -1); }
static void orbit_sel(float x, float y, float w, float h, float f, int front)
{
	if (!front) { halo(x, y, w, h, f * 0.4f, ICE, 80 * w / 256); return; }
	frame(x - 3, y - 3, w + 6, h + 6, 2, ICE, f);
	float a = now * 0.9f, cx = x + w / 2 + fcos(a) * w, cy = y + h / 2 + fsin(a) * h; // a glint running round the frame
	cx = cx < x - 3 ? x - 3 : cx > x + w + 3 ? x + w + 3 : cx, cy = cy < y - 3 ? y - 3 : cy > y + h + 3 ? y + h + 3 : cy;
	lights_on(), light(cx, cy, 22, f, IRIS), lights_off();
}
static float orbit_splash(float u)
{
	float w = ease((u - 1.8f) / 1);
	tick();
	orbit_scene(310, u);
	wordmark(640, 300, 72, 0xEEF3FF, IRIS, w);
	return w;
}
static void orbit_launch(float u) // a light going round the mark
{
	tick();
	orbit_scene(420, -1);
	gfx_alpha((int)(0x80 * 0.45f * ease(u / 0.5f))), gfx_rect(0, 0, W, H, 0x04060E), gfx_alpha(0x80);
	lights_on();
	for (int t = 0; t < 9; t++) light(640 + fcos(u * 7 - t * 0.09f) * 52, 278 + fsin(u * 7 - t * 0.09f) * 52, (13 - t) * 1.4f, t ? 0.3f - t * 0.03f : 1, 0x5A96FF);
	lights_off();
}

// ---- Techstep: hard cuts at 174 BPM: flat colour, a huge number, hazard stripes ----
#define TY 0xFFD60A
#define TK 0x0A0A0A
static void hazard(int y0) // black stripes marching over what is there, 12 px tall
{
	float off = 24 * frac(now * 30 / 24), q[8];
	static const unsigned k[4] = {TK, TK, TK, TK};
	gfx_alpha(0x80);
	for (int x = -24; x < W + 24; x += 24) {
		q[0] = x + off, q[1] = y0, q[2] = x + off + 12, q[3] = y0, q[4] = x + off, q[5] = y0 + 12, q[6] = x + off - 12, q[7] = y0 + 12;
		gfx_quad(q, k);
	}
}
static void seg7(int d, int x, int y, int w, int h, int t) // a digit out of seven bars
{
	static const unsigned char on[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
	int m = y + (h - t) / 2, s = on[d % 10];
	if (s & 1) gfx_rect(x, y, w, t, TK);
	if (s & 2) gfx_rect(x + w - t, y, t, h / 2, TK);
	if (s & 4) gfx_rect(x + w - t, m, t, h - (m - y), TK);
	if (s & 8) gfx_rect(x, y + h - t, w, t, TK);
	if (s & 16) gfx_rect(x, m, t, h - (m - y), TK);
	if (s & 32) gfx_rect(x, y, t, h / 2, TK);
	if (s & 64) gfx_rect(x, m, w, t, TK);
}
static void tech_bg(float turn)
{
	int n = (int)(turn + 0.5f) + 1;
	tick();
	gfx_alpha(0x80), gfx_dither(0);
	gfx_rect(0, 138, W, 502, TY);
	seg7(n / 10, 44, 340, 170, 290, 44), seg7(n, 250, 340, 170, 290, 44); // the position, huge, half hidden under the covers
	gfx_rect(540, 604, 676, 2, TK);
	for (int x = 540; x <= 1216; x += 52) gfx_rect(x, 596, 2, 8, TK);
	if (frac(now * 174 / 60) < 0.5f) gfx_rect(1206, 584, 10, 10, TK); // blinks on the beat
	gfx_rect(0, 0, W, 138, TK), gfx_rect(0, 640, W, 80, TK);
	hazard(138);
}
static void tech_sel(float x, float y, float w, float h, float f, int front)
{
	int b = (frac(now * 174 / 60) < 0.14f ? 5 : 0) + (int)(12 * w / 256);
	gfx_alpha((int)(0x80 * f));
	if (!front) gfx_rect((int)x + b, (int)y + b, (int)w, (int)h, TK); // a hard shadow that jumps on the beat
	gfx_alpha(0x80);
	if (front) frame(x - 2, y - 2, w + 4, h + 4, 2, TK, f);
}
static float st4(float n) { return ceilf(clampf(n) * 4) / 4; } // four hard steps
static float tech_splash(float u) // a band of colour snaps open and the name is pushed in from the side, in steps
{
	int hg = (int)(280 * st4(u / 0.3f)), top = 290 - hg / 2;
	float sl = (1 - st4((u - 0.3f) / 0.3f)) * 1300, cap = 130, ww = splash_vtext_w(&vfont_archivo_heavy, cap, 0.02f, "TEa.G3"), x0 = 640 - (150 + 40 + ww) / 2;
	tick();
	gfx_alpha(0x80), gfx_rect(0, 0, W, H, TK);
	if (hg <= 0) return 0;
	gfx_rect(0, top, W, hg, TY);
	if (hg > 40) hazard(top), hazard(top + hg - 12);
	gfx_scissor(0, top, W - 1, top + hg - 1);
	splash_mark(x0 + 75 - sl, 290, 75, TK, 0xFF5A36);
	splash_vtext(&vfont_archivo_heavy, x0 + 190 + sl, 290 + cap / 2, cap, 0.02f, "TEa.G3", TK);
	gfx_scissor(0, 0, W - 1, H - 1);
	return u > 0.7f;
}
static void tech_launch(float u) // black, two hazard bands, and the load counted in squares
{
	int n = (int)(clampf(u / 0.7f) * 16);
	tick();
	gfx_alpha(0x80), gfx_rect(0, 0, W, H, TK);
	gfx_rect(0, 150, W, 12, TY), gfx_rect(0, 558, W, 12, TY);
	hazard(150), hazard(558);
	for (int i = 0; i < 16; i++) {
		int x = 336 + i * 38;
		if (i < n) gfx_rect(x, 480, 30, 30, TY);
		else frame(x, 480, 30, 30, 2, TY, 1);
	}
}

// ---- Wireframe: a wire terrain in cyan and magenta coming at you, beating at 174 BPM ----
#define CYAN 0x39E6FF
#define MAGENTA 0xFF3CD2
static void terrain(float amp)
{
	float sc = now * 1.7f, fr = frac(sc), row[21][2], last[21][2];
	int base = (int)sc, have = 0;
	gfx_alpha(0x80), gfx_dither(0), gfx_rect(0, 0, W, H, 0x00030A);
	gfx_dither(1), gfx_fade(0, 240, W, 90, MAGENTA, 0, 0x1C), gfx_fade(0, 330, W, 90, MAGENTA, 0x1C, 0);
	lights_on();
	for (int i = 15; i >= 0; i--) { // rows from the horizon to your feet: hills at the sides, a valley in the middle
		float pz = 1 / (0.75f + (i - fr) * 0.3f), k = clampf(pz * 1.1f);
		unsigned c = scale(blend(MAGENTA, CYAN, k), 0.18f + 0.6f * k);
		for (int u = -10; u <= 10; u++) {
			float hg = rnd(u, i + base) - 0.25f, side = (abs(u) - 1) / 2.f, x = 640 + u * 300 * pz;
			hg = (hg < 0 ? 0 : hg) * clampf(side) * amp;
			row[u + 10][0] = x < -2000 ? -2000 : x > 2000 ? 2000 : x, row[u + 10][1] = 318 + 300 * pz - hg * 110 * pz;
		}
		gfx_alpha((int)(0x80 * (0.1f + 0.3f * k))), gfx_rect(0, (int)(318 - 180 * pz), W, 1, MAGENTA); // a flat ceiling, mirrored
		for (int n = 0; n < 20; n++) gfx_line(row[n][0], row[n][1], row[n + 1][0], row[n + 1][1], c, 0x80, 0x80);
		for (int n = 0; n < 21 && have; n++) gfx_line(row[n][0], row[n][1], last[n][0], last[n][1], c, 0x80, 0x80);
		memcpy(last, row, sizeof(row)), have = 1;
	}
	lights_off();
}
static void cube(float cx, float cy, float R, float a, unsigned c) // a wire cube turning on two axes; lights on
{
	float v[8][2], ca = cosf(a), sa = sinf(a), cb = cosf(a * 0.7f), sb = sinf(a * 0.7f);
	for (int i = 0; i < 8; i++) {
		float x = i & 1 ? 1 : -1, y = i & 2 ? 1 : -1, z = i & 4 ? 1 : -1, t = x * ca - z * sa;
		z = x * sa + z * ca, x = t, t = y * cb - z * sb, z = y * sb + z * cb, y = t;
		float q = 3.2f / (3.2f + z);
		v[i][0] = cx + x * R * q, v[i][1] = cy + y * R * q;
	}
	for (int i = 0; i < 8; i++)
		for (int b = 1; b < 8; b <<= 1)
			if (!(i & b)) gfx_line(v[i][0], v[i][1], v[i | b][0], v[i | b][1], c, 0x80, 0x80);
}
static void wire_bg(float turn) { (void)turn; tick(); terrain(1 + 1.3f * kick(now)); }
static void wire_sel(float x, float y, float w, float h, float f, int front)
{
	float j = kick(now) * 5;
	if (!front) return;
	frame(x - 3 + j, y - 3, w + 6, h + 6, 2, 0xFF4FD8, f), frame(x - 3 - j, y - 3, w + 6, h + 6, 2, CYAN, f);
}
static float wire_splash(float u)
{
	float k = ease((u - 0.5f) / 0.8f), j = kick(now) * 4;
	tick();
	terrain(1 + 1.3f * kick(now));
	lights_on(), cube(640, 300, 170 * ease(u / 1.2f), now * 0.6f, scale(CYAN, 0.5f)), lights_off();
	wordmark(644 + j, 300, 74, MAGENTA, MAGENTA, k * 0.9f), wordmark(640, 300, 74, 0xE8FBFF, CYAN, k);
	return k;
}
static void wire_launch(float u)
{
	tick();
	terrain(1 + 1.3f * kick(now));
	gfx_alpha((int)(0x80 * 0.4f * ease(u / 0.4f))), gfx_rect(0, 0, W, H, 0x00030A), gfx_alpha(0x80);
	lights_on(), cube(640, 330, mix(420, 250, ease(u / 0.8f)), now * 0.9f, scale(0xFF4FD8, 0.45f)), lights_off();
}

// ---- Neuro: dark drum and bass: a bass wave across the screen and a meter jumping with kick and snare ----
#define EMBER 0xFF6224
static void neuro_ground(void) { gfx_alpha(0x80), gfx_dither(1), gfx_grad(0, 0, W, H, 0x0B0A0A, 0x141110, 1), gfx_dither(0); }
static void bass(float y0, float amp, float e) // the wave, with its afterglow
{
	static float px[161], py[161];
	for (int n = 0; n < 161; n++) {
		float x = n * 8;
		px[n] = x, py[n] = y0 + amp * (saw(x * 0.011f - now * 2.1f) * 0.5f + saw(x * 0.0113f - now * 2.16f) * 0.5f) * (0.6f + 0.4f * fsin(x * 0.002f + now)) + fsin(x * 0.09f + now * 30) * e * 3;
	}
	lights_on();
	gfx_ribbon(px, py, 161, 12, EMBER, 0x0D), gfx_ribbon(px, py, 161, 6, EMBER, 0x1C), gfx_ribbon(px, py, 161, 2.5f, EMBER, 0x80);
	lights_off();
}
static void meter(int y, int lit) { for (int i = 0; i < 96; i++) gfx_rect(64 + i * 12, y, 10, 4, i < lit ? (i > 80 ? 0xFF2A2A : EMBER) : 0x2A2623); }
static void neuro_bg(float turn)
{
	float e = kick(now);
	(void)turn;
	tick();
	neuro_ground();
	for (int y = 160; y < 640; y += 48)
		for (int x = 16; x < W; x += 48) gfx_rect(x, y, 1, 1, 0x24211F);
	bass(600, 8 + 24 * e, e);
	meter(136, (int)((0.18f + 0.8f * e) * 96)); // the rule under the header is a level meter
}
static void neuro_sel(float x, float y, float w, float h, float f, int front)
{
	float e = kick(now), p = e * 4;
	if (!front) halo(x, y, w, h, f * (0.3f + 0.3f * e), EMBER, 80 * w / 256);
	else frame(x - 2 - p, y - 2 - p, w + 4 + 2 * p, h + 4 + 2 * p, 2, EMBER, f);
}
static float neuro_splash(float u) // a flat line, then the drop (on the sound's strike): the wave opens and the name lands
{
	int drop = u > 2.6f;
	float e = drop ? kick(now) : 0, f = u - 2.6f;
	tick();
	neuro_ground();
	bass(300, drop ? 26 + 90 * e : 1 + u * 4, e);
	meter(60, drop ? (int)((0.2f + 0.8f * e) * 96) : 2), meter(656, drop ? (int)((0.2f + 0.8f * e) * 96) : 2);
	if (drop) {
		gfx_rect(330, 226, 620, 150, 0x0B0A0A);
		wordmark(640, 300, 76, 0xF2EFEA, EMBER, 1);
		if (f < 0.25f) gfx_alpha((int)(0x40 * (1 - f / 0.25f))), gfx_rect(0, 0, W, H, 0xFFF0E6), gfx_alpha(0x80);
	}
	return ease(u / 0.6f);
}
static void neuro_launch(float u)
{
	float e = kick(now);
	tick();
	neuro_ground();
	bass(540, 18 + 70 * e, e);
	meter(470, (int)(96 * clampf(u / 0.7f)));
}

// ---- Fósforo: a green phosphor monitor; only the selected game's cover lights up in colour ----
#define GREEN 0x6BFF96
static void fos_col(int x, int y, float *c, void *u)
{
	float dx = x - 640, dy = y - 360, g = 1 - clampf(sqrtf(dx * dx + dy * dy) / 760);
	(void)u;
	c[0] = 1 + 8.4f * g, c[1] = 7 + 37.8f * g, c[2] = 3 + 16.8f * g;
}
static void fos_init(void) { picture(TH_FOSFORO, 1, fos_col); }
static void fos_over(void) // scanlines, the refresh bar rolling down, a flicker
{
	int y = (int)((H + 240) * frac(now * 90 / (H + 240))) - 120;
	gfx_alpha(0x1A);
	for (int sy = 2; sy < H; sy += 3) gfx_rect(0, sy, W, 1, 0);
	gfx_fade(0, y - 60, W, 60, 0x78FFA0, 0, 9), gfx_fade(0, y, W, 60, 0x78FFA0, 9, 0);
	gfx_alpha(4 + (int)(3 * fsin(now * 47))), gfx_rect(0, 0, W, H, 0), gfx_alpha(0x80);
}
static void fos_bg(float turn)
{
	static char addr[10][3][20];
	static int at = -1;
	float sc = now * 3;
	(void)turn;
	tick();
	backdrop(TH_FOSFORO, 0x031208);
	if ((int)sc != at) { // memory addresses scrolling behind the covers; written again only when a row comes in
		at = (int)sc;
		for (int i = 0; i < 10; i++)
			for (int c = 0; c < 3; c++)
				snprintf(addr[i][c], sizeof(addr[i][c]), "%08X %06X", 0x80000 + (i + at) * 16 + c * 0x4000, (unsigned)(rnd(i + at, c) * 0xFFFFFF));
	}
	gfx_alpha(0x16);
	for (int i = 0; i < 10; i++) // few of them: text is the dearest thing to draw here
		for (int c = 0; c < 3; c++) gfx_text(&gfx_font_mono, 64 + c * 420, 166 + i * 52 - (int)(frac(sc) * 52), addr[i][c], 0x50FF82);
	gfx_alpha(0x80), gfx_rect(0, 139, W, 26, 0x020A05);
}
static void fos_sel(float x, float y, float w, float h, float f, int front)
{
	int m = (int)(12 * w / 256), len = (int)(22 * w / 256), x0 = (int)x - m, y0 = (int)y - m, x1 = (int)(x + w) + m, y1 = (int)(y + h) + m;
	if (!front) return;
	gfx_alpha((int)(0x80 * f)); // corner brackets
	gfx_rect(x0, y0, len, 2, GREEN), gfx_rect(x0, y0, 2, len, GREEN), gfx_rect(x1 - len, y0, len, 2, GREEN), gfx_rect(x1 - 2, y0, 2, len, GREEN);
	gfx_rect(x0, y1 - 2, len, 2, GREEN), gfx_rect(x0, y1 - len, 2, len, GREEN), gfx_rect(x1 - len, y1 - 2, len, 2, GREEN), gfx_rect(x1 - 2, y1 - len, 2, len, GREEN);
	frame(x - 4, y - 4, w + 8, h + 8, 1, GREEN, f);
}
static float fos_splash(float u) // a boot log types itself out, then the name
{
	static const char *const log[5] = {"TEa.G3 BIOS", "EE R5900  294.912 MHz ............ OK", "RAM  32 MB ....................... OK",
	                                   "GS  1280 x 720 ................... OK", "USB MASS ......................... OK"};
	float w = ease((u - 2.6f) / 0.6f);
	int rows = 0;
	char line[48];
	tick();
	backdrop(TH_FOSFORO, 0x031208);
	for (int i = 0; i < 5; i++) {
		float a = (u - 0.3f - i * 0.45f) / 0.35f;
		if (a <= 0) break;
		snprintf(line, sizeof(line), "%.*s", (int)ceilf(clampf(a) * strlen(log[i])), log[i]);
		gfx_text(&gfx_font_mono, 96, 82 + i * 22, line, GREEN), rows = i + 1;
	}
	if ((int)(now * 2.5f) & 1) gfx_rect(96, 82 + rows * 22, 10, 16, GREEN);
	wordmark(640, 300, 84, GREEN, GREEN, w);
	fos_over();
	return w;
}
static void fos_launch(float u)
{
	(void)u;
	tick();
	backdrop(TH_FOSFORO, 0x031208);
	if ((int)(now * 2.5f) & 1) gfx_rect(635, 470, 10, 16, GREEN);
	fos_over();
}

// ---- Papel: the light one: warm paper, ink, long shadows ----
#define PAPER 0xF1EEE7
#define INK 0x1B1A17
#define RED 0xB4442C
static void papel_ground(void)
{
	gfx_alpha(0x80), gfx_dither(0), gfx_rect(0, 0, W, H, PAPER);
	gfx_dither(1), gfx_fade(0, 380, W, 340, 0xE2DDD2, 0, 0x73), gfx_dither(0), gfx_alpha(0x80);
}
static void papel_bg(float turn) { (void)turn; tick(); papel_ground(); }
static void papel_sel(float x, float y, float w, float h, float f, int front)
{
	float sc = w / 256;
	if (!front) halo(x, y + 26 * sc, w, h, f * 0.45f, 0x3C2D14, 34 * sc); // its shadow on the paper
	else frame(x - 7, y - 7, w + 14, h + 14, 3, RED, f);
}
static float papel_splash(float u) // two rules open from the centre and the name is stamped between them
{
	float k = ease((u - 0.5f) / 0.4f);
	int len = (int)(430 * ease(u / 1.2f));
	tick();
	papel_ground();
	gfx_rect(640 - len, 222, 2 * len, 1, INK), gfx_rect(640 - len, 380, 2 * len, 1, INK);
	wordmark(640, 302, 76 * (1.14f - 0.14f * k), INK, RED, k);
	return ease((u - 1) / 0.5f);
}
static void papel_launch(float u) // an ink rule draws itself
{
	tick();
	papel_ground();
	gfx_rect(440, 470, 400, 2, 0xCFC9BD), gfx_rect(440, 470, (int)(400 * ease(u / 0.7f)), 2, INK);
}

// key, name, palette {bg, panel, row, edge, dim, text, text2, label, accent, steel, inv}, list pane, card, flags,
// splash frames, status y, init, bg, sel, splash, launch, over
theme_def themes[TH_MAX] = {
	{"torres", "Torres", {0x01030F, 0x050C2E, 0x16296B, 0x4962A5, 0x3A4A80, 0xE8EEFF, 0xC3D0F5, 0xA6B8EA, 0x8FD8FF, 0x13235A, 0x031236},
	 0x4D, 0x80, TF_PIANO, 330, 430, torres_init, torres_bg, torres_orbs, torres_splash, torres_launch, NULL},
	{"nuld", "TEa.G3", {NULD_BG, 0x0C0E12, 0x161A21, 0x2A303A, 0x4A515C, NULD_FG, 0xA9B0BA, NULD_MUTED, NULD_ACID, NULD_STEEL, NULD_BG},
	 0, 0x80, TF_MARK, 240, 0, NULL, nuld_bg, nuld_sel, NULL, nuld_launch, splash_scan},
	{"cristal", "Cristal", {0x0B0A14, 0x15122A, 0x2E2A48, 0x6A6880, 0x4A4860, 0xFFFFFF, 0xEFECF6, 0xDAD5E8, 0xFFB454, 0x26233A, 0x15101F},
	 0x50, 0x58, TF_ACCENT | TF_MARK | TF_PIANO, 240, 446, NULL, cristal_bg, cristal_sel, cristal_splash, cristal_launch, NULL},
	{"aurora", "Aurora", {0x03050F, 0x080C1A, 0x18233F, 0x3A4058, 0x39405C, 0xEEF2FF, 0xCBD2EC, 0xAEB7D6, 0x7CF5D6, 0x121A30, 0x070A14},
	 0x50, 0x60, TF_MARK | TF_PIANO, 270, 430, aurora_init, aurora_bg, aurora_sel, aurora_splash, aurora_launch, NULL},
	{"liquid", "Liquid", {0x050E24, 0x1A1440, 0x3A3470, 0x9A94C0, 0x6A6590, 0xFFFFFF, 0xF0EDFA, 0xD6D6F0, 0x7FF0E0, 0x3A3560, 0x0B1F45},
	 0x60, 0x60, TF_CHROME | TF_MARK | TF_PIANO, 300, 644, liquid_init, liquid_bg, liquid_sel, liquid_splash, liquid_launch, NULL},
	{"onda", "Onda", {0x101622, 0x0A0E18, 0x2A3244, 0x8A90A0, 0x5A6070, 0xFFFFFF, 0xEEF1F8, 0xD5DBEA, 0xFFFFFF, 0x303848, 0x101622},
	 0x40, 0x48, TF_MARK | TF_PIANO, 240, 410, NULL, onda_bg, onda_sel, onda_splash, onda_launch, NULL},
	{"orbit", "Orbit", {0x04060E, 0x0D1736, 0x1B3050, 0x3C6A8C, 0x3A4670, 0xEEF3FF, 0xC4CFEA, 0xA9B6D3, ICE, 0x16224A, 0x04060E},
	 0x5C, 0x5C, TF_CHROME | TF_MARK | TF_PIANO, 240, 430, orbit_init, orbit_bg, orbit_sel, orbit_splash, orbit_launch, NULL},
	{"techstep", "Techstep", {TK, TK, 0x242420, 0xF4F4F0, 0x6A6A62, 0xF4F4F0, 0xF4F4F0, 0xB8B8B0, TY, 0x2A2A26, TK},
	 0x80, 0x80, TF_FLAT, 240, 470, NULL, tech_bg, tech_sel, tech_splash, tech_launch, NULL},
	{"wireframe", "Wireframe", {0x00030A, 0x00080E, 0x0A2A34, 0x1C7384, 0x2A5560, 0xE8FBFF, 0xB6E6F0, 0x8CC7D4, CYAN, 0x0A2630, 0x00090C},
	 0x64, 0x64, TF_GHOST | TF_PIANO, 240, 430, NULL, wire_bg, wire_sel, wire_splash, wire_launch, NULL},
	{"neuro", "Neuro", {0x0B0A0A, 0x131211, 0x1D1A18, 0x34302C, 0x4A4540, 0xF2EFEA, 0xC2BCB3, 0x9A948B, EMBER, 0x24201D, 0x0A0A0A},
	 0, 0x80, TF_MARK | TF_PIANO, 240, 440, NULL, neuro_bg, neuro_sel, neuro_splash, neuro_launch, NULL},
	{"fosforo", "F\xC3\xB3sforo", {0x010703, 0x021408, 0x0E3A1C, 0x2A8046, 0x1E5A30, GREEN, 0x4FE07C, 0x3DB965, GREEN, 0x0A2A14, 0x021006},
	 0x60, 0x70, TF_MONO | TF_MARK | TF_PIANO, 270, 430, fos_init, fos_bg, fos_sel, fos_splash, fos_launch, fos_over},
	{"papel", "Papel", {PAPER, 0xFBFAF6, 0xFFFFFF, 0xCFC9BD, 0xB0AA9E, INK, 0x45423B, 0x666158, RED, 0xE4DFD4, 0xF4F1EA},
	 0, 0x80, TF_FLAT | TF_MARK | TF_PIANO, 240, 430, NULL, papel_bg, papel_sel, papel_splash, papel_launch, NULL},
};
int themes_n = TH_N;

// ---- the community's themes (docs/themes.md): a folder each, theme.ini and maybe a picture ----
static char u_key[TH_MAX - TH_N][32], u_name[TH_MAX - TH_N][32], u_img[TH_MAX - TH_N][96]; // u_img: "" without a picture
static unsigned short *img; // the picture of theme img_of, dithered, in tiles
static int img_of = -1;
typedef struct { unsigned char *rgb; int w, h; } rgb_src;
static void rgb_col(int x, int y, float *c, void *u)
{ // ponytail: the nearest pixel; exact at 1280x720, the size the guide asks for. Lanczos (cover.c) if other sizes matter
	const rgb_src *s = u;
	const unsigned char *p = s->rgb + ((y * s->h / H) * s->w + x * s->w / W) * 3;
	c[0] = p[0], c[1] = p[1], c[2] = p[2];
}
static void image_load(int id) // about 6 MB while it runs, 1.8 MB kept; anything missing: the theme's flat colour
{
	if (img_of == id) return;
	img_of = -1;
	FILE *f = fopen(u_img[id - TH_N], "rb");
	if (!f) return;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	rewind(f);
	unsigned char *raw = n > 0 && n <= 4 << 20 ? malloc(n) : NULL;
	rgb_src s = {NULL, 0, 0};
	if (raw && fread(raw, 1, n, f) == (size_t)n) s.rgb = cover_rgb(raw, n, &s.w, &s.h);
	fclose(f);
	free(raw);
	unsigned short *lin = s.rgb ? malloc(W * H * 2) : NULL;
	if (lin && (img || (img = memalign(64, W * H * 2)))) {
		gfx_fs_dither(lin, W, H, rgb_col, &s);
		gfx_tiles_from(img, lin, W, H);
		SyncDCache(img, img + W * H);
		img_of = id;
	}
	free(lin);
	free(s.rgb);
}
static void image_bg(float turn)
{
	(void)turn;
	tick();
	gfx_alpha(0x80), gfx_dither(0);
	if (img_of == cur) gfx_image_tiled(img, W, H, 0, 0);
	else gfx_rect(0, 0, W, H, themes[cur].pal.bg);
}
static unsigned hex(const char *s, unsigned def) // "RRGGBB"; anything else: def
{
	char *e;
	unsigned v = strtoul(s, &e, 16);
	return e - s == 6 && !*e ? v : def;
}
void themes_load(const char *dir)
{
	static ini t;
	static const char *const col[11] = {"fondo", "panel", "fila", "borde", "tenue", "texto", "texto2", "etiqueta", "acento", "acero", "inverso"};
	char path[96];
	DIR *d = opendir(dir);
	struct dirent *e;
	while (d && (e = readdir(d)) && themes_n < TH_MAX) {
		int u = themes_n - TH_N, base = TH_NULD, taken = 0;
		if (e->d_name[0] == '.' || strlen(e->d_name) > 31) continue;
		snprintf(path, sizeof(path), "%s/%s/theme.ini", dir, e->d_name);
		if (!ini_load(&t, path)) continue;
		for (int i = 0; i < themes_n; i++) {
			if (i < TH_N && !strcasecmp(ini_get(&t, "", "base", "nuld"), themes[i].key)) base = i;
			taken |= !strcasecmp(e->d_name, themes[i].key);
		}
		if (taken) continue; // a built-in theme's name
		theme_def *th = &themes[themes_n];
		*th = themes[base];
		snprintf(u_key[u], sizeof(u_key[u]), "%s", e->d_name);
		snprintf(u_name[u], sizeof(u_name[u]), "%s", ini_get(&t, "", "nombre", e->d_name));
		th->key = u_key[u], th->name = u_name[u];
		unsigned *p = &th->pal.bg;
		for (int i = 0; i < 11; i++) p[i] = hex(ini_get(&t, "colores", col[i], ""), p[i]);
		const char *ac = ini_get(&t, "colores", "acento", ""); // "portada": the selected cover's colour
		if (!strcasecmp(ac, "portada")) th->flags |= TF_ACCENT;
		else if (*ac) th->flags &= ~TF_ACCENT;
		u_img[u][0] = 0;
		for (int k = 0; k < 2 && !u_img[u][0]; k++) {
			snprintf(path, sizeof(path), "%s/%s/fondo.%s", dir, e->d_name, k ? "png" : "jpg");
			FILE *f = fopen(path, "rb");
			if (f) fclose(f), strcpy(u_img[u], path);
		}
		if (u_img[u][0]) { // the picture is the ground: the rows of the list get a pane over it
			th->bg = image_bg;
			if (th->pane < 0x50) th->pane = 0x50;
		}
		themes_n++;
	}
	if (d) closedir(d);
}
void themes_enter(int id)
{
	cur = id;
	if (themes[id].init) themes[id].init(); // the base's: its splash and its launch screen may draw what it makes
	if (id >= TH_N && u_img[id - TH_N][0]) image_load(id);
}
