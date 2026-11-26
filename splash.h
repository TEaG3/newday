#pragma once
#include "logo_data.h"
// TEa.G3 loading screen: a thread draws it while the main thread waits (IOP modules, network, the PC, covers)
void splash_start(void);              // draws until splash_stop; nothing else may use gfx meanwhile
void splash_msg(const char *s);       // the status line(s) under the logo ('\n' splits lines)
void splash_stop(void);               // waits for the last frame
// the screen itself, between gfx_begin and gfx_end, for a caller that draws its own frames (s: seconds)
void splash_draw(float s);
extern const char *splash_ver; // "1.0": the corner label and the hardware label
void splash_mark(float cx, float cy, float r, unsigned ring, unsigned slash); // the ∅ mark (ring + slash), for other screens too
// the Classic theme look for the other screens: palette, backdrop, wordmark, HUD corners
#define CLASSIC_BG 0x06070A
#define CLASSIC_STEEL 0x1A1D23
#define CLASSIC_MUTED 0x8A919C
#define CLASSIC_FG 0xE6E8EC
#define CLASSIC_ACID 0xC8FF2E
void splash_floor(float s, unsigned rgb, int a); // clears to CLASSIC_BG, then the perspective floor (lines at alpha a)
void splash_scan(void);                          // CRT scanlines over everything: draw it last
// vector text in the logo fonts (logo_data.c: vfont_michroma "TEa.G3", vfont_archivo_heavy "TEa.G3/", vfont_plex_medium
// "$ TEa.G3"): cap px tall, track in em after each letter, from x on the baseline
float splash_vtext_w(const vfont *f, float cap, float track, const char *s);
void splash_vtext(const vfont *f, float x, float base, float cap, float track, const char *s, unsigned rgb);
void splash_hud(int x, int y, int w, int h, int len, unsigned c); // corner brackets around a box, at the current alpha
