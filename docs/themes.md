# Making a theme for TEa.G3

TEa.G3 has twelve built-in themes. Any additional theme is a **folder** you copy to the launcher's data directory on
the USB device or MMCE card. This guide needs no compiler or special tools—only a text editor and, optionally, an
image editor.

A working example is next to this page: [`themes/ocaso/`](themes/ocaso/) (a `theme.ini` and a `fondo.jpg`).

## What a theme is

```
mass0:/tea_g3/themes/
└── ocaso/              the folder's name is what config.ini stores ([ui] tema = ocaso)
    ├── theme.ini       name, base and colours
    └── fondo.jpg       optional: the background picture (or fondo.png)
```

- Copy the folder to `tea_g3/themes/` on the device TEa.G3 starts from (USB or MMCE). The home-device prefix may
  therefore be `mass0:`, `mmce0:` or `mmce1:`.
- Start TEa.G3, press **START**, go to **Theme** and move left / right: your theme comes after the built-in ones, and
  is shown at once behind the panel. **Save** keeps it.
- Up to **8** custom theme folders are read. This is a code limit, not a design choice: `themes.c` reserves
  `TH_MAX - TH_N` slots, currently eight after the twelve built-in themes. Folders named after built-in themes
  (`torres`, `nuld`, etc.) are skipped.
- Folder names are limited to **31 characters** by the 32-byte key buffers and the explicit length check in
  `themes.c`; plain letters and digits are the safest choice.

## theme.ini

```ini
nombre = Ocaso        ; the name shown in the setup panel (the folder's name if missing)
base = nuld           ; the built-in theme this one starts from (nuld if missing)

[colores]             ; RRGGBB, no "#". Any colour you leave out keeps the base's
fondo = 0E0714
panel = 1A0F22
fila = 3A2140
borde = 6A4A66
tenue = 5A4458
texto = FFF4E8
texto2 = E8D2C4
etiqueta = C9A9A0
acento = FF8A3C
acero = 2A1830
inverso = 0E0714
```

Lines starting with `;` or `#` are comments. The keys are in Spanish, as in `config.ini`.

### base

A theme is a built-in theme with another name, palette and, maybe, background. `base` picks which one:
`torres`, `nuld`, `cristal`, `aurora`, `liquid`, `onda`, `orbit`, `techstep`, `wireframe`, `neuro`, `fosforo`, `papel`.

From its base your theme takes everything that is not a colour of the palette:

- the **splash** and the **launch screen** (and the splash's sound),
- what is drawn **round the selected cover** (NULD's corners, Torres's orbs, Papel's shadow...),
- the **animated background**, unless you give a picture,
- how the title is drawn (chrome in `liquid` and `orbit`, a magenta ghost in `wireframe`), scanlines (`nuld`,
  `fosforo`), covers out of focus in one colour (`fosforo`).

The base's own lights keep their colours (Torres's orbs stay blue-white whatever your palette says), so pick a base
that goes with your colours. `nuld` is the most neutral one: its corners follow your `acento`.

### The colours

| Key | Used for |
|---|---|
| `fondo` | the ground where nothing else is drawn; what shows if the picture cannot be read |
| `panel` | panels: setup, options, the detail card, the pane behind the list |
| `fila` | the selected row; the frame of the options that are off |
| `borde` | 1-px edges of panels, covers and chips; rules |
| `tenue` | what is off: unchecked options, trophies not earned |
| `texto` | titles and main text |
| `texto2` | secondary text (list rows not selected) |
| `etiqueta` | small labels in the mono font (serials, counters, the status line) |
| `acento` | the selection: the bar, the filter in use, button rings, the progress bar |
| `acero` | quiet fills: the options that are on but not under the cursor, the empty part of bars |
| `inverso` | text drawn **on** an `acento` or `texto` fill (the lit chips) |

`acento = portada` makes the accent follow the colour of the selected cover instead of a fixed one.

Check two contrasts before you publish: `texto` over `panel`, and `inverso` over `acento`. The PS2's picture is 15-bit
colour: two dark colours closer than about 8 steps per channel look the same on a TV.

## The background picture

- File name: **`fondo.jpg`** or **`fondo.png`** (the JPG wins if both are there).
- Size: **1280 x 720**. Other sizes (64 to 2048 px a side) are stretched to the screen without smoothing, so they
  look worse; a different aspect ratio is distorted.
- JPEG: baseline or progressive, RGB. PNG: any; transparent parts come out near black. 4 MB at most.
- The console converts it once when the theme is chosen (a few seconds of black at start-up) to 15-bit colour with
  dithering, so smooth gradients survive, but very fine noise or film grain turns into visible dots: keep it clean.
- With a picture the base's animated background is **not** drawn (the picture takes its place); the base's splash,
  launch screen and selection are still there.
- Keep it calm where the text goes: the title at the top left, the list on the left half, the legend along the bottom.
  The list always gets a translucent `panel` pane behind its rows over a picture.
- A full-screen picture costs the console about 3 ms of each frame. That is fine over the bases that draw little
  (`nuld`, `papel`, `neuro`, `techstep`); it has not been measured over the heavy ones.

## What a theme cannot change

- **Positions and sizes**: the layout is the same in every theme (three views, the same places). This is on purpose.
- **Fonts and icons**.
- **Sounds**.
- **New animations**: only the ones of the twelve built-in themes, through `base`.
- The splash's and the launch screen's own colours.

## If it does not show up

- The folder must be at `tea_g3/themes/<name>/theme.ini` on the same home device as `tea_g3/config.ini`.
- A colour that is not exactly six hex digits is ignored (the base's is used).
- If the ground is a flat colour instead of your picture, the file could not be decoded (CMYK JPEGs, a file over 4 MB)
  or the console ran out of memory: save it again as a plain RGB JPEG, 1280 x 720.
- If the theme named in `config.ini` is not there any more, TEa.G3 starts with Torres.

## Sharing it

Zip the folder. Whoever gets it unzips it into `tea_g3/themes/`. If you publish a theme, say which TEa.G3 version you made
it on, and do not put artwork in it that is not yours to share.
