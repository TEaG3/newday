#pragma once
#define COVER_W 256  // <serial>.c16 (cover-art spec)
#define COVER_H 368
#define COVER_SW 184 // <serial>_s.c16
#define COVER_SH 264
#define COVER_HW 128 // grid tiles: half of the big cover, made in RAM (phase 10)
#define COVER_HH 184
// sq: a square cover (PS1 CD case, apps): as wide as the DVD one, as tall as wide (256x256, 184x184, 128x128)
#define COVER_TALL(sq, w, h) ((sq) ? (w) : (h))
void cover_half(const unsigned short *big, unsigned short *half, int sq); // 2x2 linear-light means + Floyd-Steinberg
// JPEG or PNG bytes -> both cover sizes as CT16 pixels (no .c16 header); 0 if the file cannot be decoded
int cover_from_image(const unsigned char *jpg, int n, unsigned short *big, unsigned short *small, int sq);
// JPEG or PNG bytes -> RGB, 3 bytes a pixel (malloc'd: the caller frees it); NULL if the file cannot be decoded
unsigned char *cover_rgb(const unsigned char *jpg, int n, int *w, int *h);
