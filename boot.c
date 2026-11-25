// TEa.G3 boot stub (phase 12), installed at mc?:/BOOT/TEa.G3.ELF.
// Legacy NULD.ELF and ORBIT.ELF entry points are refreshed for existing FreeMcBoot shortcuts.
// - The TEa.G3 Neutrino fork returns here after In Game Reset; after an IOP reset only ROM modules remain,
//   so the return path has to start from a memory card);
// - autoboot: OSDMenu / FMCB can start this file.
// It brings up the USB (iop.c, same modules as the launcher) and runs mass0:/TEa.G3.ELF through the loader, so the
// launcher itself is only ever updated on the USB. Without the USB it says so and keeps waiting.
#include <stdio.h>
#include <kernel.h>
#include <debug.h>
#include "iop.h"
#include "exec.h"

#define LAUNCHER "mass0:/TEa.G3.ELF"

int main(void)
{
	int shown = 0;
	if (!iop_load()) { init_scr(); scr_printf("\n\n  TEa.G3: no se pudieron cargar los modulos del IOP\n"); SleepThread(); }
	for (;;) {
		if (usb_wait()) {
			FILE *f = fopen(LAUNCHER, "rb");
			if (f) {
				fclose(f);
				char *argv[2] = {LAUNCHER, LAUNCHER};
				run_loader(2, argv);
			}
		}
		if (!shown) { // ponytail: libdebug text, this screen is rare
			init_scr();
			scr_printf("\n\n  TEa.G3\n\n  Conecta la USB con TEa.G3.ELF en la raiz.\n  Se reintenta solo.\n");
			shown = 1;
		}
	}
	return 0;
}
