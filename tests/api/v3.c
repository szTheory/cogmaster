/* API version 3's absolute input and appended per-frame result. */
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "../check.h"

#if COGMASTER_API_VERSION >= 3
static void *v3_alloc(void *u, size_t n) { (void)u; return malloc(n); }
static void v3_free(void *u, void *p, size_t n) { (void)u; (void)n; free(p); }
#endif
int main(void)
{
    CHECK(COGMASTER_API_VERSION >= 3);
#if COGMASTER_API_VERSION >= 3
    CHECK_EQ_U64(COGMASTER_DEVICE_PAD, 0u);
    CHECK_EQ_U64(COGMASTER_DEVICE_NONE, 1u);
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg; cfg.allocator.alloc = v3_alloc; cfg.allocator.free = v3_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_input in = {0};
    in.size = sizeof in;
    CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_ERR_NO_CONTENT);
    cogmaster_media media = {0}; media.size = sizeof media;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_OK);
    in.p1 = 64u;
    CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_ERR_ARGUMENT);
    in.p1 = 0u; in.pause = 2u;
    CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_ERR_ARGUMENT);
    in.pause = 0u; in.port2 = 2u;
    CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_ERR_ARGUMENT);
    in.port2 = 0u; in.size--;
    CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_ERR_STRUCT_SIZE);
    cogmaster_run_result r = {0}; r.size = sizeof r;
    CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(r.lagged, 1u);
    /* A legacy 12-byte result cannot overwrite the appended word. */
    r.size = 12u; r.lagged = 0xABCDu;
    CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
    CHECK_EQ_HEX(r.lagged, 0xABCDu);
    cogmaster_destroy(m);
#endif
    CHECK_DONE();
}
