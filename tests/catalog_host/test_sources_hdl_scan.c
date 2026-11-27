#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fileXio_rpc.h"
#include "hdd-ioctl.h"
#include "sources.h"

static iox_dirent_t entries[2][3];
static int entry_count[2];
static int read_count[2];
static int open_count;
static int close_count;
static int read_error_pass;
static int close_error_fd;
static int open_error_fd;
static int sector_error;
static int cancel_requested;
static int cancel_on_sector;
static int cancel_on_second_pass_read;

typedef struct {
    int calls;
    char part[APA_IDMAX + 1];
    char title[161];
    char startup[61];
    u32 start_sector;
    u32 size_kb;
    u32 disc_type;
    int is_cd;
} scan_result;

static void reset_fixture(void)
{
    memset(entries, 0, sizeof(entries));
    memset(entry_count, 0, sizeof(entry_count));
    memset(read_count, 0, sizeof(read_count));
    open_count = 0;
    close_count = 0;
    read_error_pass = 0;
    close_error_fd = 0;
    open_error_fd = 0;
    sector_error = 0;
    cancel_requested = 0;
    cancel_on_sector = 0;
    cancel_on_second_pass_read = 0;
}

static void set_base_partition(iox_dirent_t *entry, u64 size)
{
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->name, "PP.TEST01");
    entry->stat.mode = 0x1337;
    entry->stat.size = size;
    entry->stat.private_5 = 100;
}

static void set_sub_partition(iox_dirent_t *entry, u64 size)
{
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->name, "PP.TEST01"); /* APA sub-partitions retain the base partition ID. */
    entry->stat.mode = 0x1337;
    entry->stat.attr = APA_FLAG_SUB;
    entry->stat.size = size;
}

int fileXioDopen(const char *path)
{
    assert(strcmp(path, "hdd0:") == 0);
    int fd = ++open_count;
    return fd == open_error_fd ? -1 : fd;
}

int fileXioDread(int fd, iox_dirent_t *entry)
{
    assert(fd == 1 || fd == 2);
    int pass = fd - 1;
    if (read_count[pass] < entry_count[pass]) {
        *entry = entries[pass][read_count[pass]++];
        if (fd == 2 && cancel_on_second_pass_read) cancel_requested = 1;
        return 1;
    }
    return read_error_pass == fd ? -1 : 0;
}

int fileXioDclose(int fd)
{
    close_count++;
    return fd == close_error_fd ? -1 : 0;
}

int fileXioDevctl(const char *device, int command, void *arg, int arg_size, void *buffer, int buffer_size)
{
    (void)arg_size;
    assert(strcmp(device, "hdd0:") == 0);
    assert(command == HDIOC_READSECTOR);
    assert(arg != NULL);
    assert(buffer_size >= 1024);
    if (sector_error) return -1;
    memset(buffer, 0, (size_t)buffer_size);
    u32 magic = 0xDEADFEED;
    u32 disc_type = 0x12;
    memcpy(buffer, &magic, sizeof(magic));
    memcpy((char *)buffer + 8, "Test Game", 9);
    memcpy((char *)buffer + 172, "SLUS_123.45", 11);
    memcpy((char *)buffer + 236, &disc_type, sizeof(disc_type));
    if (cancel_on_sector) cancel_requested = 1;
    return 0;
}

static int scan_cancel_requested(void *context)
{
    return *(const int *)context;
}

static void collect_game(void *context, const char *part, const char *title, const char *startup,
                         u32 start_sector, u32 total_size_kb, u32 disc_type, int is_cd)
{
    scan_result *result = context;
    result->calls++;
    snprintf(result->part, sizeof(result->part), "%s", part);
    snprintf(result->title, sizeof(result->title), "%s", title);
    snprintf(result->startup, sizeof(result->startup), "%s", startup);
    result->start_sector = start_sector;
    result->size_kb = total_size_kb;
    result->disc_type = disc_type;
    result->is_cd = is_cd;
}

static void test_success_aggregates_all_apa_parts(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 2000);
    entry_count[0] = 1;
    set_sub_partition(&entries[1][0], 500);
    set_sub_partition(&entries[1][1], 300);
    entry_count[1] = 2;
    scan_result result = {0};

    int count = src_hdl_scan(collect_game, &result);
    assert(count == 1);
    assert(result.calls == 1);
    assert(strcmp(result.part, "PP.TEST01") == 0);
    assert(strcmp(result.title, "Test Game") == 0);
    assert(strcmp(result.startup, "SLUS_123.45") == 0);
    assert(result.start_sector == 2164);
    assert(result.size_kb == 1400);
    assert(result.disc_type == 0x12);
    assert(result.is_cd == 1);
    assert(open_count == 2 && close_count == 2);
}

static void test_first_directory_read_error_publishes_nothing(void)
{
    reset_fixture();
    read_error_pass = 1;
    scan_result result = {0};
    assert(src_hdl_scan(collect_game, &result) == SRC_HDL_SCAN_ERR_READ);
    assert(result.calls == 0);
    assert(open_count == 1 && close_count == 1);
}

static void test_second_directory_read_error_publishes_nothing(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 2000);
    entry_count[0] = 1;
    set_sub_partition(&entries[1][0], 500);
    entry_count[1] = 1;
    read_error_pass = 2;
    scan_result result = {0};
    assert(src_hdl_scan(collect_game, &result) == SRC_HDL_SCAN_ERR_READ);
    assert(result.calls == 0);
    assert(open_count == 2 && close_count == 2);
}

static void test_close_error_publishes_nothing(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 2000);
    entry_count[0] = 1;
    close_error_fd = 1;
    scan_result result = {0};
    assert(src_hdl_scan(collect_game, &result) == SRC_HDL_SCAN_ERR_CLOSE);
    assert(result.calls == 0);
    assert(open_count == 1 && close_count == 1);
}

static void test_second_open_failure_publishes_nothing(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 2000);
    entry_count[0] = 1;
    open_error_fd = 2;
    scan_result result = {0};
    assert(src_hdl_scan(collect_game, &result) == SRC_HDL_SCAN_ERR_OPEN);
    assert(result.calls == 0);
    assert(open_count == 2 && close_count == 1);
}

static void test_second_close_error_publishes_nothing(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 2000);
    entry_count[0] = 1;
    set_sub_partition(&entries[1][0], 500);
    entry_count[1] = 1;
    close_error_fd = 2;
    scan_result result = {0};
    assert(src_hdl_scan(collect_game, &result) == SRC_HDL_SCAN_ERR_CLOSE);
    assert(result.calls == 0);
    assert(open_count == 2 && close_count == 2);
}

static void test_sector_read_error_publishes_nothing(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 2000);
    entry_count[0] = 1;
    sector_error = 1;
    scan_result result = {0};
    assert(src_hdl_scan(collect_game, &result) == SRC_HDL_SCAN_ERR_READ);
    assert(result.calls == 0);
    assert(close_count == 1);
}

static void test_base_partition_size_overflow_publishes_nothing(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 0x200000000ULL);
    entry_count[0] = 1;
    scan_result result = {0};
    assert(src_hdl_scan(collect_game, &result) == SRC_HDL_SCAN_ERR_METADATA);
    assert(result.calls == 0);
    assert(open_count == 1 && close_count == 1);
}

static void test_cancel_before_open_publishes_nothing(void)
{
    reset_fixture();
    cancel_requested = 1;
    scan_result result = {0};
    assert(src_hdl_scan_cancelable(collect_game, &result, scan_cancel_requested, &cancel_requested) ==
           SRC_HDL_SCAN_ERR_CANCELLED);
    assert(result.calls == 0);
    assert(open_count == 0 && close_count == 0);
}

static void test_cancel_after_sector_read_closes_first_pass(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 2000);
    entry_count[0] = 1;
    cancel_on_sector = 1;
    scan_result result = {0};
    assert(src_hdl_scan_cancelable(collect_game, &result, scan_cancel_requested, &cancel_requested) ==
           SRC_HDL_SCAN_ERR_CANCELLED);
    assert(result.calls == 0);
    assert(open_count == 1 && close_count == 1);
}

static void test_cancel_during_second_pass_closes_both_passes(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 2000);
    entry_count[0] = 1;
    set_sub_partition(&entries[1][0], 500);
    entry_count[1] = 1;
    cancel_on_second_pass_read = 1;
    scan_result result = {0};
    assert(src_hdl_scan_cancelable(collect_game, &result, scan_cancel_requested, &cancel_requested) ==
           SRC_HDL_SCAN_ERR_CANCELLED);
    assert(result.calls == 0);
    assert(open_count == 2 && close_count == 2);
}

static void test_subpartition_size_overflow_publishes_nothing(void)
{
    reset_fixture();
    set_base_partition(&entries[0][0], 0x1fffffffeULL);
    entry_count[0] = 1;
    set_sub_partition(&entries[1][0], 100);
    entry_count[1] = 1;
    scan_result result = {0};
    assert(src_hdl_scan(collect_game, &result) == SRC_HDL_SCAN_ERR_METADATA);
    assert(result.calls == 0);
    assert(close_count == 2);
}

int main(void)
{
    test_success_aggregates_all_apa_parts();
    test_first_directory_read_error_publishes_nothing();
    test_second_directory_read_error_publishes_nothing();
    test_close_error_publishes_nothing();
    test_second_open_failure_publishes_nothing();
    test_second_close_error_publishes_nothing();
    test_sector_read_error_publishes_nothing();
    test_base_partition_size_overflow_publishes_nothing();
    test_subpartition_size_overflow_publishes_nothing();
    test_cancel_before_open_publishes_nothing();
    test_cancel_after_sector_read_closes_first_pass();
    test_cancel_during_second_pass_closes_both_passes();
    puts("HDL host scanner: PASS (APA aggregation, read/sector/close errors, overflow, cancellation, no partial callbacks)");
    return 0;
}
