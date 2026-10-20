/* The RAM mailbox (CONFORMANCE section 4 of the preparation notes, and
   docs/runner.md): work RAM at $C100-$C1FF holds the magic "CMDG", a
   protocol version, a status, a record count, a ROM id, a CRC-16 over the
   records and up to 60 four-byte records (test id, flags, value low, value
   high). The runner reads it with cogmaster_peek, which changes nothing. */
#include "mailbox.h"

#include <stdio.h>
#include <string.h>

#define MB_BASE 0xC100u
#define MB_STATUS 0x05u
#define MB_COUNT 0x06u
#define MB_ROM 0x07u
#define MB_CRC 0x08u
#define MB_RECORDS 0x10u
#define MB_RECORDS_MAX 60u
#define MB_SIZE 256u

uint16_t cogmaster__crc16_ccitt(const uint8_t *p, size_t n)
{
    uint32_t c = 0xFFFFu;
    for (size_t i = 0; i < n; i++) {
        c ^= (uint32_t)p[i] << 8;
        for (int b = 0; b < 8; b++) {
            c = ((c & 0x8000u) != 0u ? c << 1 ^ 0x1021u : c << 1) & 0xFFFFu;
        }
    }
    return (uint16_t)c;
}

/* A loaded instance peeks a byte, 0 to 255, at every bus address. */
static uint8_t peek(const cogmaster *m, uint32_t addr)
{
    return (uint8_t)(cogmaster_peek(m, COGMASTER_SPACE_BUS, addr) & 0xFFu);
}

int cogmaster__mailbox_done(const cogmaster *m)
{
    return peek(m, MB_BASE + MB_STATUS) == 0u;
}

void cogmaster__mailbox_print(const cogmaster *m)
{
    uint8_t mb[MB_SIZE];
    for (uint32_t i = 0; i < MB_SIZE; i++) {
        mb[i] = peek(m, MB_BASE + i);
    }
    uint32_t want = (uint32_t)mb[MB_CRC] | (uint32_t)mb[MB_CRC + 1u] << 8;
    uint32_t got = cogmaster__crc16_ccitt(mb + MB_RECORDS, MB_SIZE - MB_RECORDS);
    printf("status=%02X\nrom=%02X\nmagic=%s\ncrc=%s\n", (unsigned)mb[MB_STATUS],
           (unsigned)mb[MB_ROM], memcmp(mb, "CMDG", 4u) == 0 ? "ok" : "bad",
           want == got ? "ok" : "bad");
    uint32_t count =
        (uint32_t)mb[MB_COUNT] < MB_RECORDS_MAX ? (uint32_t)mb[MB_COUNT] : MB_RECORDS_MAX;
    for (uint32_t n = 0; n < count; n++) {
        const uint8_t *r = mb + MB_RECORDS + 4u * n;
        printf("%02X=%04X flags=%X\n", (unsigned)r[0], (unsigned)r[2] | (unsigned)r[3] << 8,
               (unsigned)r[1]);
    }
}
