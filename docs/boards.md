# Boards and probing

How an image is probed and which cartridge board it gets. The calls and structs are in [embedding.md](embedding.md).

## Probing an image

`cogmaster_probe(image, n, out)` reads an image the way `load` does, without an instance: it fills a
`cogmaster_cart_info` and returns the status `load` would give for the image alone. Appended fields distinguish
`COGMASTER_PERSISTENCE_UNKNOWN`, `COGMASTER_PERSISTENCE_NONE` and `COGMASTER_PERSISTENCE_BATTERY`.
`persistent_size` is the declared extent, independent of mapper decoding. `persistence_geometry` is
`COGMASTER_GEOMETRY_NONE`, `COGMASTER_GEOMETRY_SUPPORTED` or `COGMASTER_GEOMETRY_UNSUPPORTED`.
Unknown/unsupported geometry provides no persistent span. `identified` means a whole-body SHA-1 match;
`public_identity` distinguishes public CC0 identities from owned fixture facts. `sha1` is a zero-terminated
identity string, empty on a miss. `title` is always empty: titles live in `tests/compat/catalog.tsv`, not in the
core. A database hit sets `board_source` to `COGMASTER_BOARD_SOURCE_DB`. A database row may be unsupported by
design: the probe returns `COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD` with `board` 0, the row's slot id in
`unsupported_id` and `<slot>: <reason>` in `unsupported_reason`. `video_mode` carries a verified GG SMS-mode fact.
Old 52-byte output tags leave the appended fields untouched. AUTO machine/region use table/header facts
before defaults; explicit choices win. These facts are independent of compatibility-ledger membership.
The probe reads only the `n` bytes
at `image`, never changes them, and gives the same answer for the same bytes. It returns
`COGMASTER_STATUS_ERR_ARGUMENT` for a NULL `image` or `out`, `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE` outside the
load bounds, including a body above 1 MiB after any copier header (with only `image_size` filled), and `COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD` with `board` 0 and
every other field filled when no supported board fits.

| Field | Meaning |
|---|---|
| `image_size` | `n` |
| `body_offset` | 512 after a copier header, else 0 |
| `header_at` | where the ROM header was found: `0x7FF0`, `0x3FF0`, `0x1FF0`, or 0 for none |
| `checksum`, `checksum_ok` | the header's checksum, and 1 when it matches the sum over the range its size code names |
| `product_code`, `version` | the product number (decimal) and the version nibble |
| `region_code`, `rom_size_code` | the header's region and size nibbles |
| `machine_guess`, `region_guess` | the machine and region the region code names (`COGMASTER_MACHINE_*`, `COGMASTER_REGION_*`), or 0 |
| `board` | `COGMASTER_BOARD_ROM_ONLY` (1), `COGMASTER_BOARD_SEGA` (2), `COGMASTER_BOARD_CODEMASTERS` (3), `COGMASTER_BOARD_KOREAN` (4), `COGMASTER_BOARD_KOREAN_NB` (5) or `COGMASTER_BOARD_HICOM` (6); 0 for none |

Every header field is 0 when no header was found. The board is chosen in this order:

1. When the size is 512 bytes past a multiple of 16 KiB, the first 512 bytes are a copier's header and are skipped.
2. The ROM header is looked for at `$7FF0`, then `$3FF0`, then `$1FF0`. Its region code names the machine and
   region; its checksum is checked but a wrong one refuses nothing.
3. A Codemasters header at `$7FE0` (page count, BCD date, checksum and its complement, `$7FEF` zero) gives the
   Codemasters board.
4. An 8 KiB image whose header names the Game Gear is refused, and so is any 8 KiB image loaded as a Game Gear.
5. An image of at most 48 KiB has no mapper.
6. Otherwise the first 32 KiB are scanned for `LD (nn),A` stores. When stores to another board's registers
   (`$0002`-`$0004`, `$3FFE`, `$6000`, `$A000`) outnumber stores to the Sega registers (`$FFFC`-`$FFFF`), the
   image is refused; else it has the Sega mapper.

The boards:

- **No mapper.** The image at `$0000`, padded with `$FF` up to `$BFFF`. Writes to it change nothing.
- **Sega.** Writes to `$FFFD`, `$FFFE` and `$FFFF` select the 16 KiB bank in `$0000`, `$4000` and `$8000`; the
  first 1 KiB stays on bank 0. Bit 3 of `$FFFC` maps 16 KiB of cartridge RAM at `$8000` (bit 2 picks which of
  two banks); bits 1-0 add `$18`, `$10` or `$08` to later bank writes. Every register write also lands in work
  RAM, so `$DFFC`-`$DFFF` read them back. Fast boot leaves the banks at 0, 1 and 2.
- **Codemasters.** Writes to `$0000`, `$4000` and `$8000` select the bank in those slots; they start at 0, 1 and
  0. Bit 7 of the `$4000` write maps 8 KiB of cartridge RAM at `$A000`-`$BFFF`.
- **Korean.** A write to exactly `$A000` selects the 16 KiB bank of `$8000`-`$BFFF` (the value modulo the bank
  count); `$0000`-`$7FFF` stay on banks 0 and 1, and the banks start at 0, 1 and 2. Any other write below `$C000`
  changes nothing.
- **Korean no-bank.** No register: the image sits at `$0000` in 8 KiB banks 0 to 5, so a 32 KiB image shows its first
  16 KiB again at `$8000`. Writes change nothing.
- **Hi-Com.** A write to `$FFFF` selects the 32 KiB bank of `$0000`-`$7FFF`; the bank starts at 0, and `$8000`-`$BFFF`
  always show the first 16 KiB of the image.

The three boards bank in 8 KiB steps, and a bank number past the end wraps modulo the image. On an image whose
size is not a power of two a register write past the end also records a `mapper_range` fault. Korean no-bank and
Hi-Com are found by their hash only: nothing in the image names them.

The image is padded with `$FF` to whole 16 KiB banks, and a bank number past the end selects the bank number
modulo the bank count. Cartridge RAM starts zeroed at load, is kept by both resets and is part of the saved state.
