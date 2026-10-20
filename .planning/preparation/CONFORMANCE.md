---
title: "Conformance: vectors, fixtures, oracles, diagnostics and the compatibility ledger"
summary: "Which SMS and Game Gear test material cogmaster may use under which licence and disposition, the self-authored WLA-DX fixtures that are the primary v1.0 evidence, how released emulators and real hardware act as locators, the RAM-mailbox protocol, the ledger and its ratchet, and what each CI tier runs and costs."
read_when: "Admitting a test ROM, vector set or fixture; writing a fixture or diagnostic ROM; building the runner's mailbox or predicate code; adding a ledger row, an oracle job or a CI lane."
updated: 2026-10-20
---

# Conformance, oracles and the compatibility ledger

Implemented phase-3 evidence includes native/SMS-mode GG and SMS1/SMS2 playing hashes, input variants,
owned frontend screenshots, enabled-video state rejection, allocation and hostile-run bounds. Hashes are
compared across six native platforms and be32; wasm32 reports equality separately. Performance now measures
the scripted playing fixture but remains report-only. Formal code review precedes phase verification.
H-counter sub-dot origin, tick-600 latches and GG LCD freeze remain qualified sourced models. Three
unclassified probe assumptions remain flagged in the phase research/source coverage; binary agreement is
not hardware measurement. Moldorian persistence geometry remains unsupported.

## Where to look
| Question | Section |
|---|---|
| What test ROMs and vector sets exist, under which licence, with which disposition? | 1 |
| Which fixtures do we write ourselves, what does each prove, and how is WLA-DX pinned? | 2 |
| Which released emulators are locators, what can each emit headless, who referees timing? | 3 |
| Mailbox protocol, on-screen format, the owner's hardware-run protocol, the measurement list | 4 |
| Ledger columns, statuses, predicates, ratchet, local library preset | 5 |
| What runs per PR, nightly and locally, and what it costs | 6 |
| May this third-party fixture enter the repository? | 7 |
| Open questions and the phase that settles each | 8 |

## 1. Test-ROM and vector inventory
Disposition key: **commit** (every byte in the file is explicitly permissive and listed in `tests/roms/manifest.txt`), **build-ci** (built in CI from a pinned source, only digests committed in `tests/built/expected.txt`), **fetch** (scripted fetch at a pin, SHA-256 checked, never committed, never uploaded as an artifact), **local** (owner's machine only, `library` preset), **binary-oracle** (the ROM's result is not decoded from its source but compared with what a released emulator shows for the same ROM) [CF.16][CF.14].

| Suite | Licence of the bytes → disposition | Size, pin | Proves / does not prove | Reports via | Tier |
|---|---|---|---|---|---|
| SingleStepTests `z80/v1` [CF.01] | MIT → **commit** the first 100 tests of every one of the 1,604 files in the compact form below (160,400 tests); **fetch** the full 1,604,000 nightly at `ebe1875d48f3` | repository 283,823 KB by the GitHub API (tested) [CF.24]; 1,604 JSON files × 1,000 cases | every documented and undocumented instruction: per-T-state bus, final registers incl. MEMPTR (`wz`) and the Q latch, RAM, I/O ports; NMOS model (SCF/CCF `(F\|A)` with Q, `OUT (C),0` writes 0) which matches the NEC and Zilog NMOS parts in SMS1/SMS2 [CF.22 SP.006, SP.007, SP.038]; not interrupts (removed, SP.003), not WAIT states, not the one-chip GG's possibly CMOS part (SP.040) | the comparison in 1.1 | PR sample, nightly full |
| ZEXALL-SMS v0.21 `zexall.sms`, `zexdoc.sms` [CF.02] | GPL-2.0 → **fetch** the release asset `ZEXALL-SMS-0.21.zip` (tag `v0.21`, exists (tested) [CF.24]); digest recorded at first fetch; its source is never opened (PROVENANCE §3) | pin `8bfcd2ca594f` | CRC per instruction group against a real-system CRC list, plus the v0.20 SCF/CCF pre-test pattern [CF.22 SP.020, SP.041]; undocumented flags are masked by default, so `zexall` is run as well as `zexdoc` | screen text decoded from the name table (1.2) | nightly |
| raxoft z80test v1.2a [CF.03] | MIT → **later**: needs a shim for the Spectrum print routine and `IN $FE` (SP.134); admitted when the shim is written (v4.0 accuracy milestone), then commit | `c490c0ca94f1` | flags, MEMPTR, CCF variants against a Zilog 48K | CRC on screen | nightly, v4.0 |
| FluBBa SMS VDP Test v1.31 [CF.04] | **no licence** → **local** and **nightly fetch** only; **binary-oracle** | `490c00b38c83` | VDP data path, H/V counters, HC latch, IRQ positions, flags, collision and overflow (SP.122); MAME passes all SMS items (VDP.19) | name-table words via `peek`, compared with the name table MAME `sg1000m3` shows at the same frame | nightly, local |
| sverx SMS Test Suite v0.38 and GG Test Suite [CF.05] | **no licence** → **local**, **nightly fetch**; **binary-oracle** | `985bc04fef6f`, `1a29804d1fc8` | VDP type (315-5124 vs 5246), 50/60 Hz, GG and twin-ASIC detection, PSG volume clipping, pads, Pause/Reset (SP.123) | menu driven by a movie; detection results read from the name table; test cards by frame hash vs oracle | nightly, local |
| JoppyFurr SN76489 TestRom v1.00 (`.sms`, `.gg`, `.sg`) and YM2413 TestRom [CF.06] | MIT game code, but built with devkitSMS: the image carries the GPL-2+exception `crt0` and possibly SMSlib's ZX7/aPLib routines (SP.175, SP.176) → **build-ci** at the pin until the crt0 decision of §7 is made; then **commit** under the manifest with notice if admitted | `7d2068dfc171`, `571018fe8bfb` | PSG tone, noise, volume, GG stereo bits; FM registers (SMS-AUDIO 7) | movie-driven; transition and PCM hashes; the oracle for the SMS-AUDIO 9 hardware questions | PR (built), nightly |
| MIT homebrew games: dragon-blaster `5b48f0570aeb`, food-fighter `48fe2ea96795`, river-strike `acfcc478c14e`; sms-rogue (Unlicense) `329d788feb01`; Snepzhen-Solitaire (MIT, SRAM) `94d9d454541a` [CF.07] | same crt0 question → **fetch-at-pin** of the release zip where one exists (dragon-blaster v0.11, food-fighter v0.4, river-strike v0.10), **build-ci-at-pin** with SDCC 4.6.0 + devkitSMS on the Linux leg where none does (sms-rogue, Snepzhen); never committed until the exception text is quoted and judged (§7) | see pins | real-game paths: sprites, scrolling, PSG music, text, SRAM save; `boots@F`, `frame@F=H`, `movie:` predicates | frame, RAM, audio hashes | nightly |
| GG candidates [CF.07] | SN76489 TestRom `.gg` (MIT, crt0 question) → build-ci; liamhays/tunnel `79c0c21dffbe` (GPL-3) → fetch; GG Button Test `8c65ecd39ed6` (no licence) → local | | GG window, stereo, Start; permissively licensed GG game source is scarce (SP.154), so `boot_to_play.gg` (§2) is the primary GG evidence | hashes | nightly, local |
| Owner's cartridge dumps | commercial → **local** `library` preset only; identified by headerless SHA-1 against the CC0 lists [CF.10] | 653 + 823 + 214 list entries | the ledger's game rows (§5) | predicates | local |

**1.1 Compact vector form and the per-cycle comparison.** `tests/vectors/z80/sst-sample.bin`, one file, little-endian, generated by the C converter `cogmaster-sst2bin` (`tests/z80/sst2bin.c`) from the JSON at the pin and regenerated nightly for the full set in the build directory. It stores the **access form**: per test the initial state, a changed-field mask with the changed final values, the RAM and port lists, the T-state count and one record per bus cycle (kind, start T-state when not adjacent, address, data, refresh address for an M1), which `tests/z80/sst_stream.c` expands to every T-state through one set of templates shared by converter and harness; the layout is documented in `tests/z80/sst_stream.h`, and the committed sample measures 13,017,674 B, about 81 B per test (estimated 86 B; the per-T-state form measured 126.4 B) [CF.25]. The runner loads the initial state, executes one instruction with the Z80's bus-sample hook on, then compares: the T-state count; every sample's address, pins and, when not `null`, data; every register including `wz` (MEMPTR, SP.021–SP.022), `q` (the Q latch, SP.002, SP.010) and `ei`; `p` (1 after `LD A,I` and `LD A,R`, 0 after every other instruction, compared exactly); every RAM and port entry. No permitted-difference list exists at the start; a documented upstream inaccuracy (SP.009) becomes a dated line in `tests/vectors/z80/allow.txt` naming the upstream issue, never a silent skip (XFER-013 [CF.17]).
- **Recommendation:** first 100 tests per file in the compact form; full set nightly. **Strongest alternative:** every tenth test (16× the download per PR). **Would change if:** the sample measures above 24 MB (then the first 50 per file) or the nightly finds a failure the sample missed twice in a milestone.

**1.1a What the vectors do not cover, and what does** (SP.003, SP.005, SP.009 [CF.22]):

| Gap in `z80/v1` | Covered by | Expected from |
|---|---|---|
| INT and NMI acceptance (cost 13/11 T, IFF handling, bus byte `$FF`, HALT exit) | `tests/z80/int_tests.c`: scripted /INT and /NMI lines around EI, DI, RETN, LD A,I, HALT and block instructions | SP.023–SP.028, SP.035; the `ports` NMI counter on hardware |
| EI delay, run of EIs, DD/FD prefix runs blocking acceptance | same unit file | SP.024 |
| R register across prefixes, block ops, HALT | unit file plus a `vdp_lines` record that reads R twice in a timed loop | SP.030 |
| WAIT states, SMS1 open bus | none in v1 (the SMS inserts no WAIT); `bus.open_bus.sms1_vs_sms2` fixture record | ARCHITECTURE 13 |
| Per-cycle order of `LD (IX+d),n`, `EX (SP),HL`, RLD/RRD before the mid-2025 fix | current pin is after the fix; a failure here is triaged against the upstream issue before any allow line | SP.009 |

**1.2 Reading ZEXALL's result.** The port prints with a font whose tile index equals the ASCII code (verified once in phase 2 from the built ROM's VRAM: the runner maps tile → glyph by hashing each 8×8 pattern of the fetched ROM's font against our own ASCII table and stores the map in `tests/built/expected.txt`); `--mailbox` is not available, so the runner decodes name-table rows with `peek` and matches `OK` / `ERROR` per group with a tolerant regex. A correct core reproduces real-system CRCs, not necessarily the program's own table (SP.041): the expected list is MAME `sg1000m3`'s output at the pin, recorded as a binary-oracle fact, and a disagreement with the ROM's embedded CRC is reported, not failed.

## 2. Self-authored fixtures (primary v1.0 evidence)
Pure WLA-DX assembly, MIT, committed as source under `tests/roms/src/`, assembled in CI by WLA-DX v10.7 built from the tag tarball pinned by SHA-256 `38296a96bc20be873d17e0e88be0c5b20a15ef2ec4da5279600e56af30ad925a` (1,402,817 bytes; tag commit `91c52b1f4ef3`; API tarball URL as fallback identity) [CF.09][CF.22 SP.178]. WLA-DX injects no bytes of its own (fill only, no timestamps, SP.170), so every byte of the image is ours [CF.16 AP §2]. Sibling lessons: unclear-licence homebrew makes a poor gate and a self-authored fixture is cheaper and safer (XFER-025); choose the fixture before the milestone starts (XFER-050) [CF.17].

**Byte identity across macOS, Linux and Windows.** Each of the six legs builds WLA-DX with CMake from the checked tarball (`cmake/WlaDx.cmake`; wrong digest fails configure, `fixtures.wla_pin.self_test`), assembles every fixture with `.EMPTYFILL $FF` stated explicitly, and writes each image's SHA-256 into `hashes.txt`; the `hash-equality` job requires the eight `hashes.txt` files to agree (ENGINEERING 4 [CF.18]). Rules that keep it true: `.gitattributes` forces LF on `*.asm`/`*.inc`; no float directives (`.DBCOS` and kin); no `.INCBIN` of anything outside `tests/roms/src/`; `.sym` and listing files are never hashed. **Phase 1 experiment:** before any fixture exists, assemble `hello.sms` (header, checksum, one `jp $`) on all six legs, compare digests, and check `.COMPUTESMSCHECKSUM` against the runner's own `--probe checksum_ok=1`; a mismatch on any leg moves that leg's fixtures to a Linux-built, digest-checked copy (fetched from the Linux job through the Actions cache, never an artifact) until fixed.

| Fixture | What it proves | Why the oracle cannot be fooled |
|---|---|---|
| `boot_to_play.sms` / `.gg` | title screen (name table, sprites, backdrop); Start/Button 1 increments a visible counter, adds 8 to reg 8 (hscroll) and changes PSG channel 0's period; the counter counts 0→1 edges only (XFER-016 "a control that changes play") | three independent measures differ between movie `none` and movie `press@60`: frame hash at frame 300, mailbox record `presses`, audio transition hash; a core that draws nothing, ignores input or reports a held button as many presses fails at least one; expected exact values per movie are pinned on every leg |
| `vdp_lines.sms` | reg $0A line interrupts every 24 lines; the handler writes a per-band reg 8 so bands scroll by different amounts; reads H and V counters at handler entry into the mailbox (expected HC $06 frame / $07 line, SP.048); samples the V counter wrap ($DA→$D5 NTSC 192, SP.052); the frame handler resets; a variant writes reg 9 at line 100 (vscroll latch, ARCHITECTURE 13 [CF.19]) | the picture is the band pattern (frame hash) and the counters are numbers judged by `expected.csv`; a per-frame vs per-line reload error shifts a band, a wrong interrupt tick changes HC |
| `mapper_sega.sms` | a 512 KB image whose 32 banks each start with {bank, ~bank, 14 bytes of bank-specific code}; pages every bank through slots 1 and 2, sums signatures, checks slot 0's first 1 KB fixed, bank wrap (`bank % pages`), `$FFFC` b3/b2 on-cart RAM window and RAM bank, the `$DFFC` mirror (SP.094), b1–0 shift | the sum over 32 distinct signatures and the RAM pattern are written to the mailbox and the sum is drawn; a mapper that mirrors or ignores a register produces a different sum |
| `mapper_codemasters.sms` | Codemasters header at `$7FE0`, no `TMR SEGA`; registers at `$0000/$4000/$8000`, reset 0,1,0, slot 0 pageable (vectors change: checked with interrupts off), Ernie Els 8 KB RAM at `$A000` via b7 (CART 2.2 [CF.20]) | as above; the probe must classify it `codemasters` (`--probe mapper_guess`) |
| `battery.sms` | at boot reads magic + counter from on-cart RAM, branches visibly (counter 0 vs n+1 drawn as a glyph), writes the next value with `$FFFC` b3 on; `save_generation` increments | the fresh-process continuation (HOSTS 4 continuation protocol, Playstead SL-01 [CF.21]): run 1 with no `.srm` shows 1, run 2 in a new process with run 1's save shows 2; the two frame hashes must differ and the second `.srm` must equal the first plus one |
| `psg_sweep.sms` / `.gg` | a register script table: latch/data protocol (Alex Kidd case), tone sweep, period 0/1, noise 16/32/64/tone-2, volume steps, pops (SP.086–SP.088); `.gg` adds `$06` masks | the unit test derives the expected transition stream from the same script table without the Z80 (two independent paths to one hash); a frame counter record per segment moves if a write is dropped |
| `ports.sms` / `.gg` | region by `$3F` TH write and `$DD` read, TV type by lines between frame interrupts (262/313), `$3E` and `$DE/$DF` reads, Pause NMI counter at `$0066` (movie: presses at 100 and 200 → 2; held → 1), H-latch value after TH 1→0 and 0→1; `.gg`: port `$00` Start/NJAP/NNTS bits (SP.111, SP.112) | every value is a mailbox record judged per profile (export NTSC, Japan, PAL, GG); the picture shows them in hex |
| GG variants | `boot_to_play.gg` places the counter and a border ruler inside the 160×144 window (rows 23/24 and 167/168 marked); `psg_sweep.gg` stereo; `ports.gg` Start | the SMS-mode crop and the GG window hash differently by construction |

**Design rules.** (1) A counter only a new input edge changes; a fixture never reads an input level into a result. (2) No self-passing verdicts: the ROM measures and draws, `expected.csv` judges. (3) ≤ 32 KB of authored content; images that are large only to exercise paging (`mapper_*`) are assembler fill plus 16-byte bank stamps, built in CI, never committed as bytes and excluded from release archives. (4) A GG image is the SMS source assembled with a GG header (region nibble 6) where behaviour allows; otherwise a `.gg` source shares every include. (5) Each fixture initialises all state it depends on (no reliance on power-on values except where that is the test), waits for V counter `$B0` before touching the VDP (SP.115), keeps the stack below `$DFF0`, and never depends on the BIOS. (6) Own 8×8 glyphs only (hex digits, `OK`, `ER`); no third-party font (XFER-019). (7) Every fixture has a frame cap after which the runner reports `hang`.
**Fixture matrix** (predicates as in §5; frames chosen so the title screen is settled and one movie event has happened):

| Fixture | Machine | Predicates pinned | Movies | Phase |
|---|---|---|---|---|
| `boot_to_play` | sms, gg | `frame@300`, `mailbox@300`, `audio@300`, state round trip at 150 | `none`, `press@60`, `press@60+120+180` (expects 3) | P3 (P4 audio) |
| `vdp_lines` | sms (gg variant for the window rows) | `frame@120`, `mailbox@120` (HC/VC records, wrap points) | `none` | P3 |
| `mapper_sega` | sms | `mailbox@60` (sum, RAM pattern, mirror), `frame@60` | `none` | P2 (RAM predicate first), P3 (picture) |
| `mapper_codemasters` | sms | `mailbox@60`, `frame@60`, `--probe mapper_guess=codemasters` | `none` | P2 |
| `ports` | sms × {export-NTSC, Japan, PAL}, gg | `mailbox@200` per profile | `none`, `pause@100+200`, `pause-held@100` | P2 (`none`; the Pause movies and the H latch wait for `--input`) |
| `psg_sweep` | sms, gg | `audio@600` (transition), PCM digest per rate, `mailbox@600` | `none` | P4 |
| `battery` | sms | run 1 `frame@120`, `.srm` bytes; run 2 `frame@120` differs | `none` | v1.0 phase 3 (GAME-05) |
| `hello` | sms | SHA-256 of the image only (toolchain identity) | none | P1 |

- **Recommendation:** the eight fixtures above, all landing in v1.0: phase 2 (`mapper_*`, `ports`, `hello`), phase 3 (`boot_to_play`, `vdp_lines`, `battery`, which GAME-05 requires) and phase 4 (`psg_sweep`). **Strongest alternative:** one large "test suite" ROM with a menu. **Would change if:** CI assembly time per leg exceeds 30 s (then prebuilt digests from one leg).

## 3. Oracles and locators
Released binaries only; no copyleft, non-commercial or closed source is opened (AGENTS rule 1, PROVENANCE §3). Agreement is a locator written to the ledger, never a gate.

| Oracle | Licence, binary | BIOS | Headless interface and what it can emit | Tier |
|---|---|---|---|---|
| MAME at the pin [CF.10] | GPL-2 as a whole (binary only); its SMS files are BSD-3 and may be read | `sms`, `sms1`, `smsj`, `smspal`, `sms1pal`, `smskr`, `sms*br` all declare a BIOS in `user1` with no "none" option, so they refuse to start without a dump (tested: `sms.cpp` L1015–1155); `sg1000m3` (Mark III, 315-5124, Japan) has no BIOS region and `gamegear` offers BIOS "none" (L1082, L1162) | `-video none -sound none -nothrottle -seconds_to_run N -autoboot_script x.lua`; Lua frame callback, program-space reads of `$C000–$DFFF` and cart RAM, PC, frame number, exit snapshot PNG, `-wavwrite` (method tested on another driver in the sibling [CF.14]; SMS drivers untested here); VDP register and PSG state not exposed (unverified) | nightly: `sg1000m3` and `gamegear`; local with the owner's BIOS: `sms`, `smspal`, `smsj` |
| Genesis Plus GX and Gearsystem as libretro cores [CF.12] | non-commercial / GPL-3 → binaries from the libretro buildbot, pinned by date and SHA-256 | none needed | loaded by our own `libretro.host` test harness (which exists to prove our core loads): `retro_run` per frame, video callback buffer, `retro_get_memory_data(SYSTEM_RAM)`, audio callback; gives RAM hash, quantised frame and PCM per frame with no display (derived, untested) | nightly |
| ares [CF.11] | ISC; source may be read; released binaries for three OSes | optional | no frame cap or scripting in the release CLI (unverified beyond the README); the agent runs it and reads screenshots (AGENTS rule 6) | local |
| Emulicious | closed, Java | optional | remote debugger protocol only (unverified); its value is Calindro's hardware comparisons (SP.199) | local, by hand |
| MiSTer SMS core [CF.13] | no LICENSE (facts only); the `.rbf` is a released binary | none | a simulation harness would need the RTL in the workspace, which rule 1 forbids; the core runs only on an owner's MiSTer if one exists (§8) with the §4 ROMs; its constants (x = 485–488, VDPTEST-tuned) are cited, not copied | v4.0 referee |
| Real hardware | the owner's consoles | the console's own | the §4 diagnostic ROMs; photo or capture card | arbitrates |

**Per-frame comparison** (the runner writes one record per frame at the frame-interrupt tick; alignment search k ∈ [−2, 2] frames because boot paths differ):

| Field | cogmaster | MAME Lua | libretro host | Hardware | Expected noise |
|---|---|---|---|---|---|
| RAM SHA-256 over `$C000–$DFFF` (+ cart RAM) | yes | `read_u8` loop, hashed outside Lua | `SYSTEM_RAM` span | mailbox only | fast boot vs BIOS: `$C000` hand-off byte, SP, `$DFFC–$DFFF` mirror (SP.107, SP.109); exclude `$C000–$C00F` and `$DFF0–$DFFF` in a second, "core" hash |
| Frame hash | RGB444 crop (ARCHITECTURE 5) | PNG via level tables | RGB565/XRGB8888 | photo | quantise both sides to the 2-bit (SMS) or 4-bit (GG) channel index before hashing; static screens only; MAME's SMS1 blue level differs (SP.073) |
| VDP registers R0–R10, status, line counter | yes | unverified | none | none | informational |
| PSG state (tones, volumes, LFSR, latch) | yes | none | none | none | PCM is a locator for a wrong octave, missing channel or wrong LFSR, never a digest (SMS-AUDIO 7) |
| Tick / frame number | yes | frame number | frame count | none | MAME frame cadence vs our 59,736-cycle frame |

**When a locator disagrees.** (1) The nightly writes `oracle=<name>:agree@F` or `diverge@F` into the ledger row's evidence and opens or updates the one rolling `nightly` issue (ENGINEERING 3); nothing turns red. (2) The agent reproduces the first divergent frame with `--trace first-fault` and the per-line counters (ARCHITECTURE 8) and names the device. (3) It then decides whose error it is from documents, never from the oracle's source: a T2 document or a hardware number outranks the oracle; two oracles agreeing against us with no document is a hypothesis for the hardware list in §4. (4) A fix lands with a fixture record or a unit test, not with a tuned constant. (5) A divergence that persists with a cited document on our side becomes a dated note in the ledger row, not an allow line.

- **Recommendation:** MAME `sg1000m3` + `gamegear` nightly on Linux x64 as routine locators; GPGX through the libretro host as the second opinion; owner-BIOS MAME drivers local. **Strongest alternative:** RetroArch `--max-frames-ss` screenshots of GPGX under Xvfb (needs a real video driver; untested). **Would change if:** the libretro-host approach fails to load a buildbot core on the runner twice, or MAME `sg1000m3` disagrees with VDPTEST where `sms1pal` passes (then the local BIOS lane becomes the locator).

## 4. Own diagnostic ROMs and the RAM mailbox
The fixtures of §2 double as diagnostics: they measure and never judge. **Mailbox** at `$C100–$C1FF` (work RAM, away from the `$DFF0–$DFFF` stack convention and the mapper mirror):

| Address | Content |
|---|---|
| `$C100–$C103` | magic `CMDG` |
| `$C104` | protocol version 1 |
| `$C105` | status: `$80` running, `$81` waiting for input, `$00` done, `$FF` fault (the ROM detected an impossible state) |
| `$C106` / `$C107` | record count / ROM id |
| `$C108–$C109` | CRC-16 (CCITT, poly `$1021`, init `$FFFF`) over `$C110–$C1FF` |
| `$C110 + 4n` | record n: test id u8, flags u8 (bit 0 valid, bit 1 timed out, bit 2 profile-dependent), value u16 LE; up to 60 records |

The runner's `mailbox@F` predicate peeks at frame F (or at status `$00`, or at the frame cap → `hang`), checks magic and CRC, prints `id=value` lines, and judges them against `tests/diag/expected.csv` (`rom, test, profile, value or range, source hw|doc|oracle|derived, citation`). **On screen:** one row per record, `NN VVVV`, in our 16 hex glyphs, drawn inside the GG window (columns 6–25, rows 3–20) so one image reads on both machines; the CRC-16 in double-height digits so a transcription can be checked. Hardware runs use the same bytes.
`tests/diag/expected.csv`, three example rows (a range means both ends inclusive; `profile` `*` means every profile):
```
rom,test,profile,expected,source,citation
vdp_lines,hc_at_frame_int,*,06,hw,SP.048
vdp_lines,vc_wrap_from,sms-ntsc,DA,hw,SP.052
ports,region_echo,sms-export,1,doc,SP.111
```
A row with source `oracle` names the oracle and pin in `citation`; a row with source `derived` names the preparation section. The runner exits 1 on any record outside its row and prints every record either way; a missing row is an `infrastructure_error`, never a pass (XFER-013).
- **Recommendation:** measurements in RAM and on screen, verdicts in `expected.csv`. **Strongest alternative:** in-ROM pass/fail codes. **Would change if:** every expected value is settled by hardware and in-ROM verdicts lose nothing.

**Owner hardware-run protocol** (asked for only with a reason, AGENTS rule 6). (1) The agent prepares `tests/hw/<date>/` with the ROM set and a `session.csv` template; the owner copies the images to a flash cartridge. (2) Record once per session: console (SMS1, SMS2, Mark III, GG, model/board if visible), region, TV standard, VDP type as `ports.sms` or sverx's suite reports it, cartridge model and firmware, video path. (3) Cold-boot each ROM twice; photograph the screen (or capture); for audio items record line-out or headphone output with any recorder. (4) Type the values into `session.csv` (`console, rom, record, value`); `cogmaster-run --mailbox-check session.csv` recomputes the CRC-16 and refuses a row set whose CRC differs from the photographed one. (5) Commit the CSV only; photos and recordings stay local (EXIF, location). (6) Flash carts may add bus delay or alter mapper power-on state, so every ROM initialises what it does not measure.

**Measurement list** (each row names the ROM record and the question it settles):

| Question (source) | ROM, record | Settled by |
|---|---|---|
| vscroll latch per frame or per line (SMS-VDP 10, ARCHITECTURE 13) | `vdp_lines` reg-9-at-line-100 variant: does the lower half move within the frame? (photo) | VDP timing phase, hardware if available |
| SMS1 zoom slot rule (SMS-VDP 10) | `vdp_lines` SMS1 variant: two zoomed sprites 15 px apart, collision flag record | SMS1 profile phase |
| Frame/line interrupt ticks 607/608 vs ≈ 654 (SMS-SYSTEM 9, ARCHITECTURE 13) | `vdp_lines` HC-at-entry records | VDP timing phase |
| Open-bus byte on SMS1: which instruction byte (SMS-SYSTEM 9) | `ports` `IN A,($3E)` after `LD A,$12` / `LD (nn),A` forms, per profile | bus phase |
| Access-slot corruption (ARCHITECTURE 13) | `vdp_lines` fast-OUTI segment with display on: corrupted-tile count (photo) | later seed |
| FM/PSG mix ratio (SMS-AUDIO 9) | YM2413 TestRom alternating a PSG and an FM tone; RMS ratio of the recording | FM phase, owner capture |
| GG port `$06` in SMS mode (SMS-AUDIO 9, ARCHITECTURE 13) | `psg_sweep.gg` assembled with an SMS header, run on a GG: headphone panning audible? | gamegear phase |
| H-counter latch edge TH 1→0 vs 0→1 (CART 8) | `ports` two latch records | VDP phase |
| Sports Pad timeout 143 µs vs 60 µs (CART 8) | `ports` v2.0 variant, only if a Sports Pad exists | peripheral phase |
| Pause NMI line (SP.047), V counter at `$0066` entry | `ports` NMI record | VDP timing phase |
| Mapper power-on values (SP.093) | `mapper_sega` reads `$DFFC–$DFFF` before any write | boot phase |
| SMS2 top-level PSG clipping, period 0 on SMS1/Mark III (SMS-AUDIO 9) | SN76489 TestRom constant-tone mode, recording | psg phase |
| GG window rows 24–167 (SP.072) | `boot_to_play.gg` ruler marks (photo) | gamegear phase |

## 5. Compatibility ledger and ratchet
`tests/compat/ledger.csv` uses nine logical quoted-CSV columns. Commas and doubled embedded quotes
are legal in quoted fields; semicolons stay data. Embedded record newlines and empty records reject.
The bounded shared parser and current contracts are implemented in `scripts/compat-common.sh` and
documented in `docs/conformance.md`; generated public counts are in `docs/compatibility.md`.
Local enumeration is independent of public matching: `scripts/library.sh` runs every supported regular
candidate in one flat directory, including metadata misses. Symlinks, directories and other extensions
are excluded. Only admitted fixture hashes get fixture expectations; unknown observations remain local.
`library.synthetic` proves the real preset using owned bytes. Private `library.local` is absent from CI.
`scripts/compat-update.sh` accepts only pre-seeded public CC0 cases with unchanged predicates/configuration
and expectations, takes titles from the immutable public catalog and replaces local evidence with the
current core revision. New golden values, private identities and fixture changes are discarded. See
`docs/conformance.md` for the fixed bounds and explicit update command; no library directory is uploaded.

| Column | Meaning |
|---|---|
| `id` | headerless SHA-1 (the key of MAME's CC0 lists); for own fixtures `fixture/<name>`; for built third-party ROMs `built/<name>` |
| `title` | the CC0 list's description verbatim (CC0 permits it); fixture names otherwise; a title the lists lack is `unlisted` and its id is never written (§5 last paragraph) |
| `machine` | `sms`, `gg`, `sg` |
| `region` | `ntsc-u`, `ntsc-j`, `pal`, `any` |
| `mapper` | `none`, `sega`, `codemasters`, `korean`, `zemina`, `janggun`, `eeprom`, … (CART 2.2 names) |
| `predicate` | `boots@F` (strict smoke: sampled display enabled, at least two distinct native crops, no first fault; HALT is healthy), `frame@F=H`, `ram@F:$ADDR=VV`, `mailbox@F`; fixed `cfg:` prefix carries video/devices/script/content SHA-256. Audio awaits phase 4. |
| `status` | `pass`, `fail`, `unknown` (never evaluated; seed rows), `unsupported` (names the missing board or device; exit 77), `skipped` (deliberate, with a reason; never in CI tiers), `infrastructure_error` (harness or oracle broke; CI red, neither pass nor fail; XFER-049) |
| `evidence` | 16-hex hash prefix, mailbox CRC, or run URL, plus the core commit (12 hex) |
| `date` | ISO date of the last evaluation |

The live ledger has accepted independent own-fixture expectations and unknown CC0 game seeds.
No current output becomes its own golden value; no metadata membership establishes support.
`tests/compat/allow.txt` lines read `YYYY-MM-DD <id> <predicate> <reason> behaviour-rev=<n> expires=YYYY-MM-DD`; `policy.allow_expiry` fails on an expired line.

Tier is not a column: admitted own-fixture rows run in CI after their `tests/built/expected.txt`
digest is checked; unlisted or unevaluated games do not. The shared POSIX-sh parser generates
`docs/compatibility.md` with predicate/fixture and status × machine × mapper counts;
`policy.compat_doc_fresh` regenerates and compares it in CI and hygiene.

- **Ratchet:** normalized identity includes ROM, resolved machine/region/TV/board/video, devices,
  script identity/content and predicate/frame/address. Expected hashes/bytes are protected values,
  excluded from the key. Allowed transitions: `unknown` → anything, `fail` → `pass`, `pass` → `pass`.
  Deleted passing cases, substituted profiles/scripts or changed expected values need a dated line
  in `tests/compat/allow.txt` naming the behaviour-revision bump and expiry within 60 days.
  CI compares fresh results in both directions; protection is the following implementation slice.
- **Protected baseline:** the ratchet job first checks out `github.event.pull_request.base.sha`, copies `tests/compat/ledger.csv` and `scripts/compat-ratchet.sh` to `$RUNNER_TEMP`, and only then checks out the PR; main's script judges main's ledger against the PR's results, so a PR cannot loosen its own judge [CF.14].
- **Owner's dumps, locally:** `scripts/library.sh` loads git-ignored `.env.local` (`COGMASTER_ROM_DIR`, optional `COGMASTER_BIOS_DIR`), runs `cmake --workflow --preset library`; configure fails if the directory is missing (no self-skip). It hashes each file, looks the headerless SHA-1 up in the build-time table from the three CC0 lists, registers `library.<id>` only for matches, and writes `build/library/results.csv`; `scripts/compat-update.sh` merges it into the ledger. What leaves the machine: the public SHA-1, the CC0 title, the predicate result (a frame or RAM hash is a single fact, ASSET_POLICY §2), the date. What never leaves: file names, paths, any hash of a file absent from the lists (such files are counted, not named), frames, RAM dumps, saves (ASSET_POLICY §1). `ci*` presets never define `COGMASTER_ROM_DIR` or register library tests, so the exact inventory holds (ENGINEERING 3). Hygiene rejects `.env.local`, ROM paths and ROM bytes.
- **Recommendation:** nine-column conventional quoted CSV and one shared bounded parser. **Strongest
  alternative:** sorted TSV [CF.15]. **Would change if:** a consumer needs embedded record newlines;
  quoted commas and semicolons are already covered by executable controls.

## 6. CI tiers and cost
| Tier | Contents | Estimated added minutes | Basis |
|---|---|---|---|
| Per PR × 6 legs (ubuntu 26.04 x64/arm64, macos-15 arm64/x64, windows-2025 x64/arm64) | units; committed vector sample; WLA-DX built from the pinned tarball, fixtures assembled and hashed; fixture frame/audio/state hashes; `hash-equality`; `fuzz.regress.*`; `policy.*`, `hygiene.*` | +1–2 per leg | vectors: 160,400 tests × an assumed N = 5 µs per test (load state, run one instruction, compare about 12 samples) ≈ 0.8 s; phase 2 measures N and the number replaces this estimate. Fixtures: 8 images × 600 frames = 4,800 frames at an assumed ≥ 1,000 fps ≈ 5 s. WLA-DX build: small C, assumed 20–60 s; fetch 1.4 MB. All (derived) [CF.25] |
| Per PR × 1 (Linux) | `asan` short runs (vectors and fixtures at ×3), `be32` under qemu (vectors ≈ 160,400 × 5 µs × 20 ≈ 16 s), `wasm32` report, `perf` Ir/frame (4 ROMs × 360 frames under Cachegrind, assumed 50× ≈ 2 min), ratchet, doc freshness | 3–5 | ENGINEERING 4, 8 [CF.18] |
| Nightly | full vectors (fetch at the pin, convert, run: 1,604,000 × 5 µs ≈ 8 s compute, download and JSON conversion dominate; repository 277 MiB); ZEXALL and ZEXDOC (1–3 h of emulated time at an unmeasured speed-up: budget set after the first run, per-group timeout); fetched suites (VDPTEST, sverx, homebrew) with MAME `sg1000m3`/`gamegear` and libretro-host GPGX locators; `bench` tails p50–p99.999; libFuzzer 60 s × 4 targets; `suite-flake` | 15–40 | measured after the first ten runs |
| Local | `library` preset; owner-BIOS MAME drivers; ares, Emulicious by hand; hardware sessions; one RetroArch session per milestone (HOSTS 3) | owner and agent time | not budgeted |

Nightly red opens or updates one rolling `nightly` issue and never blocks a release (AGENTS rule 8); the exact-inventory check (`tests/inventory/<preset>.txt`) covers the nightly preset too, so a lane that silently stops running is a configure failure, not a green run (XFER-013).

**Measured before gating:** a new lane runs report-only (outside `CI required`) for its first 10 runs on `main`; it then gets a timeout of twice its slowest of the last 40 successful runs with the run ID in a comment, and joins the roll-up (XFER-012). A lane over budget moves down a tier rather than being trimmed blindly. Oracle agreement never gates. Fuzz targets are report-only until green for the same 10 runs (XFER-049). Fetched and built third-party ROMs are never uploaded as artifacts: anyone with read access to a public repository can download them [CF.14]. Per-PR wall time target ≤ 12 min at v1 close (ENGINEERING 4).
- **Recommendation:** the tiers above. **Strongest alternative:** ZEXDOC on one PR leg (minutes to hours, unmeasured). **Would change if:** nightly ZEXALL catches a regression the vector sample missed more than once in a milestone.

## 7. Admission rules for third-party fixtures
1. Judge the **bytes that end up in the ROM**: game or test code, the C runtime (`crt0`), libraries (SMSlib, PSGlib, GSLib, and the ZX7/aPLib decompressors SMSlib embeds), fonts, graphics, music drivers and data, generated bytes (headers, checksums are tool output). A tool's own licence (WLA-DX GPL-2+, SDCC GPL) does not reach its output unless the tool copies its own runtime in (SDCC does, through `crt0.rel` and `libsdcc`, SP.174) (XFER-019).
2. Licence evidence = LICENSE plus per-file headers **at the exact pin** (XFER-020); re-check at every pin bump.
3. Disposition: **commit** only if every byte source is MIT, BSD, ISC, Zlib, Unlicense, CC0 or explicitly public domain, no third-party logo is drawn, and the notice is in `THIRD-PARTY-NOTICES.md`; **build-ci** or **fetch** for unstated or mixed licences and for copyleft binaries (digests only, no artifacts); **local** for commercial dumps, BIOS and anything not script-fetchable (forum attachments); **binary-oracle** when the source may not be opened to learn its result layout.
4. Firmware inside permissive repositories stays out (ASSET_POLICY §1).
5. Manifest row (ASSET_POLICY §2): `path<TAB>source<TAB>pin<TAB>licence<TAB>sha256<TAB>notice`; licence is an SPDX expression, `NOASSERTION` when unstated; build-ci rows live in `tests/built/expected.txt` with tool pins, source pins, ROM SHA-256 and frame/audio hashes.
6. Own ROMs: MIT source committed, bytes built, own glyphs, rebuilt and compared on every leg (§2).
7. **Byte attribution for an SDCC build** (the devkitSMS case): build at the pin on the Linux leg with `--no-std-crt0` left off (as the upstream Makefile does), keep the linker map, and sum the bytes each module contributes (`crt0_sms.rel`, `SMSlib`, `PSGlib`, the game's objects, data banks from `assets2banks`); the sum must equal the image size minus fill, and every module must have a licence row in the admission note. The map's exact layout at SDCC 4.6.0 is (unverified) until phase 3 runs it; if the map is unusable, link twice with and without each library and diff the images.
8. **Fetched release zips** are admitted by digest only: the first fetch records SHA-256 of the zip and of each `.sms`/`.gg` inside in `tests/built/expected.txt`; a later fetch that differs is an `infrastructure_error`, and a pin bump is a PR.

**First admission decision (phase 3): the devkitSMS crt0 exception.** Every devkitSMS-built ROM (JoppyFurr's, haroldo-ok's, sverx's) contains `crt0_sms.s` object code, GPL-2 with a special exception (Michael Hope 2000, sverx 2022) [CF.08]. As read so far (SP.176): linking the crt0 into an executable does not by itself place the executable under the GPL; it removes no other reason the executable might be GPL; changes to the crt0 stay GPL-2. What it does **not** say (unverified until quoted): whether redistributing the ROM still requires the GPL text and a source offer for the crt0 bytes themselves, and whether SMSlib's embedded ZX7/aPLib routines carry terms of their own. `policy.notices` admits only `MIT` or `MIT AND` a permissive licence, so a committed devkitSMS ROM would fail it unless `tests/roms/` is granted a `GPL-2.0-only WITH <exception>` row. Decision procedure: quote the exception verbatim in `THIRD-PARTY-NOTICES.md`, quote SMSlib's decompressor headers, then choose: (a) keep **build-ci/fetch at pin** (default; no policy change; the ROMs still run nightly), or (b) admit `commit` for MIT games with the crt0 notice and a pointer to the pinned source. Nothing in v1.0 depends on (b).
- **Recommendation:** rules 1–6 and default (a). **Strongest alternative:** admit by repository licence alone (would wrongly commit SDK bytes). **Would change if:** upstreams add per-file SPDX tags a script can check, or nightly fetch flakiness costs more than a notice.

## 8. Open questions
This file answers PROMPT-COVERAGE rows 3, 7, 10, 27 and 36 [CF.23]; each open item below names the phase that closes it, so no GSD phase re-asks.

| Question | Settled by |
|---|---|
| N µs per vector test; sample size in MB; ZEXALL emulated-time speed-up | phase 2 (Z80): measure, write the numbers into §6 and the lane timeouts |
| Does `ZEXALL-SMS-0.21.zip` carry both `zexall.sms` and `zexdoc.sms`; their SHA-256 | phase 2 first fetch |
| Does the libretro host load buildbot GPGX/Gearsystem cores headless on ubuntu 26.04; which `SYSTEM_RAM` span each exposes | phase 3 nightly lane, report-only |
| MAME `gamegear` with an `.sms` image: SMS mode or refusal; `sg1000m3` as the SMS1 locator for VDPTEST | phase 3 |
| crt0 exception text and SMSlib decompressor terms (§7) | phase 3, first admission decision |
| Which consoles, flash cartridge, capture path and MiSTer the owner has; PAL unit? | owner, when the first hardware run is requested (v1.0 phase 3 at the earliest) |
| z80test shim and its admission | v4.0 |
| Compact-vector regeneration if SingleStepTests ships a new pin (SP.010 precedent) | nightly: a pin bump is a PR that regenerates the sample and re-runs the full set |

## Sources
| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| CF.01 | T1 | github.com/SingleStepTests/z80 (LICENSE MIT, README, `v1/` tree, issues #3 #5 #11; `gh api repos/…` size 283,823 KB (tested)) | `ebe1875d48f3` | vector format, counts, NMOS model, repository size |
| CF.02 | T1 | github.com/maxim-zhao/zexall-sms (LICENSE GPL-2.0; releases `v0.21` asset `ZEXALL-SMS-0.21.zip` listed via `gh api` (tested)) | `8bfcd2ca594f`; tag `v0.21` | ZEXALL disposition and fetch target |
| CF.03 | T1 | github.com/raxoft/z80test (license.txt MIT, readme.txt) | `c490c0ca94f1` | z80test shim need |
| CF.04 | T1 | github.com/FluBBaOfWard/SmsVdpTest (no LICENSE at pin) | `490c00b38c83` | VDPTEST disposition |
| CF.05 | T1 | github.com/sverx/SMSTestSuite, github.com/sverx/GGTestSuite (no LICENSE at pin) | `985bc04fef6f`, `1a29804d1fc8` | suites disposition |
| CF.06 | T1 | github.com/JoppyFurr/SN76489-TestRom, YM2413-TestRom (LICENSE MIT; devkitSMS build) | `7d2068dfc171`, `571018fe8bfb` | audio ROMs |
| CF.07 | T1 | haroldo-ok/{dragon-blaster, food-fighter, river-strike, sms-rogue}, JoppyFurr/Snepzhen-Solitaire, liamhays/tunnel, GameGearSamples/ButtonTest (LICENSE files at pin, release assets) | `5b48f0570aeb`, `48fe2ea96795`, `acfcc478c14e`, `329d788feb01`, `94d9d454541a`, `79c0c21dffbe`, `8c65ecd39ed6` | homebrew candidates |
| CF.08 | T1 | github.com/sverx/devkitSMS `LICENSES.txt`, `crt0/src/crt0_sms.s` header | `533ae572c897` | crt0 exception, SMSlib terms |
| CF.09 | T1 | github.com/vhelin/wla-dx tag `v10.7` tarball, SHA-256 computed in SMSPOWER-EXTRACTS 7 | `91c52b1f4ef3`; digest `38296a96…925a` | assembler pin |
| CF.10 | T1/T3 | github.com/mamedev/mame `src/mame/sega/sms.cpp` L1015–1165 (BSD-3; fetched raw at the pin and read here (tested)); `hash/sms.xml`, `gamegear.xml`, `sg1000.xml` (CC0); mametesters 6053 via SMS-VDP 8 | `1d7a454ed79e` | BIOS requirement per driver, CC0 titles and SHA-1 keys, VDPTEST pass |
| CF.11 | T1 | github.com/ares-emulator/ares LICENSE (ISC), releases | `9408cb43d494` | ares as a local locator |
| CF.12 | T1 | github.com/ekeeke/Genesis-Plus-GX LICENSE.txt (non-commercial); libretro buildbot core downloads; docs.libretro.com Gearsystem page | accessed 2026-10-18 (via PROVENANCE PV.13, CART.24–26) | binary-only oracles |
| CF.13 | T3 | github.com/MiSTer-devel/SMS_MiSTer (no LICENSE; facts via SMS-VDP VDP.07) | `8443a0d19a88` | referee status |
| CF.14 | T1 | github.com/szTheory/muhtari `.planning/preparation/CONFORMANCE.md` (format exemplar; console-agnostic mechanisms: disposition key, mailbox shape, ledger and ratchet, protected baseline, measure-before-gating, artifact exposure, MAME Lua method) | `952fce970117` | mechanisms only; no 7800 fact reused |
| CF.15 | T1 | github.com/szTheory/nesturbator (`tests/roms/manifest.txt`, binary vector precedent) | `9f9386c661f6` | manifest and TSV alternative |
| CF.16 | T1 | `ASSET_POLICY.md` §1–5, `PROVENANCE.md` §1–4, `AGENTS.md` rules 1, 6–8, 11, 12 | this repository, 2026-10-18 | dispositions, manifest format, binary-only rule |
| CF.17 | T1 | `.planning/LESSONS.md` XFER-005, 012, 013, 016, 019, 020, 025, 044, 049, 050, 058 | this repository, 2026-10-18 | vectors first, exact inventory, playability proof, self-authored fixtures, fuzz report-only, tolerant acceptance |
| CF.18 | T1 | `.planning/preparation/ENGINEERING.md` §1, 3, 4, 8 | 2026-10-18 | runners, inventory, lanes, Ir gate, timeouts |
| CF.19 | T1 | `.planning/preparation/ARCHITECTURE.md` §5–8, 11, 13 | 2026-10-18 | hashes, peek, first fault, mailbox placement, build order, open conflicts |
| CF.20 | T1 | `.planning/preparation/SMS-VDP.md` §8, 10; `SMS-SYSTEM.md` §9; `SMS-AUDIO.md` §6, 7, 9; `SMS-CARTRIDGES-AND-PERIPHERALS.md` §1, 2, 7, 8 | 2026-10-18 | test ROM facts, open questions, mapper semantics, save sizes |
| CF.21 | T1 | `.planning/preparation/HOSTS-AND-INTEGRATION.md` §3–4 | 2026-10-18 | RetroArch e2e bounds, runner predicates, continuation protocol |
| CF.22 | T1/T2/T4 | `.planning/preparation/SMSPOWER-EXTRACTS.md` SP.001–SP.011, SP.020, SP.021, SP.038–SP.041, SP.047, SP.048, SP.052, SP.072, SP.073, SP.086–SP.088, SP.093, SP.094, SP.107–SP.115, SP.120–SP.155, SP.170–SP.179 | pins as listed there | vector semantics, hardware numbers, ROM inventory, toolchain |
| CF.23 | T1 | `.planning/preparation/PROMPT-COVERAGE.md` rows 3, 7, 10, 27, 36 | 2026-10-18 | what this file must answer |
| CF.24 | T1 | GitHub REST `repos/{o}/{r}/releases`, `git/trees` (zexall-sms), `repos` (SingleStepTests) run here | 2026-10-18 | release asset and size facts marked (tested) |
| CF.25 | T1 | Arithmetic run here: 1,604 × 100 = 160,400; × 5 µs = 0.802 s; 1,604,000 × 5 µs = 8.02 s; 126 B × 160,400 ≈ 20.2 MB; qemu factor 20 assumed | 2026-10-18 | §1.1 and §6 estimates (derived) |
