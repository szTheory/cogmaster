---
id: SEED-011
status: dormant
planted: 2026-11-11
planted_during: phase 10, plan 11
trigger_when: the owner holds a Game Gear BIOS dump or asks for a built-in-game BIOS, a Mark III profile or a BIOS boot without a cartridge, or the real frontend drifts from the in-process fake
scope: medium
---

# SEED-011: BIOS boot follow-ups left out of the first phase

## Why This Matters

The first BIOS phase boots one 8 KiB Master System image beside a cartridge and proves it with a self-authored image.
Each item below was left out on purpose: either no source in hand settles it, or it needs a machine feature the core
does not have, or the real check is a hosted or local run that would add ceremony for no recurring catch. None of them
changes a hash today. Each says what triggers it and which rule keeps it out.

## The Game Gear 1 KiB BIOS

Trigger: the owner has a Game Gear BIOS dump and wants it to run. Kept out by: a self-authored 1 KiB image can only
prove our own model, and no held source says in which mode a real Majesco BIOS writes `$3E`.

- The native-mode `$3E` latch exists only when a BIOS is attached. Fast boot and every existing test keep `$A8` and
  the unmapped-write fault, so no existing hash or fault count moves.
- The `$3E` bit 6 fix goes behind its own test. Today the core takes the cartridge off on a Game Gear when a game sets
  bit 6 in Master System mode; hardware, MAME and ares ignore bit 6 there. The check that nothing else moved is
  `git diff --quiet origin/main -- tests/io` and the library ledger. If anything moves, the doc is amended, not the test.
- The window is 1,024 bytes at `$0000-$03FF`, switched by bit 3 of `$3E`.
- `bios.gg` and its `.info` row land together with the code that reads it, because every `.info` claim maps to a test.

## Built-in-game BIOSes

Trigger: the owner asks for a console whose BIOS carries a game. Kept out by: it needs paging, which the 8 KiB image
does not.

- The 16 KiB Store Display image and the 128 KiB and 256 KiB images.
- Paging through `$FFFD` to `$FFFF` while the BIOS is enabled; three bank entries, as ares does.
- The runner already recognises the dumps and says "recognised, not supported yet".

## A BIOS boot with an empty slot or the test card

Trigger: a user wants the built-in maze or the BIOS logo with no cartridge. Kept out by: the core needs a CPU run with
no cartridge present; load without content is `ERR_NO_CONTENT` today.

## A region hint from a recognised Japanese dump

Trigger: a user complains that a Japanese dump boots with an export region. Kept out by: `--region japan` and the
libretro region option stay the user's call; the file name already picks the region in libretro.

## A Mark III profile

Trigger: a request for the Japanese console without the FM unit or with its own port rules. Kept out by: the machine
profile table has no row for it; the BIOS check is the Master System one.

## A hosted RetroArch BIOS session

Trigger: the real frontend drifts from the in-process fake (it stops handing the system folder, or ignores the options
file). Kept out by: `tests/retroarch` is near its 300-line budget, and the in-process test already hashes the frame.
The step is YAML only and free of counted code: a system folder under the job's temporary directory holding the owned
image, and an options file selecting the BIOS boot.

## CI `hygiene.sh --history`

Trigger: a BIOS added and deleted inside one pull request and pushed with the hooks bypassed reaches the remote.
Measured here: 25 commits scan in 4.0 s on macOS. A CI step needs the hygiene job's timeout re-measured and
`fetch-depth: 0`. Kept out by: a measurement that has not been repeated on a hosted lane, and the release scan already
runs over every archive.

## A publish-job post-download scan

Trigger: an archive can reach a release without passing the `ci` release scan. Kept out by: the archives are built and
scanned in `ci`; a second scan after the download repeats the same check on the same bytes.

## Phase 10 review leftovers

Found by the phase review and kept out of the fix round. Each is a scenario, not a requirement.

- Hex dump layouts: a firmware pasted as `xxd` or `hexdump -C` output (offset and ASCII columns on every 16-byte line)
  breaks the 32-byte window, so the scan reports nothing. The layouts are listed as not covered in `docs/testing.md`.
- Byte-run rule: a 512-byte array in a file with CRLF line endings, or with a trailing `//` comment on each line, does
  not trip the retyped-bytes rule in `scripts/hygiene.sh`.
- Hook temp names: `scripts/hygiene.sh` names staged and history blobs by mapping `/` and `:` to `_`, so `a/b.rom` and
  `a_b.rom` share a temp file and the first is overwritten unscanned; a hit names the temp path, not the repository path.
- File names holding a newline: the window scan reads the file list line by line, so a tracked file whose name contains
  a newline is split into two non-files and skipped.
- Core: a BIOS-attached silent empty-slot read is keyed on a BIOS being attached, not enabled, so a game's first-fault
  record differs between a BIOS boot and a fast boot.
- Core: a state binds to a BIOS by CRC-32 alone, so a crafted 8 KiB image with the same CRC loads against another.
- Core: a tag size between the old and new config struct size selects fast boot with no signal.
- Core: the BIOS size exists only as a literal in the public header comment, so hosts hard-code 8192 in three places.
- Core: `bios.size` is only ever 0 or 8192 and reads as a presence flag, so it could be renamed `attached`.
- Core: an automatic machine on a Japanese cartridge takes the SMS 2 AND arbitration, where the first-generation board
  lets the cartridge win.
- Core: every writer of `$3E` still pairs a write with `bus_decode` by convention; a single setter was not added because
  a state load replaces the whole register block and cannot go through one.
- Host: the libretro module links the BIOS digest table and SHA-1 it never calls, because `host/bios.c` is one unit.
- Host: the exit table's refusal text is split across two literals, and `COGMASTER_BIOS_SIZE` in the host duplicates the
  core's accepted size with no cross-check.
