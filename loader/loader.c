// Second-stage ELF loader (phase 6). The launcher copies this ELF into RAM the BIOS leaves free (0x84000-0x100000,
// linkfile) and runs it; it loads argv[0] through the IOP's LOADFILE with the modules the launcher left
// running, then hands over argv + 1. It does NOT reset the IOP: Neutrino started with -qb expects USB + fileXio already
// loaded, and ps2sdk's elf-loader resets the IOP (first console run fell back to the browser). Idea from nhddl
// (reference only, docs/sources.md); own code.
#include <kernel.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <tamtypes.h>
#include <ps2sdkapi.h>

// no libc start-up or shutdown work: smaller, and nothing touches the IOP
void _libcglue_init(void) {}
void _libcglue_deinit(void) {}
void _libcglue_args_parse(int argc, char **argv) { (void)argc, (void)argv; }
DISABLE_PATCHED_FUNCTIONS();
DISABLE_EXTRA_TIMERS_FUNCTIONS();
PS2_DISABLE_AUTOSTART_PTHREAD();

int main(int argc, char *argv[])
{
	t_ExecData elf = {0};
	if (argc < 1) return -1;
	if (argc < 2 || !argv[0] || !*argv[0]) return -1;
	SifInitRpc(0);
	SifLoadFileInit();
	int r = SifLoadElf(argv[0], &elf); // argv[0]: the file; argv[1..]: the program's own argv (phase 11)
	SifLoadFileExit();
	// Crucial: do not erase the launcher's main memory until the target ELF has loaded successfully.
	// A missing/corrupt ELF therefore fails before the destructive hand-off boundary.
	if (r != 0 || !elf.epc) { SifExitRpc(); return -2; }
	for (u32 a = 0x100000; a < (u32)GetMemorySize(); a += 16) *(volatile u128 *)a = 0;
	FlushCache(0);
	/* The second-stage contract reserves argv[0] for the ELF path and passes argv[1..] to the target.
	   An empty first target argument is replaced with the ELF path for the legacy POPStarter convention;
	   a caller-supplied non-empty value means the target's argv[0] is not necessarily a path. */
	if (!*argv[1]) argv[1] = argv[0];
	FlushCache(0);
	FlushCache(2);
	return ExecPS2((void *)elf.epc, (void *)elf.gp, argc - 1, argv + 1);
}
