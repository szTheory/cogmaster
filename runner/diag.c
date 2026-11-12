/* --diagnose: what to watch, and the one line it prints (diag.h). The frame-600 verdict is the
   boots predicate of --expect, computed from the same state by the same function. */
#include "diag.h"

#include <stdio.h>
#include <string.h>

#include "exit_status.h"
#include "probe.h"

const char *cogmaster__fault_kind_name(uint32_t kind)
{
    static const char *const kinds[] = {
        [0] = "none",
        [COGMASTER_FAULT_KIND_UNMAPPED_READ] = "unmapped_read",
        [COGMASTER_FAULT_KIND_UNMAPPED_WRITE] = "unmapped_write",
        [COGMASTER_FAULT_KIND_ROM_WRITE] = "rom_write",
        [COGMASTER_FAULT_KIND_MAPPER_RANGE] = "mapper_range",
        [COGMASTER_FAULT_KIND_ED_UNDEFINED] = "ed_undefined",
        [COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE] = "unsupported_video_mode",
    };
    return kind < sizeof kinds / sizeof kinds[0] ? kinds[kind] : "unknown";
}

const char *cogmaster__fault_device_name(uint32_t device)
{
    static const char *const devices[] = {
        [COGMASTER_FAULT_DEVICE_NONE] = "none",     [COGMASTER_FAULT_DEVICE_CPU] = "cpu",
        [COGMASTER_FAULT_DEVICE_CART] = "cart",     [COGMASTER_FAULT_DEVICE_RAM] = "ram",
        [COGMASTER_FAULT_DEVICE_MAPPER] = "mapper", [COGMASTER_FAULT_DEVICE_CTRL] = "ctrl",
        [COGMASTER_FAULT_DEVICE_VDP] = "vdp",
    };
    return device < sizeof devices / sizeof devices[0] ? devices[device] : "unknown";
}

void cogmaster__diag_init(cogmaster__diag *d)
{
    memset(d, 0, sizeof *d);
}

int cogmaster__diag_frame(cogmaster__diag *d, const cogmaster *m, uint32_t call, uint32_t display,
                          uint16_t *pixels)
{
    char hex[65];
    uint32_t nonblank = 0u;
    uint32_t before = d->e.distinct;
    int status = cogmaster__expect_observe(&d->e, m, display, pixels, hex, &nonblank);
    if (status != COGMASTER__EXIT_OK)
        return status;
    if (display && d->first_display == 0u)
        d->first_display = call;
    if (nonblank && d->first_nonblank == 0u)
        d->first_nonblank = call;
    if (before < 2u && d->e.distinct == 2u)
        d->first_change = call;
    uint32_t known = 0u;
    for (uint32_t i = 0u; i < d->held; i++)
        known |= (uint32_t)(strcmp(d->seen[i], hex) == 0);
    if (!known && d->held < COGMASTER__DIAG_HELD)
        memcpy(d->seen[d->held++], hex, sizeof hex);
    if (call == 600u) {
        d->distinct600 = d->held;
        d->verdict600 = (uint32_t)cogmaster__expect_boots(&d->e);
    }
    return COGMASTER__EXIT_OK;
}

/* A frame number as text: the number, or "none" for 0. */
static void number(char out[12], uint32_t v)
{
    if (v == 0u)
        memcpy(out, "none", 5u);
    else
        snprintf(out, 12u, "%lu", (unsigned long)v);
}

int cogmaster__diag_print(const cogmaster__diag *d, const cogmaster *m, const uint8_t *image,
                          size_t image_size, uint32_t ran)
{
    if (ran < COGMASTER__DIAG_FRAMES) {
        fprintf(stderr, "cogmaster-run: --diagnose stopped before its last frame\n");
        return COGMASTER__EXIT_FAILURE;
    }
    cogmaster_fault fault = {0};
    fault.size = sizeof fault;
    cogmaster_cart_info cart = {0};
    cart.size = sizeof cart;
    if (cogmaster_get_fault(m, &fault) != COGMASTER_STATUS_OK ||
        cogmaster_probe(image, image_size, &cart) != COGMASTER_STATUS_OK)
        return COGMASTER__EXIT_FAILURE;
    char display[12], nonblank[12], change[12];
    number(display, d->first_display);
    number(nonblank, d->first_nonblank);
    number(change, d->first_change);
    char pc[5] = "none";
    if (fault.present)
        snprintf(pc, sizeof pc, "%04X", (unsigned)fault.pc);
    printf("diagnose 1 verdict600=%s first_display=%s first_nonblank=%s first_change=%s "
           "distinct600=%lu distinct1800=%lu fault=%s dev=%s pc=%s mapper=%s\n",
           d->verdict600 ? "pass" : "fail", display, nonblank, change,
           (unsigned long)d->distinct600, (unsigned long)d->held,
           fault.present ? cogmaster__fault_kind_name(fault.kind) : "none",
           fault.present ? cogmaster__fault_device_name(fault.device) : "none", pc,
           cogmaster__board_name(cart.board));
    return COGMASTER__EXIT_OK;
}
