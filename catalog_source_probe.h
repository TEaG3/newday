#ifndef TEAG3_CATALOG_SOURCE_PROBE_H
#define TEAG3_CATALOG_SOURCE_PROBE_H

/* Return 1 only when the source root can be opened and closed successfully.
   Omit the trailing slash for PS2 HDD roots such as hdd0:. */
int catalog_source_root_probe(const char *root, int no_trailing_slash);

#endif
