#ifndef TEA_RUNTIME_INSTALL_H
#define TEA_RUNTIME_INSTALL_H
int runtime_install(const char *home);
/* Verify one embedded runtime asset against its recorded size and CRC before hand-off. */
int runtime_asset_valid(const char *home, const char *relative_path);
#endif
