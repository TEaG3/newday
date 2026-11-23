#pragma once

// 1 = valid ZSO opened for the current file descriptor, 0 = not a ZSO, -1 = malformed/unreadable ZSO.
int zso_ee_open(int fd);
int zso_ee_read(unsigned lba, void *buf, unsigned sectors); // 1 = all logical sectors read
void zso_ee_close(void);
