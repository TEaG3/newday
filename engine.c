#include "engine.h"

#include <assert.h>
#include <stdio.h>
#include <strings.h>

engine_id engine_select(engine_format format, engine_source source, const char *preference)
{
    int force_opl = preference && !strcasecmp(preference, "opl");

    /* USBExtreme UL is an OPL-only container. Keep it on OPL even in Neutrino mode. */
    if (format == ENGINE_FORMAT_UL)
        return source == ENGINE_SOURCE_USB ? ENGINE_OPL_UL : ENGINE_UNSUPPORTED;

    /* Auto deliberately keeps the already established Neutrino route. */
    if (!force_opl)
        return ENGINE_NEUTRINO;

    /* OPL's native BDM adapter handles USB, MX4SIO, iLink and ATA-backed FAT/exFAT. */
    if (source == ENGINE_SOURCE_USB || source == ENGINE_SOURCE_BDM)
        return ENGINE_OPL_BDM;

    /* OPL has a separate native SMB/ETH backend; it is not the Neutrino SMB FHI. */
    if (source == ENGINE_SOURCE_SMB)
        return ENGINE_OPL_ETH;

    /* APA/HD Loader requires OPL's dedicated HDD mode and existing OPL data partition. */
    if (source == ENGINE_SOURCE_HDL)
        return ENGINE_OPL_HDD;

    /* Graceful fallback: the source remains on its existing Neutrino route. */
    return ENGINE_NEUTRINO;
}

#ifdef SELFTEST
int main(void)
{
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "auto") == ENGINE_NEUTRINO);
    assert(engine_select(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_USB, "auto") == ENGINE_NEUTRINO);
    assert(engine_select(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "auto") == ENGINE_OPL_UL);
    assert(engine_select(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "neutrino") == ENGINE_OPL_UL);
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "opl") == ENGINE_OPL_BDM);
    assert(engine_select(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_USB, "opl") == ENGINE_OPL_BDM);
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_BDM, "opl") == ENGINE_OPL_BDM);
    assert(engine_select(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_BDM, "opl") == ENGINE_OPL_BDM);
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_SMB, "opl") == ENGINE_OPL_ETH);
    assert(engine_select(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_SMB, "opl") == ENGINE_OPL_ETH);
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_HDL, "opl") == ENGINE_OPL_HDD);
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_OTHER, "opl") == ENGINE_NEUTRINO);
    assert(engine_select(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_OTHER, "opl") == ENGINE_NEUTRINO);
    assert(engine_select(ENGINE_FORMAT_ISO, ENGINE_SOURCE_BDM, "neutrino") == ENGINE_NEUTRINO);
    assert(engine_select(ENGINE_FORMAT_UL, ENGINE_SOURCE_OTHER, "auto") == ENGINE_UNSUPPORTED);
    puts("engine selection selftest ok");
    return 0;
}
#endif
