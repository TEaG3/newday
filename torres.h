#pragma once
// Torres theme (v1.3): a field of prisms of light in a blue night and seven orbs, drawn live by the GS.
extern int torres_bench; // make BENCH=1: parts left out, to time them on the console (1 backdrop image, 2 prisms,
                         // 4 the GS dither, 8 every other prism, 16 motes and orbs)
void torres_init(void);          // dithers the backdrop (under a second, 0.9 MB): once, while the screen is black
void torres_bg(float turn);      // the home's background, a frame; the field turns a little with turn (the selection)
// the seven orbs on a tilted ring round a cover (its box, focus f 0..1): the back half before the cover, the front after
void torres_orbs(float x, float y, float w, float h, float f, int front);
float torres_splash(float u);    // u seconds in: the camera comes down, the orbs gather and leave the name; its alpha
void torres_launch(float u);     // a game starts: the towers sink and the orbs close into one light at (640, 278)
