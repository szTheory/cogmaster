/* The controller a dump defaults to and the region a paddle dump needs, over every row of the
   hash table: cogmaster-test-cart-device <case>. Each expected value below is written again from
   the table's own codes and the availability rule of the ports, and shares no code with the
   library; the test asserts per row and never counts rows of one kind, so it does not move when
   rows are added. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "cartdb.h"
#include "../check.h"

#define NOT_SET 0x5A5Au /* a region guess the rows must leave alone unless they name one */
/* The table's controller codes; any other, including 4, reads as a pad. */
#define CODE_PHASER 1u
#define CODE_PADDLE 2u
#define CODE_SPORTS 3u

static uint8_t image[1024];

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static uint32_t intent(uint8_t code)
{
    return code == CODE_PHASER   ? COGMASTER_DEVICE_LIGHT_PHASER
           : code == CODE_PADDLE ? COGMASTER_DEVICE_PADDLE
           : code == CODE_SPORTS ? COGMASTER_DEVICE_SPORTS_PAD
                                 : COGMASTER_DEVICE_PAD;
}

/* The availability rule: a Sports Pad or a Light Phaser on any console but a Game Gear, a Paddle
   only on a Japanese console. */
static uint32_t resolved(uint32_t want, uint32_t machine, uint32_t region)
{
    if ((want == COGMASTER_DEVICE_SPORTS_PAD || want == COGMASTER_DEVICE_LIGHT_PHASER) &&
        machine != COGMASTER_MACHINE_GG)
        return want;
    if (want == COGMASTER_DEVICE_PADDLE && machine != COGMASTER_MACHINE_GG &&
        region == COGMASTER_REGION_JAPAN)
        return want;
    return COGMASTER_DEVICE_PAD;
}

static int test_defaults(void)
{
    size_t rows = 0;
    for (size_t i = 0;; i++) {
        const struct cogmaster__cart_fact *row = cogmaster__cartdb_row(i);
        if (row == NULL)
            break;
        cogmaster_cart_info info;
        memset(&info, 0, sizeof info);
        info.size = (uint32_t)sizeof info;
        (void)cogmaster__cart_fact_to_info(row, &info);
        if (info.default_port1 != intent(row->ctrl1) || info.default_port2 != intent(row->ctrl2)) {
            fprintf(stderr, "row %lu: ctrl %u %u gave ports %u %u\n", (unsigned long)i,
                    (unsigned)row->ctrl1, (unsigned)row->ctrl2, (unsigned)info.default_port1,
                    (unsigned)info.default_port2);
            check_failures++;
        }
        rows++;
    }
    CHECK(rows > 0u);
    return 0;
}

static int test_region(void)
{
    for (size_t i = 0;; i++) {
        const struct cogmaster__cart_fact *row = cogmaster__cartdb_row(i);
        if (row == NULL)
            break;
        cogmaster_cart_info info;
        memset(&info, 0, sizeof info);
        info.region_guess = NOT_SET;
        (void)cogmaster__cart_fact_to_info(row, &info);
        int paddle = row->ctrl1 == CODE_PADDLE || row->ctrl2 == CODE_PADDLE;
        uint32_t want = row->region != 0u ? row->region : paddle ? COGMASTER_REGION_JAPAN : NOT_SET;
        if (info.region_guess != want) {
            fprintf(stderr, "row %lu: region %u ctrl %u %u read as %u, expected %u\n",
                    (unsigned long)i, (unsigned)row->region, (unsigned)row->ctrl1,
                    (unsigned)row->ctrl2, (unsigned)info.region_guess, (unsigned)want);
            check_failures++;
        }
    }
    return 0;
}

static cogmaster *build(uint32_t machine, uint32_t region, int with_image)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL)
        exit(1);
    cogmaster_media md;
    memset(&md, 0, sizeof md);
    md.size = (uint32_t)sizeof md;
    md.machine = machine;
    md.region = region;
    if (with_image) {
        memset(image, 0x76u, sizeof image);
        md.image = image;
        md.image_size = sizeof image;
    }
    CHECK_EQ_U64(cogmaster_load(m, &md), COGMASTER_STATUS_OK);
    return m;
}

static void info_ports(cogmaster *m, uint32_t *p1, uint32_t *p2)
{
    cogmaster_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(cogmaster_get_info(m, &info), COGMASTER_STATUS_OK);
    *p1 = info.default_port1;
    *p2 = info.default_port2;
}

/* A row's intent put into an instance as a load of that dump would leave it. */
static void hold(cogmaster *m, const cogmaster_cart_info *info)
{
    m->table_device[0] = (uint8_t)info->default_port1;
    m->table_device[1] = (uint8_t)info->default_port2;
}

static int test_degrade(void)
{
    static const struct {
        uint32_t machine, region;
    } profile[3] = {{COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT},
                    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN},
                    {COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT}};
    for (size_t k = 0; k < 3u; k++) {
        cogmaster *m = build(profile[k].machine, profile[k].region, 0);
        for (size_t i = 0;; i++) {
            const struct cogmaster__cart_fact *row = cogmaster__cartdb_row(i);
            if (row == NULL)
                break;
            cogmaster_cart_info info;
            memset(&info, 0, sizeof info);
            (void)cogmaster__cart_fact_to_info(row, &info);
            hold(m, &info);
            uint32_t p1 = 99u, p2 = 99u;
            info_ports(m, &p1, &p2);
            uint32_t w1 = resolved(intent(row->ctrl1), profile[k].machine, profile[k].region);
            uint32_t w2 = resolved(intent(row->ctrl2), profile[k].machine, profile[k].region);
            if (p1 != w1 || p2 != w2) {
                fprintf(
                    stderr, "profile %lu row %lu: ctrl %u %u resolved to %u %u, expected %u %u\n",
                    (unsigned long)k, (unsigned long)i, (unsigned)row->ctrl1, (unsigned)row->ctrl2,
                    (unsigned)p1, (unsigned)p2, (unsigned)w1, (unsigned)w2);
                check_failures++;
            }
        }
        /* The three cases the rule is named for, stated outright. */
        m->table_device[0] = COGMASTER_DEVICE_PADDLE;
        m->table_device[1] = COGMASTER_DEVICE_SPORTS_PAD;
        uint32_t p1 = 99u, p2 = 99u;
        info_ports(m, &p1, &p2);
        CHECK_EQ_U64(p1, profile[k].region == COGMASTER_REGION_JAPAN &&
                                 profile[k].machine != COGMASTER_MACHINE_GG
                             ? COGMASTER_DEVICE_PADDLE
                             : COGMASTER_DEVICE_PAD);
        CHECK_EQ_U64(p2, profile[k].machine != COGMASTER_MACHINE_GG ? COGMASTER_DEVICE_SPORTS_PAD
                                                                    : COGMASTER_DEVICE_PAD);
        cogmaster_destroy(m);
    }
    return 0;
}

static int test_none(void)
{
    /* A zero-length image names no row, and a probe of it carries a pad on both ports. */
    cogmaster_cart_info info;
    memset(&info, 0xA5, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK(cogmaster_probe(image, 0u, &info) != COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.default_port1, COGMASTER_DEVICE_PAD);
    CHECK_EQ_U64(info.default_port2, COGMASTER_DEVICE_PAD);
    /* An image whose SHA-1 is in no row reads as a pad on both ports. */
    memset(image, 0x76u, sizeof image);
    memset(&info, 0xA5, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(cogmaster_probe(image, sizeof image, &info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.identified, 0u);
    CHECK_EQ_U64(info.default_port1, COGMASTER_DEVICE_PAD);
    CHECK_EQ_U64(info.default_port2, COGMASTER_DEVICE_PAD);
    /* A Japanese console with no content and with an unmatched image resolves to a pad on both
       ports, also after a table intent was held by an earlier load. */
    for (int with_image = 0; with_image < 2; with_image++) {
        cogmaster *m = build(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, with_image);
        uint32_t p1 = 99u, p2 = 99u;
        info_ports(m, &p1, &p2);
        CHECK_EQ_U64(p1, COGMASTER_DEVICE_PAD);
        CHECK_EQ_U64(p2, COGMASTER_DEVICE_PAD);
        m->table_device[0] = COGMASTER_DEVICE_PADDLE;
        m->table_device[1] = COGMASTER_DEVICE_SPORTS_PAD;
        info_ports(m, &p1, &p2);
        CHECK_EQ_U64(p1, COGMASTER_DEVICE_PADDLE);
        CHECK_EQ_U64(p2, COGMASTER_DEVICE_SPORTS_PAD);
        cogmaster_media md;
        memset(&md, 0, sizeof md);
        md.size = (uint32_t)sizeof md;
        md.region = COGMASTER_REGION_JAPAN;
        if (with_image) {
            md.image = image;
            md.image_size = sizeof image;
        }
        CHECK_EQ_U64(cogmaster_load(m, &md), COGMASTER_STATUS_OK);
        info_ports(m, &p1, &p2);
        CHECK_EQ_U64(p1, COGMASTER_DEVICE_PAD);
        CHECK_EQ_U64(p2, COGMASTER_DEVICE_PAD);
        cogmaster_destroy(m);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "defaults") == 0)
        test_defaults();
    else if (strcmp(argv[1], "region") == 0)
        test_region();
    else if (strcmp(argv[1], "degrade") == 0)
        test_degrade();
    else if (strcmp(argv[1], "none") == 0)
        test_none();
    else
        return 2;
    CHECK_DONE();
}
