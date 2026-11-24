#include "engine.h"

#include <assert.h>
#include <stdio.h>
#include <strings.h>

static int is_opl(engine_id engine)
{
    return engine == ENGINE_OPL_UL || engine == ENGINE_OPL_BDM ||
           engine == ENGINE_OPL_ETH || engine == ENGINE_OPL_HDD;
}

engine_decision engine_assess(engine_format format, engine_source source,
                              const char *preference, int opl_runtime,
                              int opl_network_ready)
{
    engine_decision d = {ENGINE_NEUTRINO, ENGINE_REASON_AUTO_NEUTRINO_BASELINE};
    int force_opl = preference && !strcasecmp(preference, "opl");
    int force_neutrino = preference && !strcasecmp(preference, "neutrino");

    /* UL is a split USBExtreme container: Neutrino cannot consume this layout. */
    if (format == ENGINE_FORMAT_UL) {
        if (source != ENGINE_SOURCE_USB) {
            d.engine = ENGINE_UNSUPPORTED;
            d.reason = ENGINE_REASON_UNSUPPORTED_FORMAT_SOURCE;
        } else if (!opl_runtime) {
            d.engine = ENGINE_UNSUPPORTED;
            d.reason = ENGINE_REASON_OPL_RUNTIME_MISSING;
        } else {
            d.engine = ENGINE_OPL_UL;
            d.reason = ENGINE_REASON_UL_REQUIRES_OPL;
        }
        return d;
    }

    if (force_neutrino) {
        d.reason = ENGINE_REASON_USER_NEUTRINO;
        return d;
    }

    if (force_opl) {
        d.reason = ENGINE_REASON_USER_OPL;
        if (source == ENGINE_SOURCE_USB || source == ENGINE_SOURCE_BDM)
            d.engine = ENGINE_OPL_BDM;
        else if (source == ENGINE_SOURCE_SMB) {
            if (opl_network_ready) d.engine = ENGINE_OPL_ETH;
            else d.reason = ENGINE_REASON_OPL_NETWORK_CONFIG_INVALID;
        } else if (source == ENGINE_SOURCE_HDL)
            d.engine = ENGINE_OPL_HDD;
        else
            d.reason = ENGINE_REASON_OPL_SOURCE_UNAVAILABLE;
    }

    /* Never hand off to an OPL adapter that is not present in the embedded runtime. */
    if (is_opl(d.engine) && !opl_runtime) {
        d.engine = ENGINE_NEUTRINO;
        d.reason = ENGINE_REASON_OPL_RUNTIME_MISSING;
    }
    return d;
}

const char *engine_reason_name(engine_reason reason)
{
    switch (reason) {
    case ENGINE_REASON_AUTO_NEUTRINO_BASELINE: return "auto-neutrino-baseline";
    case ENGINE_REASON_UL_REQUIRES_OPL: return "ul-requires-opl";
    case ENGINE_REASON_USER_NEUTRINO: return "preference-neutrino";
    case ENGINE_REASON_USER_OPL: return "preference-opl";
    case ENGINE_REASON_OPL_RUNTIME_MISSING: return "opl-runtime-missing";
    case ENGINE_REASON_OPL_NETWORK_CONFIG_INVALID: return "opl-network-config-invalid";
    case ENGINE_REASON_OPL_SOURCE_UNAVAILABLE: return "opl-source-unavailable";
    default: return "unsupported-format-source";
    }
}

engine_id engine_select(engine_format format, engine_source source, const char *preference)
{
    return engine_assess(format, source, preference, 1, 1).engine;
}

#ifdef SELFTEST
int main(void)
{
    engine_decision d;
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "auto", 1, 1);
    assert(d.engine == ENGINE_NEUTRINO && d.reason == ENGINE_REASON_AUTO_NEUTRINO_BASELINE);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_USB, "auto", 1, 1);
    assert(d.engine == ENGINE_NEUTRINO);
    d = engine_assess(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "auto", 1, 1);
    assert(d.engine == ENGINE_OPL_UL && d.reason == ENGINE_REASON_UL_REQUIRES_OPL);
    d = engine_assess(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "neutrino", 1, 1);
    assert(d.engine == ENGINE_OPL_UL);
    d = engine_assess(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "auto", 0, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_OPL_RUNTIME_MISSING);
    d = engine_assess(ENGINE_FORMAT_UL, ENGINE_SOURCE_SMB, "auto", 1, 1);
    assert(d.engine == ENGINE_UNSUPPORTED);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "opl", 1, 1);
    assert(d.engine == ENGINE_OPL_BDM);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_BDM, "opl", 1, 1);
    assert(d.engine == ENGINE_OPL_BDM);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_SMB, "opl", 1, 1);
    assert(d.engine == ENGINE_OPL_ETH);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_SMB, "opl", 1, 0);
    assert(d.engine == ENGINE_NEUTRINO && d.reason == ENGINE_REASON_OPL_NETWORK_CONFIG_INVALID);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_HDL, "opl", 1, 1);
    assert(d.engine == ENGINE_OPL_HDD);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_HDL, "auto", 1, 1);
    assert(d.engine == ENGINE_NEUTRINO);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "opl", 0, 1);
    assert(d.engine == ENGINE_NEUTRINO && d.reason == ENGINE_REASON_OPL_RUNTIME_MISSING);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_OTHER, "opl", 1, 1);
    assert(d.engine == ENGINE_NEUTRINO && d.reason == ENGINE_REASON_OPL_SOURCE_UNAVAILABLE);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_BDM, "neutrino", 1, 1);
    assert(d.engine == ENGINE_NEUTRINO && d.reason == ENGINE_REASON_USER_NEUTRINO);
    puts("engine decision selftest ok");
    return 0;
}
#endif
