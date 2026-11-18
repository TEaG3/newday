#pragma once
// Themes (v1.3): a palette plus what only that theme draws, behind the same layout. themes.c has the table and the
// scenes of ten of them; torres.c is Torres; NULD (v1.2's look) is splash.c's floor, corners and scanlines.
typedef struct { unsigned bg, panel, row, edge, dim, text, text2, label, accent, steel, inv; } palette;
enum { TH_TORRES, TH_NULD, TH_CRISTAL, TH_AURORA, TH_LIQUID, TH_ONDA, TH_ORBIT, TH_TECHSTEP, TH_WIREFRAME, TH_NEURO,
       TH_FOSFORO, TH_PAPEL, TH_N };
enum {
	TF_CHROME = 1,  // the title in chrome (gfx_text_chrome)
	TF_GHOST = 2,   // the title over a magenta copy of itself
	TF_FLAT = 4,    // the ground is bright or flat colour: text drawn straight on it gets a pane of its own
	TF_ACCENT = 8,  // the accent is the selected cover's colour (themes_accent)
	TF_MONO = 16,   // covers out of focus in the theme's one colour
	TF_MARK = 32,   // the launch screen shows NULD's mark (the others put a light of their own there)
	TF_PIANO = 64,  // its splash goes with the piano sound (S_TORRES); without it, v1.2's
};
typedef struct {
	const char *key, *name;  // config.ini's [ui] tema; the name in the setup panel
	palette pal;
	unsigned char pane, card; // alpha (0..0x80) of the pane behind the list's rows (0: none) and of the saves card
	unsigned char flags;      // TF_*
	short splash_frames;      // the splash's shortest run, frames after the black hold
	short status_y;           // where its splash has the status line (the progress bar goes 38 px under it)
	void (*init)(void);       // once before its first frame (may take a second: a backdrop is dithered)
	void (*bg)(float turn);   // the home's background, a frame; turn: the selection's eased position
	void (*sel)(float x, float y, float w, float h, float f, int front); // round the selected cover: behind it, then in front
	float (*splash)(float u); // its splash at u seconds; returns the alpha for the status line (NULL: splash.c's)
	void (*launch)(float u);  // behind the title when something starts
	void (*over)(void);       // over everything, last (scanlines)
} theme_def;
extern const theme_def themes[TH_N];
// the selected cover as it is drawn (CT16, w x h), every frame it is there: for the themes that take from it
void themes_cover(const unsigned short *pix, int w, int h);
unsigned themes_accent(void); // TF_ACCENT: the cover's colour, eased
