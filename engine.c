#include "engine.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

engine_id engine_select(engine_format format, engine_source source, const char *preference)
{
    int force_opl = preference && !strcasecmp(preference, "opl");

    /* USBExtreme UL is an OPL-only container; never pretend Neutrino can read it. */
    if (format == ENGINE_FORMAT_UL)
        return source == ENGINE_SOURCE_USB ? ENGINE_OPL_UL : ENGINE_UNSUPPORTED;

    if (force_opl)
        return source == ENGINE_SOURCE_USB ? ENGINE_OPL_USB : ENGINE_UNSUPPORTED;

    /* auto and neutrino intentionally keep ISO/ZSO on the existing Neutrino path. */
    return ENGINE_NEUTRINO;
}

#ifdef SELFTEST
int main(void)
{
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "auto") == ENGINE_NEUTRINO);
    assert(engine_select(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_USB, "auto") == ENGINE_NEUTRINO);
    assert(engine_select(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "auto") == ENGINE_OPL_UL);
    assert(engine_select(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "neutrino") == ENGINE_OPL_UL);
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "opl") == ENGINE_OPL_USB);
    assert(engine_select(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_USB, "opl") == ENGINE_OPL_USB);
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_OTHER, "opl") == ENGINE_UNSUPPORTED);
    assert(engine_select(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_OTHER, "opl") == ENGINE_UNSUPPORTED);
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_OTHER, "neutrino") == ENGINE_NEUTRINO);
    assert(engine_select(ENGINE_FORMAT_UL, ENGINE_SOURCE_OTHER, "auto") == ENGINE_UNSUPPORTED);
    puts("engine selection selftest ok");
    return 0;
}
#endif
