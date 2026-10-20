/* API v6: fixed battery media and mapped-write generations. */
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "../check.h"

static void *allocate(void *u, size_t n)
{
    (void)u;
    return malloc(n);
}
static void release(void *u, void *p, size_t n)
{
    (void)u;
    (void)n;
    free(p);
}
int main(void)
{
    CHECK(COGMASTER_API_VERSION >= 6);
    CHECK_EQ_U64(COGMASTER_STATUS_ERR_SAVE_SIZE, 9u);
    CHECK_EQ_U64(COGMASTER_BATTERY_UNKNOWN, 0u);
    CHECK_EQ_U64(COGMASTER_BATTERY_NONE, 1u);
    CHECK_EQ_U64(COGMASTER_BATTERY_KNOWN, 2u);
    CHECK_EQ_U64(COGMASTER_BATTERY_UNSUPPORTED, 3u);
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = allocate;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_battery b;
    memset(&b, 0x5a, sizeof b);
    b.size = sizeof b;
    uint8_t before[sizeof b];
    memcpy(before, &b, sizeof b);
    CHECK_EQ_U64(cogmaster_get_battery(m, &b), COGMASTER_STATUS_ERR_NO_CONTENT);
    CHECK(memcmp(&b, before, sizeof b) == 0);
    CHECK_EQ_U64(cogmaster_get_battery(NULL, &b), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_battery(m, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    cogmaster_media media = {0};
    media.size = sizeof media;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_get_battery(m, &b), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(b.knowledge, COGMASTER_BATTERY_NONE);
    CHECK(b.data == NULL);
    CHECK_EQ_U64(b.length, 0u);
    CHECK_EQ_U64(b.generation, 0u);
    struct {
        cogmaster_battery b;
        uint8_t tail[16];
    } extended;
    memset(&extended, 0x5a, sizeof extended);
    extended.b.size = sizeof extended;
    CHECK_EQ_U64(cogmaster_get_battery(m, &extended.b), COGMASTER_STATUS_OK);
    CHECK_EQ_HEX(extended.tail[0], 0x5au);
    cogmaster_destroy(m);
    CHECK_DONE();
}
