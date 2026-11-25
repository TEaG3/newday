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
    engine_decision d = {ENGINE_UNSUPPORTED, ENGINE_REASON_OPL_SOURCE_UNAVAILABLE};
    int force_opl = preference && !strcasecmp(preference, "opl");
    int automatic = !preference || !*preference || !strcasecmp(preference, "auto");

    /* Neutrino is retired as a game engine. Legacy settings fail closed. */
    if (preference && !strcasecmp(preference, "neutrino")) {
        d.reason = ENGINE_REASON_ENGINE_REMOVED;
        return d;
    }
    if (!automatic && !force_opl) {
        d.reason = ENGINE_REASON_ENGINE_REMOVED;
        return d;
    }

    /* USBExtreme is a split USB container and is handled only by OPL UL. */
    if (format == ENGINE_FORMAT_UL) {
        if (source != ENGINE_SOURCE_USB)
            d.reason = ENGINE_REASON_UNSUPPORTED_FORMAT_SOURCE;
        else if (!opl_runtime)
            d.reason = ENGINE_REASON_OPL_RUNTIME_MISSING;
        else {
            d.engine = ENGINE_OPL_UL;
            d.reason = ENGINE_REASON_UL_REQUIRES_OPL;
        }
        return d;
    }

    /* ZSO is supported by OPL BDM/ETH, not by the APA/HDL mini-launch path. */
    if (format == ENGINE_FORMAT_ZSO && source == ENGINE_SOURCE_HDL) {
        d.reason = ENGINE_REASON_UNSUPPORTED_FORMAT_SOURCE;
        return d;
    }

    if (source == ENGINE_SOURCE_USB || source == ENGINE_SOURCE_BDM) {
        d.engine = ENGINE_OPL_BDM;
        d.reason = force_opl ? ENGINE_REASON_USER_OPL : ENGINE_REASON_AUTO_OPL_ADAPTER;
    } else if (source == ENGINE_SOURCE_SMB) {
        if (!opl_network_ready)
            d.reason = ENGINE_REASON_OPL_NETWORK_CONFIG_INVALID;
        else {
            d.engine = ENGINE_OPL_ETH;
            d.reason = force_opl ? ENGINE_REASON_USER_OPL : ENGINE_REASON_AUTO_OPL_ADAPTER;
        }
    } else if (source == ENGINE_SOURCE_HDL && format == ENGINE_FORMAT_ISO) {
        d.engine = ENGINE_OPL_HDD;
        d.reason = force_opl ? ENGINE_REASON_USER_OPL : ENGINE_REASON_AUTO_OPL_ADAPTER;
    } else if (format == ENGINE_FORMAT_ZSO) {
        d.reason = ENGINE_REASON_ZSO_REQUIRES_OPL;
    }

    /* Every supported route must have OPL; never fall back to a retired engine. */
    if (is_opl(d.engine) && !opl_runtime) {
        d.engine = ENGINE_UNSUPPORTED;
        d.reason = ENGINE_REASON_OPL_RUNTIME_MISSING;
    }
    return d;
}

const char *engine_reason_name(engine_reason reason)
{
    switch (reason) {
    case ENGINE_REASON_AUTO_NEUTRINO_BASELINE: return "legacy-neutrino-route-disabled";
    case ENGINE_REASON_AUTO_OPL_ADAPTER: return "auto-opl-adapter";
    case ENGINE_REASON_UL_REQUIRES_OPL: return "ul-requires-opl";
    case ENGINE_REASON_USER_NEUTRINO: return "legacy-neutrino-preference-disabled";
    case ENGINE_REASON_USER_OPL: return "preference-opl";
    case ENGINE_REASON_OPL_RUNTIME_MISSING: return "opl-runtime-missing";
    case ENGINE_REASON_OPL_NETWORK_CONFIG_INVALID: return "opl-network-config-invalid";
    case ENGINE_REASON_OPL_SOURCE_UNAVAILABLE: return "opl-source-unavailable";
    case ENGINE_REASON_UNSUPPORTED_FORMAT_SOURCE: return "unsupported-format-source";
    case ENGINE_REASON_ZSO_REQUIRES_OPL: return "zso-requires-opl";
    case ENGINE_REASON_ENGINE_REMOVED: return "engine-removed";
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
    assert(d.engine == ENGINE_OPL_BDM && d.reason == ENGINE_REASON_AUTO_OPL_ADAPTER);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_USB, "auto", 1, 1);
    assert(d.engine == ENGINE_OPL_BDM);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_BDM, "auto", 1, 1);
    assert(d.engine == ENGINE_OPL_BDM);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_BDM, "auto", 1, 1);
    assert(d.engine == ENGINE_OPL_BDM);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_SMB, "auto", 1, 1);
    assert(d.engine == ENGINE_OPL_ETH);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_SMB, "auto", 1, 1);
    assert(d.engine == ENGINE_OPL_ETH);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_HDL, "auto", 1, 1);
    assert(d.engine == ENGINE_OPL_HDD);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_HDL, "auto", 1, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_UNSUPPORTED_FORMAT_SOURCE);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_SMB, "auto", 1, 0);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_OPL_NETWORK_CONFIG_INVALID);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_SMB, "auto", 1, 0);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_OPL_NETWORK_CONFIG_INVALID);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "auto", 0, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_OPL_RUNTIME_MISSING);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_USB, "auto", 0, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_OPL_RUNTIME_MISSING);
    d = engine_assess(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "auto", 1, 1);
    assert(d.engine == ENGINE_OPL_UL && d.reason == ENGINE_REASON_UL_REQUIRES_OPL);
    d = engine_assess(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "neutrino", 1, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_ENGINE_REMOVED);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "neutrino", 1, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_ENGINE_REMOVED);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_HDL, "opl", 1, 1);
    assert(d.engine == ENGINE_OPL_HDD);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_SMB, "opl", 1, 1);
    assert(d.engine == ENGINE_OPL_ETH);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_OTHER, "auto", 1, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_OPL_SOURCE_UNAVAILABLE);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "opl", 0, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_OPL_RUNTIME_MISSING);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_OTHER, "neutrino", 1, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_ENGINE_REMOVED);
    puts("engine decision selftest ok");
    return 0;
}
#endif
