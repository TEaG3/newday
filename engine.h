#ifndef TEAG3_ENGINE_H
#define TEAG3_ENGINE_H

typedef enum {
    ENGINE_NEUTRINO = 0,
    ENGINE_OPL_UL,
    ENGINE_OPL_BDM,
    ENGINE_OPL_ETH,
    ENGINE_OPL_HDD,
    ENGINE_UNSUPPORTED
} engine_id;

typedef enum {
    ENGINE_FORMAT_ISO = 0,
    ENGINE_FORMAT_ZSO,
    ENGINE_FORMAT_UL
} engine_format;

typedef enum {
    ENGINE_SOURCE_OTHER = 0,
    ENGINE_SOURCE_USB,
    ENGINE_SOURCE_BDM,
    ENGINE_SOURCE_SMB,
    ENGINE_SOURCE_HDL
} engine_source;

typedef enum {
    ENGINE_REASON_AUTO_NEUTRINO_BASELINE = 0,
    ENGINE_REASON_AUTO_OPL_ADAPTER,
    ENGINE_REASON_UL_REQUIRES_OPL,
    ENGINE_REASON_USER_NEUTRINO,
    ENGINE_REASON_USER_OPL,
    ENGINE_REASON_OPL_RUNTIME_MISSING,
    ENGINE_REASON_OPL_NETWORK_CONFIG_INVALID,
    ENGINE_REASON_OPL_SOURCE_UNAVAILABLE,
    ENGINE_REASON_UNSUPPORTED_FORMAT_SOURCE
} engine_reason;

typedef struct {
    engine_id engine;
    engine_reason reason;
} engine_decision;

/* All engine choices are made here; capability flags describe this exact launch. */
engine_decision engine_assess(engine_format format, engine_source source,
                              const char *preference, int opl_runtime,
                              int opl_network_ready);
const char *engine_reason_name(engine_reason reason);

/* Kept as a simple compatibility helper for existing host fixtures. */
engine_id engine_select(engine_format format, engine_source source, const char *preference);

#endif
