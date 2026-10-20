/* SingleStepTests z80 vectors: the test record, the committed sample's
   format and the per-T-state bus stream. The converter and the harness both
   build every stream through sst_stream_build, so they cannot disagree about
   the cycle templates.

   Sample format, little-endian, read byte-wise:
     magic "CGM-Z80V", u32 version (1), u32 tests per file, u32 file count,
     then per file, in ascending sst_file_name order, its u16 file index and
     that many test records.
   Test record:
     initial state: every field in SST_FIELDS order, u8 or u16 by width;
     final state: u32 mask of the fields that differ from the initial state,
       then those fields in order;
     initial RAM: u8 count, then count x (u16 addr, u8 value);
     final RAM: u8 count; with bit 7 set the addresses are the initial RAM's
       in order and only count & 0x7F u8 values follow, otherwise as above;
     ports: u8 count, then count x (u16 addr, u8 value, u8 'r' or 'w');
     u8 total T-states; u8 access count, then per access a u8 kind (bit 7
       set when a u8 start T-state follows, clear when the access starts
       where the previous one ended, or at 0), u16 addr, u8 data, and for an
       M1 a u16 refresh address.
   A test is named by its file and its zero-based index within that file. */
#ifndef COGMASTER_SST_STREAM_H
#define COGMASTER_SST_STREAM_H

#include <stddef.h>
#include <stdint.h>

#define SST_MAGIC "CGM-Z80V"
#define SST_VERSION 1u
#define SST_FILES 1604u
#define SST_MAX_RAM 32u
#define SST_MAX_PORTS 8u
#define SST_MAX_ACCESS 16u
#define SST_MAX_T 64u
#define SST_NAME_MAX 16u

/* The state fields, in sample order; the names are upstream's keys. */
#define SST_FIELDS 25u
extern const char *const sst_field_names[SST_FIELDS];
extern const uint8_t sst_field_width[SST_FIELDS]; /* 1 or 2 bytes */

/* Bus cycle kinds: an opcode fetch with its refresh, memory read and write,
   I/O read and write. */
enum { SST_M1 = 1, SST_MR = 2, SST_MW = 3, SST_IOR = 4, SST_IOW = 5 };

/* Pin letters of one T-state, as bits: "rwmi" upstream, '-' when low. */
enum { SST_PIN_R = 1, SST_PIN_W = 2, SST_PIN_M = 4, SST_PIN_I = 8 };

struct sst_access {
    uint8_t kind;
    uint8_t t; /* the T-state the bus cycle starts on, from 0 */
    uint16_t addr;
    uint8_t data;
    uint16_t refresh; /* M1 only */
};

struct sst_tick {
    uint16_t addr;
    int16_t data; /* -1 when nothing is on the data bus */
    uint8_t pins;
};

struct sst_cell {
    uint16_t addr;
    uint8_t value;
};

struct sst_port {
    uint16_t addr;
    uint8_t value;
    uint8_t dir; /* 'r' or 'w' */
};

struct sst_test {
    uint16_t initial[SST_FIELDS];
    uint16_t final[SST_FIELDS];
    struct sst_cell ram_in[SST_MAX_RAM];
    struct sst_cell ram_out[SST_MAX_RAM];
    struct sst_port ports[SST_MAX_PORTS];
    struct sst_access access[SST_MAX_ACCESS];
    unsigned n_ram_in, n_ram_out, n_ports, n_access, total_t;
};

/* Expands n accesses into total per-T-state entries of out (SST_MAX_T room).
   M1: [pc,-,----][pc,-,r-m-][refresh,op,----][refresh,-,----]; memory read:
   [a,-,----][a,-,r-m-][a,d,----]; memory write: [a,-,----][a,d,-wm-][a,-,----];
   I/O read: [p,-,----][p,-,----][p,-,r--i][p,d,----]; I/O write:
   [p,-,----][p,-,----][p,d,-w-i][p,-,----]; any other T-state is idle and
   repeats the previous address with no data and no pins. Returns 0, or -1
   when accesses overlap, run out of order or past total, or total exceeds
   SST_MAX_T. */
int sst_stream_build(const struct sst_access *a, unsigned n, unsigned total, struct sst_tick *out);

/* Writes the pins of one T-state as upstream's four letters plus a NUL. */
void sst_pins_text(uint8_t pins, char out[5]);

/* The upstream file name of file index (0 to SST_FILES - 1) without
   ".json", for example "02", "dd 34" or "fd cb __ 7e". */
void sst_file_name(unsigned index, char out[SST_NAME_MAX]);

/* The prefix group of file index: "base", "cb", "dd", "ddcb", "ed", "fd" or
   "fdcb". */
const char *sst_file_group(unsigned index);

/* Appends one encoded test to out (at least 1024 bytes free); returns the
   number of bytes written. */
size_t sst_encode(const struct sst_test *t, uint8_t *out);

/* Decodes one test from buf[*pos..len), advancing *pos. Returns 0, or -1 on
   a truncated or out-of-range record. */
int sst_decode(const uint8_t *buf, size_t len, size_t *pos, struct sst_test *t);

#endif /* COGMASTER_SST_STREAM_H */
