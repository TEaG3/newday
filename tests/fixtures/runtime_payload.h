#ifndef TEAG3_RUNTIME_PAYLOAD_TEST_FIXTURE_H
#define TEAG3_RUNTIME_PAYLOAD_TEST_FIXTURE_H
typedef struct {
    const char *path;
    const unsigned char *data;
    const unsigned int *size;
    unsigned int crc32;
} tea_runtime_asset;
extern const tea_runtime_asset tea_runtime_assets[];
extern const unsigned int tea_runtime_assets_count;
extern const char tea_runtime_bundle_id[];
#endif
