# Starting through a BIOS

cogmaster starts a game the way the console's BIOS would have left it, with no BIOS code run. That is the default,
and it is what you want almost always. If you have a Master System BIOS dump of your own, the runner and the
libretro core can run it instead and let it start the cartridge. cogmaster ships no BIOS and fetches none; the
public digests of known dumps live in [ASSET_POLICY.md](../ASSET_POLICY.md) and are not repeated here.

## What you bring

A dump of exactly 8,192 bytes from a Master System, which you own. The Game Gear BIOS (1,024 bytes), the 16,384 byte
store-display image and the larger images with a built-in game are not run in this release; when the runner
recognises one of them it says so, and it still refuses it. The 8 KiB images of the public table are Master System 1
class (they belong to the SMS1 and Japanese machines), so give a real dump with `--machine sms1`; the Master System II
video chip differs, and a BIOS tuned to the first one can draw differently on it.

## The runner

```sh
bin/cogmaster-run --boot bios --bios FILE --frames 600 --hash game.sms
```

`--boot fast` is the default and equals leaving the option out. `--bios FILE` is allowed only with `--boot bios`,
each option once, in any order, with every other option except `--probe` and `--expect`. Frame N of a BIOS run is not
frame N of a fast run, because the BIOS runs first, so an `--input` script written for fast boot misaligns.

| Exit | Meaning here |
|---|---|
| 2 | The command line is wrong: a repeated, empty or unknown value, `--bios` without `--boot bios`, no cartridge. Nothing is read. |
| 77 | The BIOS is refused: no `--bios`, no file at that path, a file that is not 8,192 bytes, or a Game Gear cartridge. |
| 1 | A file could not be read: a `--bios` directory, an unreadable file or a missing cartridge. |

Every message is fixed text; none prints the path or a byte of the file. A BIOS run prints one line on standard
error: `BIOS recognised: NAME` when the file is a known dump, otherwise `BIOS not recognised (sha1 DIGEST); running
it as given`. A dump of the right size that is not in the table, a patched one for example, still boots. Recognition
only names the file: it picks no machine, region or FM sound. [runner.md](runner.md) lists the options.

## RetroArch

Put the file in RetroArch's system folder, under one of three names, and set the core option **Boot (reload
content)** to **BIOS from the system folder**; it takes effect when the content loads.

| File | Used for |
|---|---|
| `bios_J.sms` | Region **Japan NTSC** |
| `bios_E.sms` | TV **PAL** (the **Export PAL** setting of the region option) |
| `bios_U.sms` | Everything else, including **Automatic** on a game with no Japanese mark |

The name is chosen by the resolved region and TV and by nothing else: no other name, no subfolder, no search of the
current folder and no fallback from one file to another. The path is never built from the game's file name or from
an option string. A European user who has only `bios_E.sms` and leaves the TV at the default NTSC gets the fast
boot and one warning; set the region to PAL. A Game Gear game is never started through a BIOS.

Every refusal, an absent folder or file, a file that is not 8,192 bytes or a Game Gear game, gives the ordinary fast
boot and one warning in RetroArch's log that names the file and never the folder; a load never fails because of a
BIOS. A file merely sitting in the folder changes nothing until you choose the option. The core's information file
lists the three files as optional firmware. Each such row is a claim, and `libretro.host.bios` tests it.

## What locks up, and why

A real BIOS checks the cartridge's header (the `TMR SEGA` mark and, on some consoles, the checksum and the
region) before it starts the game. Some Japanese, Korean and wrong-header games that run under fast boot fail that check and sit at the BIOS
screen for good. That is the BIOS doing what it did on the console, not a fault in the emulator, and it is why fast
boot is the default.

A BIOS-boot Reset button does not reach the CPU, and the address `$0066` belongs to the BIOS's own handler until
the BIOS has switched the cartridge on. A soft reset from the library starts the CPU again at address 0 and keeps
RAM, video memory and colour memory.

## Who answers a read

Port `$3E` switches the BIOS (bit 3), the cartridge (bit 6), the card slot (bit 5) and the expansion slot (bit 7); a
set bit switches the part off. The BIOS answers only `$0000` to `$1FFF`, and only while it is on.

| Machine | Cartridge and BIOS both on, `$0000`-`$1FFF` | Only one on | Neither on |
|---|---|---|---|
| Master System (SMS1, any region) | the cartridge wins | the one that is on | nothing drives the bus |
| Master System II (SMS2) | the two ANDed | the one that is on | nothing drives the bus |

The Japanese console is whichever of the two you pick; the region never changes the rule. A real BIOS never leaves
both on: it copies itself to RAM, switches itself off and then the cartridge on. The case is reachable only by a
hostile or test image, and the core handles it within its usual bounds. These rows are models, not measurements:

- **BIOS on alone, `$2000`-`$BFFF`:** nothing drives the bus there. This is a model choice; the reference emulators
  disagree (one mirrors the 8 KiB, one reads zeros) and nothing real depends on it.
- **An empty card or expansion slot:** with a BIOS attached, a read of a slot that is enabled and empty (bit 5 or
  bit 7 clear) returns the same open-bus byte as before but records no fault, because a real BIOS probes the card,
  then the cartridge, then the expansion slot and would otherwise be the first fault of every run. With both bits
  set, or with no BIOS, it is a fault as ever. This is a model row.
- **`$E0` at power-on:** expansion, cartridge and card off, RAM, BIOS and I/O on. It is derived from "BIOS visible,
  cartridge hidden", not measured.
- **`$DFF0` and the 0, 0, 1, 2 paging** of the fast-boot hand-off are conventions with indirect evidence: exact for
  a cartridge up to 48 KiB, an approximation after a checksum run over 64 KiB or more.

## What the tests can show

No dump is in the repository, so the tests run a self-authored BIOS-shaped image of 8,192 bytes. It proves the
mechanism: the BIOS window, the `$3E` walk, the hand-off to the cartridge through the mapper, and that the fast boot
and the BIOS boot take their constants from one place in the core. It cannot show what Sega's BIOS leaves behind: the
logo tiles, the video registers, work RAM above `$C000`, the Z80 registers, the sound chip's attenuation or the
beam's phase. Treat a game that depends on any of those as untested under a BIOS.

The checks are named `bios.*` for the core, `runner.bios.*` for the runner, `host.bios` for the file reader and
the digest table, `libretro.host.bios` for the RetroArch option and `policy.firmware_*` for the scans that keep dumps
out of the tree and the release archives. A BIOS run of the self-authored image is also compared across the eight
platform lanes with the other device rows.
