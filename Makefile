# make            -> bench.elf
# make APP=modetest -> modetest.elf
# make APP=launcher -> TEa.G3.ELF (Arabic TEa.G3 launcher UI)
# make APP=boot     -> boot.elf (memory-card stub that runs mass0:/TEa.G3.ELF; embedded in the launcher)
# Jellyfin (jfplay.elf) lives in its own repo, danielnuld/orbit-jellyfin, installed as mass0:/APPS/Jellyfin
APP ?= bench
SFX = splash torres move edge confirm panel # sfx/*.wav from tools/sfx.py -> SPU2 ADPCM (ps2sdk adpenc)
EE_BIN = $(if $(filter launcher,$(APP)),TEa.G3.ELF,$(APP).elf)
IRX_FILES = iomanX fileXio sio2man mcman mcserv freepad libsd audsrv bdm bdmfs_fatfs usbd_mini usbmass_bd_mini $(if $(filter launcher,$(APP)),ps2dev9 netman smap ata_bd ps2hdd_bdm mx4sio_bd_mini iLinkman IEEE1394_bd_mini mmceman)
# launcher extras: network for cover downloads (net.c); game sources, loaded per config.ini (sources.c, phase 14)
RUNTIME_OBJS = $(patsubst %.c,%.o,$(wildcard runtime_asset_*.c))
EE_OBJS = $(APP).o iop.o $(if $(filter launcher,$(APP)),runtime_install.o runtime_payload.o catalog_stage.o catalog_guard_ps2.o $(RUNTIME_OBJS) gfx.o splash.o torres.o themes.o logo_data.o iso.o zso_ee.o ul.o engine.o ini.o lang_ar.o net.o fmcb.o update.o achievements.o sources.o vmc.o cover.o icon.o combo.o font_data.o ui_data.o exec.o loader_elf.o boot_elf.o $(SFX:%=sfx_%.o)) $(if $(filter boot,$(APP)),exec.o loader_elf.o) $(IRX_FILES:=_irx.o)
EE_LIBS = $(if $(filter launcher,$(APP)),-L$(PS2SDK)/ports/lib -lwolfssl -ljpeg -lpng -lz -lnetman -lps2ip -Xlinker --wrap=open -Xlinker --wrap=read) $(if $(filter boot,$(APP)),-ldebug) -laudsrv -lcdvd -lmc -lfont -lpacket -ldma -lgraph -ldraw -lpad -lfileXio -lpatches -lc
EE_INCS += -I$(PS2SDK)/ports/include # wolfssl, jpeglib (ps2sdk ports)
EE_CFLAGS += -MMD -MP $(if $(BENCH),-DTORRES_BENCH) $(if $(THEME),-DTHEME_FORCE=$(THEME)) # THEME=n: that theme, whatever config.ini says (tests) # .d files: editing a header rebuilds its users (v1.0 shipped a stale gfx.o)

all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)

# Arabic UI glyphs are generated from the pinned Noto Sans Arabic UI font.
.PHONY: font
font:
	python3 tools/fetch_arabic_font.py
	python3 tools/font.py
	python3 tools/logo.py

tools/fonts/NotoSansArabicUI.ttf:
	python3 tools/fetch_arabic_font.py

font_data.c: tools/font.py tools/fonts/NotoSansArabicUI.ttf
	python3 tools/font.py

sfx_%.c: sfx/%.wav
	$(PS2SDK)/bin/adpenc $< sfx_$*.adp && $(PS2SDK)/bin/bin2c sfx_$*.adp $@ sfx_$*

loader/loader.elf: loader/loader.c loader/linkfile
	$(MAKE) -C loader

loader_elf.c: loader/loader.elf
	$(PS2SDK)/bin/bin2c $< $@ loader_elf

boot_elf.c: boot.c iop.c exec.c loader/loader.elf
	$(MAKE) APP=boot all && $(PS2SDK)/bin/bin2c boot.elf $@ boot_elf

ps2hdd_bdm_irx.c: # the file name has a dash, the C symbol cannot
	$(PS2SDK)/bin/bin2c $(PS2SDK)/iop/irx/ps2hdd-bdm.irx $@ ps2hdd_bdm_irx

%_irx.c:
	$(PS2SDK)/bin/bin2c $(PS2SDK)/iop/irx/$*.irx $@ $*_irx

test: font
	python3 tools/font.py --selftest
	python3 tools/lang_check.py # checks Arabic/English, defaults, language tables and the font pipeline
	cc -std=c99 -Wall -DSELFTEST bench.c -o /tmp/bench_selftest && /tmp/bench_selftest
	cc -std=c99 -Wall -DSELFTEST gfx.c font_data.c -o /tmp/gfx_selftest && /tmp/gfx_selftest
	cc -std=gnu99 -Wall -DSELFTEST iso.c -o /tmp/iso_selftest && /tmp/iso_selftest
	cc -std=c99 -Wall -DSELFTEST ul.c -o /tmp/ul_selftest && /tmp/ul_selftest
	cc -std=c99 -Wall -DSELFTEST engine.c -o /tmp/engine_selftest && /tmp/engine_selftest
	cc -std=gnu99 -Wall -DSELFTEST ini.c -o /tmp/ini_selftest && /tmp/ini_selftest
	cc -std=gnu99 -Wall -DSELFTEST net.c -o /tmp/net_selftest && /tmp/net_selftest
	cc -std=gnu99 -Wall -Wno-deprecated-declarations -DSELFTEST achievements.c iso.c -DSELFTEST_LIB -lcrypto -o /tmp/ra_selftest && RA_WL=$(RA_WL) RA_GAME=$(RA_GAME) /tmp/ra_selftest $(RA_CHECK) # RA_WL / RA_GAME: a real watch list / client /game answer
	cc -std=gnu99 -Wall -DSELFTEST vmc.c -o /tmp/vmc_selftest && /tmp/vmc_selftest $(VMC_CHECK)
	cc -std=gnu99 -Wall -DSELFTEST update.c -o /tmp/update_selftest && /tmp/update_selftest
	cc -std=gnu99 -Wall -DSELFTEST fmcb.c -o /tmp/fmcb_selftest && /tmp/fmcb_selftest
	cc -std=gnu99 -Wall -DSELFTEST combo.c -o /tmp/combo_selftest && /tmp/combo_selftest
	cc -std=gnu99 -Wall -DSELFTEST icon.c -lm -o /tmp/icon_selftest && /tmp/icon_selftest $(ICON_CHECK)
	cc -std=gnu99 -Wall -DSELFTEST -I. -Itests/fixtures runtime_install.c tests/runtime_install_host.c -o /tmp/runtime_install_selftest && /tmp/runtime_install_selftest
	cc -std=gnu99 -Wall -DSELFTEST -Dmain=gfx_main -c gfx.c -o /tmp/gfx_host.o && cc -std=gnu99 -Wall -DSELFTEST cover.c /tmp/gfx_host.o font_data.c -ljpeg -lpng -lm -o /tmp/cover_selftest && /tmp/cover_selftest $(COVER_CHECK)

nuld_hash: tools/nuld_hash.c achievements.c iso.c vmc.c # the game server's helper (phase 17), static: nuld has no compiler
	cc -std=gnu99 -O2 -Wall -Wno-deprecated-declarations -DSELFTEST -DSELFTEST_LIB -DNULD_HASH -static $^ -lcrypto -lpthread -o $@

clean:
	rm -f *.elf *.o *.d *_irx.c sfx_*.c *.adp loader_elf.c boot_elf.c runtime_asset_*.c runtime_payload.c runtime_payload.h runtime-manifest.txt nuld_hash
	$(MAKE) -C loader clean

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
-include $(EE_OBJS:.o=.d)
