#ifndef TEAG3_TEST_PS2_KERNEL_H
#define TEAG3_TEST_PS2_KERNEL_H

typedef struct {
    int count;
    int max_count;
    int init_count;
    int wait_threads;
    unsigned int attr;
    unsigned int option;
} ee_sema_t;

int CreateSema(ee_sema_t *sema);
int DeleteSema(int sema_id);
int WaitSema(int sema_id);
int SignalSema(int sema_id);

#endif
