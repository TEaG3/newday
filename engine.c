#include "engine.h"
#include <assert.h>
#include <stdio.h>
engine_decision engine_assess(engine_format format, engine_source source,
                              const char *preference, int opl_runtime, int opl_network_ready)
{
    (void)preference;
    (void)opl_network_ready;
    engine_decision d = {ENGINE_UNSUPPORTED, ENGINE_REASON_UNSUPPORTED_FORMAT_SOURCE};
    if (!opl_runtime) { d.reason = ENGINE_REASON_OPL_RUNTIME_MISSING; return d; }
    if (format == ENGINE_FORMAT_UL) {
        if (source == ENGINE_SOURCE_USB) { d.engine = ENGINE_OPL_UL; d.reason = ENGINE_REASON_UL_REQUIRES_OPL; }
        return d;
    }
    if (format == ENGINE_FORMAT_ZSO && source == ENGINE_SOURCE_HDL) {
        d.reason = ENGINE_REASON_ZSO_REQUIRES_OPL; return d;
    }
    if (source == ENGINE_SOURCE_USB || source == ENGINE_SOURCE_BDM) {
        d.engine = ENGINE_OPL_BDM; d.reason = ENGINE_REASON_OPL_ADAPTER; return d;
    }
    if (source == ENGINE_SOURCE_HDL && format == ENGINE_FORMAT_ISO) {
        d.engine = ENGINE_OPL_HDD; d.reason = ENGINE_REASON_OPL_ADAPTER; return d;
    }
    return d;
}
const char *engine_reason_name(engine_reason reason)
{
    switch (reason) {
    case ENGINE_REASON_OPL_ADAPTER: return "opl-adapter";
    case ENGINE_REASON_UL_REQUIRES_OPL: return "ul-requires-opl";
    case ENGINE_REASON_OPL_RUNTIME_MISSING: return "opl-runtime-missing";
    case ENGINE_REASON_ZSO_REQUIRES_OPL: return "zso-requires-opl";
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
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "legacy", 1, 1); assert(d.engine == ENGINE_OPL_BDM);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_USB, "auto", 1, 1); assert(d.engine == ENGINE_OPL_BDM);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_BDM, "auto", 1, 1); assert(d.engine == ENGINE_OPL_BDM);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_HDL, "auto", 1, 1); assert(d.engine == ENGINE_OPL_HDD);
    d = engine_assess(ENGINE_FORMAT_ZSO, ENGINE_SOURCE_HDL, "auto", 1, 1); assert(d.engine == ENGINE_UNSUPPORTED);
    d = engine_assess(ENGINE_FORMAT_UL, ENGINE_SOURCE_USB, "auto", 1, 1); assert(d.engine == ENGINE_OPL_UL);
    d = engine_assess(ENGINE_FORMAT_UL, ENGINE_SOURCE_BDM, "auto", 1, 1); assert(d.engine == ENGINE_UNSUPPORTED);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_OTHER, "auto", 1, 1); assert(d.engine == ENGINE_UNSUPPORTED);
    d = engine_assess(ENGINE_FORMAT_ISO, ENGINE_SOURCE_USB, "opl", 0, 1);
    assert(d.engine == ENGINE_UNSUPPORTED && d.reason == ENGINE_REASON_OPL_RUNTIME_MISSING);
    puts("OPL-only engine decision selftest ok");
    return 0;
}
#endif
