# Neutrino IGR UI — retained design reference

This folder preserves the **visual layer only** from the former Neutrino IGR integration. It is not a game runtime and is not linked into TEa.G3 or OPL.

The active PS2 game engine is OPL. The old Neutrino loader, IGR execution path, kernel patches, and Neutrino ELF are not included here.

## Preserved interface

- A compact dark overlay with the TEa.G3 acid-green accent.
- Arabic and English labels for Restart, Power Off, and Cancel.
- Right-aligned Arabic glyphs generated as pre-shaped bitmaps from Noto Sans Arabic UI.
- A short Arabic achievement notice.
- UP/DOWN navigation, CROSS confirmation, and CIRCLE cancellation.

The exact layout, colors, labels, and interaction requirements are recorded in `menu-design.json`. The generator in this folder recreates the Arabic bitmap header for a future OPL IGR port.

## Safety boundary

`menu_arabic.h` is generated during CI for validation only. It is not compiled into the current OPL runtime. No Neutrino loader, game-sector reader, IOP module, or kernel hook is called.

The overlay must remain disabled until the menu state machine is ported to OPL and pause/resume, GS-idle, controller input, VBlank timing, and return-to-launcher behavior pass hardware acceptance on a real PS2.
