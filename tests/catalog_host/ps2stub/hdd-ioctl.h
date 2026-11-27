#ifndef TEAG3_TEST_HDD_IOCTL_H
#define TEAG3_TEST_HDD_IOCTL_H
#include <tamtypes.h>
#define APA_IDMAX 32
#define APA_FLAG_SUB 0x0001u
#define HDIOC_READSECTOR 0x1234
typedef struct {
    u32 lba;
    u32 nsectors;
} hddAtaTransfer_t;
#endif
