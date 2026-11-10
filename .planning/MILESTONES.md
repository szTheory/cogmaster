# Milestones

## v1.0 SMS and Game Gear games play with sound in RetroArch (Shipped: 2026-11-09)

**Phases completed:** 4 phases, 62 plans, 137 tasks (the per-plan task counts in STATE.md; the archive tool counted 53 from summary front matter)

**Delivered:** Master System and Game Gear games play with sound in RetroArch from an integer-only MIT C library, with the Z80 vector-exact and frame and audio hashes identical on every platform. Released as v0.1.0 to v0.4.0, one per phase; phase 4 was PR #11 and the release v0.4.0 PR #12.

**Stats:**
- Requirements: 26 of 26 satisfied (FRAME 9, CPU 5, GAME 7, SND 5); audit PR #13, status tech_debt, no gaps, 18 of 18 connections, 6 of 6 flows
- Timeline: 22 days, 2026-10-18 (scaffold commit 66fb150) to 2026-11-09 (audit merge 1a75a2b); 13 commits on main, each phase one squashed pull request
- Files changed in the range: 482, with 93,711 insertions and 176 deletions (git diff --shortstat 66fb150 HEAD)
- Code: about 7,700 lines of C in `src/` and `include/`, about 26,000 with `tests/`; 399 of 399 tests in the `ci` workflow
- Git range: 66fb150 to 1a75a2b

**Known verification overrides:** 11 newly acknowledged, 0 carried forward from a prior close (see STATE.md Deferred Items). Closeout type: override_closeout. Items: 1 pending todo, 8 dormant seeds for later milestones, 2 deferred-item notes (one resolved by commits 3834dff and 2cceac5, one a local actionlint metadata limitation). The phase 1 to 3 verification clocks read stale (a summary changed after the report); every phase report says passed and the milestone audit confirmed them.

**Tech debt carried forward** (from the milestone audit, `.planning/milestones/v1.0-MILESTONE-AUDIT.md`):
- Phase 1: provisional CI timeouts (windows-arm64 is 3 minutes and failed one main run); phase 1 review dispositions never triaged (WR-02 and WR-04 still live); the post-merge CI follow-ups todo (perf gate in report mode, nightly issue path unexercised, no dependabot configuration, wasm32 report-only).
- Phase 2: review findings IN-01 to IN-06 deferred; fuzz targets do not drain audio or the trace log.
- Phase 3: the README Quick start covers the no-content test card only.
- Phase 4: the `boot_to_play` perf allow line expires 2027-01-07; the hosted sound check proves non-silence, not PCM equality, and `tests/retroarch` is at 299 of 300 lines; review IN-01 to IN-04 open; be32 and wasm32 audio timings provisional; nightly lanes not yet run on the sound code; `cogmaster_info.audio_channels` and `.fm` are read only by tests; third-party sound ROM conformance deferred with no lane.

**Key accomplishments:**
- The library draws the SMS and Game Gear test cards into the real 256x192 RGB444 field, and `cogmaster-run --frames 60 --hash` prints the SHA-256 of each crop. Both digests equal the independent planning oracle, and a 14-test exact-inventory `ci` workflow pins them.
- The test-card instance now has an internal little-endian state visitor. Its 64-byte header, counters and first-fault record round-trip byte for byte, and a RECORD-driven sweep refuses every out-of-range field without changing the instance. The public API gains `cogmaster_reset`, `cogmaster_get_fault` and `cogmaster_fault`, and a test shows that no error path of any public call changes the instance's state.
- `cogmaster-run --png N:FILE` now writes frame N's crop as a stored-deflate RGB PNG. The colours come from an own palette helper in `host/` that multiplies each RGB444 channel by 0x11, so `c >> 4` gives the original back. The SMS card's PNG has a pinned SHA-256, and the test compares that digest only after an independent decoder has matched every pixel to the card.
- `cogmaster-run --bench N` prints nearest-rank frame-time percentiles in whole microseconds and peak RSS in KiB. SIGINT and SIGTERM stop the run cleanly after the current frame. `--help` and `--version` work, and `docs/runner.md` holds an exit table that a test keeps equal to `--help`. A test proves the core makes no allocator call after load. The Ir/frame comparison script is ready in report mode.
- `cogmaster_libretro` loads with no content and shows the Master System test card. It reports the NTSC rate 1,640,625/27,379 Hz and its geometry from `cogmaster_get_info`, and sends silence of the exact rational length. An in-process host runs it as RetroArch does. The host turns frame 60 back into RGB444 and gets the same pinned hash as `cogmaster-run --frames 60 --hash`, in both pixel formats and for the Game Gear geometry too.
- `nm` now checks the Release `libcogmaster.a` on three points: it imports only the four memory functions (plus Apple's `bzero`), it has no writable data, and it exports only the `cogmaster_` names the header declares. A fixture archive is the negative control for each check. The header compiles alone as C17 and as C++17 with warnings fatal. A version-1 consumer drives the whole API. Byte budgets, the no-self-skip rule, guide/header name agreement and the notices rule all run in `ci`, each with a control that must fail.
- SingleStepTests z80 vectors run end to end. A C converter writes a 13 MB access-form sample, and a link-seam harness rebuilds every T-state through the same stream builder. The new Z80 core passes all 100 sampled vectors each for NOP and LD (BC),A, and all 200 for the DD/FD 00 prefix steps.
- An owned ROM now draws Mode 4 tiles and responds to absolute pad input through the library, runner and production libretro module, with independently calculated pixels.
- Shared CMake/POSIX checks preserve the existing negative controls while enforcing 448 counted physical lines against 4,516 core lines and the complete 293-line RetroArch helper budget.
- RetroArch 1.22.2 replays real frontend input into the owned SMS game, and both released and scrolled screenshots match the runner and independent pixels within a complete 300-line hosted helper budget.
- One Mode 4 renderer now emits immutable raster intervals with complete background semantics and shared SMS1 address and bus rules.
- The live Mode 4 field now composes eight saved SAT slots, SMS1 count-dependent zoom, ninth-match overflow and shared-opaque-dot collision.
- Completed-field libretro presentation preserves native GG identity while a real RetroArch Start replay matches every pixel of the runner's LCD crop.
- Fresh runner processes continue classified battery counters after exit and signals; libretro exports the same stable writable span through Restart.
- The runner judges accepted bounded predicates; one quoted CSV parser publishes four fixture evaluations and explicit unknown/zero coverage without support claims.
- Normalized passing compatibility cases now ratchet through a copied base judge, with bounded dated revision exceptions and fresh fixture agreement.
- Requirements, roadmap criteria, decisions and preparation notes aligned to owned audio fixtures, an achievable 801 or 802 per-frame cadence and one owner-confirmed SOFT-reset rule, with third-party ROM conformance recorded as deferred.
- API 9 (behaviour 11, state 11) with an exact 801/802 (965/966 PAL) per-frame PCM cadence from the library, a libretro adapter that drains it with no pacing state of its own, and a caller-owned, allocation-free-after-load audio trace log.
- API 9 completed: media.fm chooses the YM2413 at load, info reports audio channels and the resolved FM unit, the state carries the fm_unit header byte and a bounded synth phase with a signed visitor helper, abi.api_v9 runs in every abi lane, and docs/audio.md states the host contract.
- Band-limited step synthesis behind cogmaster__synth_level: the nesturbator 16 x 32 Q15 kernel, a 1,024-slot int64 impulse ring per channel, a level-gaining mixer (840, or 491 and 1402 beside the FM unit), a leaky DC blocker and a saturating int16 output, saved in the state and proven by seven portable integer tests (85.0 dB alias suppression, 28,944 peak against 29,204).
- The SN76489 model in src/psg.c: Z80 writes to $40-$7F stepped lazily on a 48 tick grid and stamped at the write tick into the band-limited synth, reset to power-on constants with its levels snapshotted through the synth at tick 0, saved with a bounded step origin, and proven by eleven closed-form cases (all 1,024 periods, all 16 volumes on 4 channels, the 8 noise controls, the 57,337 shift cycle).
- A write to port $06 in the Game Gear's native mode catches the PSG up and routes each channel left or right by its mask bit with one stored copy, the Game Gear port block and $3E/$3F decode now follow the video mode (so a Game Gear console in Master System mode ignores $06 and decodes $3E like a Master System), and a whole-frame OUT hammer proves PSG steps, kernel additions, ring reach and the trace log stay under closed-form bounds on NTSC, PAL and Game Gear.
- cogmaster-run --hash-audio drains the core's level-change log and samples every frame, merges the log into a canonical stream of 11-byte transitions and prints two SHA-256 lines, and the owned table-driven psg_sweep fixture exercises every PSG register bit and extreme with 38 judged mailbox records.
- The Game Gear image of the owned psg_sweep fixture plays in stereo through the runner, and the four psg_sweep transition and PCM hashes are frozen in tests/audio and compared on every lane through hashes.txt.
- The playable fixture now chimes at boot, plays a decaying tone 1 note on each new press and alternates Game Gear speakers by counter parity, with its picture, ledger and mailbox evidence unchanged and four audio rows frozen in tests/audio.
- An integer-only, globals-free YM2413 (699 logic lines in src/opll.c, src/opll_reg.c and src/opll.h) ported from MIT emu2413 and proved equal to the unmodified original over 2,000,000 seeded register-stream samples, with its notice, provenance and a tightened notices check landing in the same plan.
- fm.lockstep proves the integer YM2413 equals the unmodified emu2413 over 2,000,000 seeded samples against a digest frozen in a text file (nothing fetched), seven chip-level cases pin key-on, the drums, the noise register, both LFOs, the test register, the slot count and the sum of the sounding voices, and the run found that the sum the chip returns can leave 16 bits.
- $F0/$F1 write the YM2413 on an exact 216-tick catch-up, $F2 mutes PSG or FM at the mixer, the 16-bit FM sum is mixed through a total saturating bias so no int16 assumption remains, and the chip's block, reset snapshot and cost bound are saved, tested and documented.
- fm_sweep, an owned WLA-DX program that detects the YM2413 through its own $F2 routine (1 on the Japanese console, 0 on export and Game Gear), plays instrument-1 notes, every rhythm key and $F2 steps 0 to 3, and is judged by mailbox rows, a C detection test under media.fm and three new frozen pin rows each for transitions and PCM.
- An in-process host that plays psg_sweep and boot_to_play on SMS and GG through the libretro module with batch-only, single-only and both callbacks and requires the runner's exact PCM each time, plus the cogmaster_fm option (auto, off, on) in all three option tables.
- A real RetroArch 1.22.2 records psg_sweep on both consoles and a silent control, judged by a 19-line cmake -P checker inside the 300-line hosted budget, and the pushed phase head is green on every hosted lane with equal audio hashes.

---
