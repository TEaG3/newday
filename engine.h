#ifndef TEAG3_ENGINE_H
#define TEAG3_ENGINE_H

typedef enum {
    ENGINE_NEUTRINO = 0,
    ENGINE_OPL_UL,
    ENGINE_OPL_USB,
    ENGINE_UNSUPPORTED
} engine_id;

typedef enum {
    ENGINE_FORMAT_ISO = 0,
    ENGINE_FORMAT_ZSO,
    ENGINE_FORMAT_UL
} engine_format;

typedef enum {
    ENGINE_SOURCE_OTHER = 0,
    ENGINE_SOURCE_USB
} engine_source;

/* preference: auto, neutrino, or opl. Source and image format stay independent. */
engine_id engine_select(engine_format format, engine_source source, const char *preference);

#endif
