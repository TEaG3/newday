// Torres theme (v1.3; the prototype "Temas NULD en vivo"): prisms of light rising out of a blue night, lights
// drifting up between them and seven orbs. Everything live is light ADDED to the picture (gfx_add), so nothing
// needs sorting; the backdrop (the radial glow, the night at the top, the vignette) never changes and is dithered
// once on the EE (docs/image-quality.md: the GS dither leaves steps in big dark gradients). Lights fade into the
// same night per corner (lit), instead of being covered by gradients afterwards.
#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <kernel.h>
#include "gfx.h"
#include "splash.h"
#include "torres.h"

#define TAU 6.2831853f
#define NIGHT 0x01030F
#define INK 0xE8EEFF
#define ICE 0x8FD8FF
#define ORB 0x5A96FF
#define FOCAL 900.f
#define ORBK 1.6f // the orbs' size over the prototype's: gfx_glow falls off sooner than its sprite did

typedef struct { float x, z, h, d, ph; } pillar;
typedef struct { float x, z, y, v, ph; } mote;
static pillar pil[289];
static mote motes[46];
static int npil;
static unsigned short *bg; // the backdrop's left half, 640x720 CT16 in 256x128 tiles
int torres_bench;
static float now, rise;    // the scene's clock (s); how far the towers are out of the ground

static float clampf(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
static float ease(float k) { k = clampf(k); return k * k * (3 - 2 * k); }
static float mix(float a, float b, float k) { return a + (b - a) * k; }
static float rnd(int i, int j) // 0..1, fixed per (i, j)
{
	unsigned h = (unsigned)i * 374761393u + (unsigned)j * 668265263u;
	h = (h ^ h >> 13) * 1274126177u;
	return ((h ^ h >> 16) & 0xFFFF) / 65536.f;
}
// how much of a light is left at a screen point: none in the night at the top, less towards the corners
static float shade(float x, float y)
{
	float dx = x - 640, dy = y - 360;
	return clampf((y - 110) / 210) * (1 - 0.55f * clampf((sqrtf(dx * dx + dy * dy) - 300) / 500));
}

static void bg_col(int x, int y, float *c, void *u) // radial #1A3FA8 -> #0A1A5C -> night, centred low
{
	(void)u;
	static const float in[3] = {26, 63, 168}, mid[3] = {10, 26, 92}, out[3] = {1, 3, 15};
	float dx = x - 640, dy = y - 500, r = sqrtf(dx * dx + dy * dy) / 760, s = shade(x, y);
	for (int k = 0; k < 3; k++) {
		float g = r < 0.4f ? mix(in[k], mid[k], r / 0.4f) : r < 1 ? mix(mid[k], out[k], (r - 0.4f) / 0.6f) : out[k];
		c[k] = out[k] + (g - out[k]) * s;
	}
}

void torres_init(void)
{
	if (npil) return;
	for (int i = -8; i <= 8; i++)
		for (int j = -8; j <= 8; j++)
			if (rnd(i, j) > 0.4f) {
				float t = rnd(j + 9, i - 3);
				pil[npil++] = (pillar){i, j, 0.25f + t * t * 2.8f, sqrtf(i * i + j * j), rnd(i + 40, j) * 9};
			}
	for (int i = 0; i < 46; i++)
		motes[i] = (mote){(rnd(i, 101) - 0.5f) * 15, (rnd(i, 102) - 0.5f) * 15, rnd(i, 103) * 5, 0.12f + rnd(i, 104) * 0.25f, rnd(i, 105) * 9};
	// the left half only: the picture is the same mirrored, and sending all of it took 3 ms of every frame on the console
	unsigned short *lin = malloc(GFX_W / 2 * GFX_H * 2);
	bg = memalign(64, GFX_W / 2 * GFX_H * 2);
	if (lin && bg) {
		gfx_fs_dither(lin, GFX_W / 2, GFX_H, bg_col, NULL);
		gfx_tiles_from(bg, lin, GFX_W / 2, GFX_H);
		SyncDCache(bg, bg + GFX_W / 2 * GFX_H);
	} else free(bg), bg = NULL; // no memory: a flat night
	free(lin);
}

// ---- the camera: over the field at height ch, cd back from its centre, looking down by the pitch ----
static float ca, sa, cp, sp, cd, ch;
static int proj(float x, float y, float z, float *o) // o: screen x, y, depth; 0 behind the camera
{
	float xr = x * ca - z * sa, zr = x * sa + z * ca + cd, yr = y - ch, zv = -yr * sp + zr * cp;
	if (zv < 0.7f) return 0;
	float sx = 640 + FOCAL * xr / zv, sy = 300 - FOCAL * (yr * cp + zr * sp) / zv;
	o[0] = sx < -2000 ? -2000 : sx > 2000 ? 2000 : sx; // the GS's coordinates end at +-2048
	o[1] = sy < -2000 ? -2000 : sy > 2000 ? 2000 : sy;
	o[2] = zv;
	return 1;
}
static unsigned glow(unsigned rgb, float k) // the colour of a light of strength k: added to the picture, it is its alpha
{
	return (unsigned)((rgb >> 16 & 255) * k) << 16 | (unsigned)((rgb >> 8 & 255) * k) << 8 | (unsigned)((rgb & 255) * k);
}
static void light(float x, float y, float s, float a) // a soft blue light with a white heart, radius s
{
	int al = (int)(0x80 * a), r = (int)s, c = (int)(s * 0.45f);
	if (al < 1 || r < 1) return;
	gfx_alpha(al);
	gfx_glow((int)x - r, (int)y - r, 2 * r, 2 * r, ORB, ORB);
	if (c > 0) gfx_glow((int)x - c, (int)y - c, 2 * c, 2 * c, 0xFFFFFF, 0xFFFFFF);
}
static void lights_on(void) { gfx_dither(!(torres_bench & 4)), gfx_add(1); }
static void lights_off(void) { gfx_add(0), gfx_dither(0), gfx_alpha(0x80); }

static void scene(float h, float d, float pitch, float yaw, float turn)
{
	now += 1 / 60.f;
	gfx_alpha(0x80), gfx_dither(0);
	if (bg && !(torres_bench & 1)) gfx_image_mirrored(bg, GFX_W / 2, GFX_H);
	else gfx_rect(0, 0, GFX_W, GFX_H, NIGHT);
	float a = now * 0.045f + turn * 0.1f + yaw;
	ca = cosf(a), sa = sinf(a), cp = cosf(pitch), sp = sinf(pitch), cd = d, ch = h;
	lights_on();
	// The prisms were 5 of the home's 11.6 ms on the console (2026-11-18, make BENCH=1; the goal is 8.3). So: the field
	// is turned once per prism and its four corners are fixed offsets from there, one shade per prism (its top is
	// small on screen), only the sides that face the camera, no rim on the far ones, none drawn that would not show.
	static const signed char cs[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}, nrm[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
	float ox[4], oz[4], nx[4], nz[4];
	for (int k = 0; k < 4; k++) {
		ox[k] = (cs[k][0] * ca - cs[k][1] * sa) * 0.3f, oz[k] = (cs[k][0] * sa + cs[k][1] * ca) * 0.3f;
		nx[k] = nrm[k][0] * ca - nrm[k][1] * sa, nz[k] = nrm[k][0] * sa + nrm[k][1] * ca;
	}
	for (int i = 0; i < npil && !(torres_bench & 2); i += 1 + !!(torres_bench & 8)) {
		const pillar *p = &pil[i];
		float xr = p->x * ca - p->z * sa, zr = p->x * sa + p->z * ca + cd, zc = ch * sp + zr * cp; // zc: depth at the ground
		if (zc < 1.2f) continue;
		float fog = clampf(1 - (zc - 2.5f) / 15), sxc = 640 + FOCAL * xr / zc;
		if (fog < 0.06f || sxc < -160 || sxc > GFX_W + 160) continue;
		float br = (now * 0.5f + p->ph) * (1 / TAU); // it breathes: 5 % taller and back, a parabola for the sine
		br -= (int)br;
		float hh = p->h * ease(rise * 0.8f - p->d * 0.09f) * (1 + 0.05f * (br < 0.5f ? 16 * br * (0.5f - br) : -16 * (br - 0.5f) * (1 - br)));
		float zt = zc - hh * sp;
		if (hh < 0.03f || zt < 0.9f) continue;
		float sh = fog * shade(640 + FOCAL * xr / zt, 300 - FOCAL * ((hh - ch) * cp + zr * sp) / zt);
		if (sh < 0.03f) continue; // in the night at the top
		float b[4][2], u[4][2], q[8];
		int ok = 1;
		for (int k = 0; k < 4 && ok; k++) {
			float X = xr + ox[k], Z = zr + oz[k], zb = ch * sp + Z * cp, zu = zb - hh * sp;
			if (zb < 0.7f || zu < 0.7f) { ok = 0; break; }
			float ib = FOCAL / zb, iu = FOCAL / zu, v;
			v = 640 + X * ib, b[k][0] = v < -2000 ? -2000 : v > 2000 ? 2000 : v; // the GS's coordinates end at +-2048
			v = 300 - (Z * sp - ch * cp) * ib, b[k][1] = v < -2000 ? -2000 : v > 2000 ? 2000 : v;
			v = 640 + X * iu, u[k][0] = v < -2000 ? -2000 : v > 2000 ? 2000 : v;
			v = 300 - (Z * sp + (hh - ch) * cp) * iu, u[k][1] = v < -2000 ? -2000 : v > 2000 ? 2000 : v;
		}
		if (!ok) continue;
		unsigned side = glow(0x6EA5FF, 0.3f * sh), top = glow(0x96CDFF, 0.36f * sh), rim = glow(0xD7EEFF, 0.6f * sh);
		for (int k = 0; k < 4; k++) { // the sides towards the camera (at xr = zr = 0): light at the top edge, none at the ground
			int n = (k + 1) & 3;
			if (nx[k] * (xr + nx[k] * 0.3f) + nz[k] * (zr + nz[k] * 0.3f) > 0) continue;
			unsigned col[4] = {0, 0, side, side};
			q[0] = b[k][0], q[1] = b[k][1], q[2] = b[n][0], q[3] = b[n][1], q[4] = u[n][0], q[5] = u[n][1], q[6] = u[k][0], q[7] = u[k][1];
			gfx_quad(q, col);
		}
		unsigned tc[4] = {top, top, top, top};
		for (int k = 0; k < 4; k++) q[2 * k] = u[k][0], q[2 * k + 1] = u[k][1];
		gfx_quad(q, tc);
		for (int k = 0; k < 4 && fog > 0.3f; k++) { // and its rim
			int n = (k + 1) & 3;
			gfx_line(u[k][0], u[k][1], u[n][0], u[n][1], rim, 0x80, 0x80);
		}
	}
	for (int i = 0; i < 46 && !(torres_bench & 16); i++) { // lights drifting up
		const mote *m = &motes[i];
		float c[3];
		if (!proj(m->x, fmodf(m->y + now * m->v, 5), m->z, c)) continue;
		float s = 30 / c[2];
		light(c[0], c[1], s < 2 ? 2 : s > 10 ? 10 : s,
		      (0.3f + 0.4f * fabsf(sinf(now * 1.3f + m->ph))) * clampf(1 - (c[2] - 2) / 16) * shade(c[0], c[1]));
	}
	lights_off();
}

void torres_bg(float turn)
{
	if (rise < 30) rise += 1.1f / 60;
	scene(4.2f, 9, 0.42f, 0, turn);
}

void torres_orbs(float x, float y, float w, float h, float f, int front)
{
	float sc = w / 256, cx = x + w / 2, cy = y + h - 36 * sc, rx = w / 2 + 52 * sc, ry = 40 * sc;
	if (torres_bench & 16) return;
	lights_on();
	for (int i = 0; i < 7; i++)
		for (int tr = 0; tr < 5; tr++) { // each with a short tail
			float th = now * 1.25f + i * TAU / 7 - tr * 0.07f, sn = sinf(th);
			if ((sn > 0) != front) continue;
			light(cx + cosf(th) * rx, cy + sn * ry, ORBK * (15 + 6 * sn) * (1 - tr * 0.13f) * (sc < 0.6f ? 0.6f : sc), f * (tr ? 0.28f - tr * 0.05f : 1));
		}
	lights_off();
}

float torres_splash(float u)
{
	float k = ease(u / 5), m = clampf(u / 2.6f), rad = 430 * powf(1 - ease(m), 1.2f), fl = u - 2.6f, w = ease((u - 2.7f) / 1.1f);
	rise = u * 1.1f;
	scene(mix(11, 4.2f, k), mix(3, 9, k), mix(1.15f, 0.42f, k), (1 - k) * 1.5f, 0);
	lights_on();
	if (m < 1)
		for (int i = 0; i < 7; i++)
			for (int tr = 0; tr < 6; tr++) { // the orbs spiral in to where the name will be
				float a = i * TAU / 7 + u * 2.4f - tr * 0.06f;
				light(640 + cosf(a) * rad, 300 + sinf(a) * rad * 0.5f, ORBK * (16 + 10 * m) * (1 - tr * 0.12f), tr ? 0.3f - tr * 0.04f : 1);
			}
	if (fl > 0 && fl < 1.2f) { // they meet: a flash that opens and dies (the sound's piano strike, tools/sfx.py torres)
		float R = 80 + fl * 520, a = (1 - fl / 1.2f) * (1 - fl / 1.2f);
		gfx_alpha((int)(0x80 * a));
		gfx_glow((int)(640 - R), (int)(300 - R * 0.6f), (int)(2 * R), (int)(1.2f * R), ORB, ORB);
		gfx_glow((int)(640 - R / 2), (int)(300 - R * 0.3f), (int)R, (int)(0.6f * R), 0xFFFFFF, 0xFFFFFF);
	}
	if (w > 0) gfx_alpha((int)(0x38 * w)), gfx_glow(260, 170, 760, 260, ORB, ORB); // a glow behind the name
	lights_off();
	if (w > 0) {
		const float cap = 72, r = cap * 0.6875f, gap = cap * 0.42f;
		float x = 640 - (2 * r + gap + splash_vtext_w(&vfont_michroma, cap, 0.06f, "NULD")) / 2;
		gfx_alpha((int)(0x80 * w));
		splash_mark(x + r, 300, r, INK, ICE);
		splash_vtext(&vfont_michroma, x + 2 * r + gap, 300 + cap / 2, cap, 0.06f, "NULD", INK);
		gfx_alpha(0x80);
	}
	return w;
}

void torres_launch(float u)
{
	if (rise > 2.6f) rise = 2.6f; // the far towers go first
	rise = rise > 2.6f / 50 ? rise - 2.6f / 50 : 0;
	scene(4.2f, 9, 0.42f, u * 0.25f, 0);
	gfx_alpha((int)(0x80 * 0.6f * ease(u / 0.8f))), gfx_rect(0, 0, GFX_W, GFX_H, NIGHT), gfx_alpha(0x80);
	float sh = 1 - ease(u / 0.85f);
	lights_on();
	for (int i = 0; i < 7; i++)
		for (int tr = 0; tr < 5; tr++) {
			float th = now * 1.25f + u * u * 4 + i * TAU / 7 - tr * 0.07f * (1 + u), sn = sinf(th);
			light(640 + cosf(th) * 180 * sh, 278 + sn * 40 * sh, ORBK * (15 + 6 * sn) * (1 - tr * 0.13f), ease(u / 0.3f) * (tr ? 0.28f - tr * 0.05f : 1));
		}
	if (sh < 0.3f) light(640, 278, 34 + 60 * (0.3f - sh), 1 - sh / 0.3f); // one light
	lights_off();
}
