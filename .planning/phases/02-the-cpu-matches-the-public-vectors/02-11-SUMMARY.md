---
phase: 02-the-cpu-matches-the-public-vectors
plan: 11
subsystem: runner
status: complete
tags:
  - runner
  - probe
  - fault
  - seeds
requires:
  - "02-09: cogmaster_probe and cogmaster_cart_info"
  - "02-10: the first-fault call sites and the COGMASTER_FAULT_KIND_* and COGMASTER_FAULT_DEVICE_* codes"
provides:
  - "cogmaster-run loads a cartridge: positional FILE or --rom FILE, read behind a 64 MiB guard"
  - "--machine auto resolves header region code, then a .gg name, then the Master System; --region auto|ntsc-u|ntsc-j|pal sets region and tv"
  - "--probe prints 21 key=value lines in a fixed order: header fields, guesses, and sha1, sha256, crc32, md5 over the whole file and after a copier header"
  - "Exit 77 (refused) for unsupported size, board or profile, files over 64 MiB, and .sg/.sc files with 'unsupported: SG-1000 (planned)'"
  - "--trace first-fault: one 'fault frame=... kind=...' line or 'fault none', exit 0"
  - "runner/args.c, runner/probe.c, runner/sha1.c (RFC 3174), runner/md5.c (RFC 1321)"
  - "cogmaster-test-seeds: build-time synthetic images in build/<preset>/tests/seeds, the 64 MiB + 1 file in build/<preset>/tests/oversize"
affects:
  - "Plan 16 (fuzz): the replay and libFuzzer read build/<preset>/tests/seeds"
  - "Later runner options (--mailbox, --expect) join runner/args.c"
actuals:
  tokens: 22600
  tasks: 3
  commits: 5
plan_head_before: 822561078f52c40a77adb12c6fbded90b4142124
plan_head_after: 69b54212bf340125b4f2b42c82a3817d45517ff8
tech-stack:
  added: []
  patterns:
    - "Digests stay in runner/, each a small init/update/final module written from its RFC, so the core links only mem*"
    - "Test images are generated at build time by a C program run through the lane's emulator; no image bytes are committed"
    - "The 64 MiB guard reads one byte at offset 64 MiB, so it needs no file size arithmetic that a 32-bit long could overflow"
key-files:
  created:
    - runner/args.c
    - runner/args.h
    - runner/probe.c
    - runner/probe.h
    - runner/sha1.c
    - runner/sha1.h
    - runner/md5.c
    - runner/md5.h
    - tests/fuzz/seeds.c
    - tests/fuzz/tests.cmake
  modified:
    - CMakeLists.txt
    - runner/main.c
    - runner/exit_status.h
    - tests/runner/tests.cmake
    - tests/runner/test_runner.c
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/runner.md
    - .planning/preparation/HOSTS-AND-INTEGRATION.md
decisions:
  - "Header-derived probe fields (checksum to rom_size_code) print none when the image has no ROM header, like header_at"
  - "machine_guess is what --machine auto picks (header, then .gg, then sms), so it is always sms or gg; region_guess is ntsc-j for a Japanese region code, else ntsc-u"
  - "--probe exits 77 for an unsupported size as well as an unsupported board, after printing every key"
  - "--probe takes only a file; combining it with --frames, --bench, --hash, --png or --trace is a usage error"
  - "Exit 77 is named 'refused' in the table that --help and docs/runner.md share"
  - "--trace first-fault prints its line before any --hash or --bench output; a stop signal before any fault still prints fault none"
requirements-completed: []
coverage:
  - id: D1
    description: "--probe of a 128 KiB export header image prints the 21 keys in order with tolerant formats, header_at $7FF0, checksum_ok 1 and mapper_guess sega; a bad checksum prints checksum_ok 0"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.probe\\.keys$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "SHA-1 and MD5 equal the RFC 3174 and RFC 1321 vectors for the empty string, abc, 56-byte, 62-byte, 80-byte, 640-byte and one-million-byte inputs; the md5, sha1 and sha256 lines equal CMake's digests of the whole file"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.probe\\.digests$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "A 16 KiB + 512 image prints copier_header 1 and headerless digests equal to the digests of the same body without the copier header; a headerless image prints none for all four"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.probe\\.headerless$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "An image storing to another board's registers exits 77 from --probe with every key printed and mapper_guess unsupported"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.probe\\.unsupported$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D5
    description: "1,023-byte and 1 MiB + 513 images exit 77; 1,024 and 1 MiB + 512 run; a 64 MiB + 1 file exits 77 naming 64 MiB, for a run and for --probe"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.refuse\\.size$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D6
    description: ".sg, .sc and .SG files exit 77 with unsupported: SG-1000 (planned), for a run and for --probe"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.refuse\\.sg1000$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D7
    description: "Game Gear or Japanese console with PAL, an 8 KiB Game Gear image and another board's image exit 77"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.refuse\\.profile$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D8
    description: "A Game Gear header image named .sms and a headerless .gg image hash like --machine gg and unlike --machine sms; a headerless .sms hashes like --machine sms"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.load\\.machine_auto$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D9
    description: "--region auto, ntsc-u, ntsc-j and pal each run two frames with exit 0; an unknown name exits 2"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.load\\.region$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D10
    description: "LD A,0 then IN A,($00) prints one fault line with op $DB, addr $0000, width 8, dev ctrl and kind unmapped_read, exit 0"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.trace\\.fault$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D11
    description: "DI; HALT prints fault none, exit 0, with the hash line after it when --hash is given"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.trace\\.none$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D12
    description: "The exit status table --help prints, now with 77 refused, equals docs/runner.md's"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.exit\\.table$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D13
    description: "A file name, --probe, --trace first-fault and --region are accepted; a second file, an empty --rom, --trace other than first-fault, an unknown region and --probe with run options are usage errors; an unreadable file exits 1"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.usage\\.' --output-on-failure"
        status: pass
    human_judgment: false
duration: 9min
completed: 2026-10-20
---

# Phase 2 Plan 11: Runner probe, cartridge loading, refusals and first-fault trace Summary

`cogmaster-run` now loads cartridges, picks the machine and region, refuses what it cannot run with exit 77, prints a 21-key `--probe` report with SHA-1, SHA-256, CRC-32 and the RetroAchievements MD5 over the whole file and the headerless body, and prints the first guest fault with `--trace first-fault`. Test images come from a C generator that runs at build time.

## Performance

- **Duration:** about 9 minutes of measured wall time
- **Started:** 2026-10-20T03:54:15Z
- **Completed:** 2026-10-20
- **Tasks:** 3
- **Files modified:** 20

## Accomplishments

- The command-line parsing moved into `runner/args.c`, so `runner/main.c` is 383 lines, under the 800-line cap.
- `runner/sha1.c` and `runner/md5.c` were written from RFC 3174 and RFC 1321. They pass the RFC vectors, including inputs that cross block boundaries and a one-million-byte input.
- `--probe` prints the fixed keys. The four `*_headerless` digests match the digests of the same body with no copier header.
- Exit 77 is now in the shared exit table. It covers sizes, boards and profiles the core does not support, files over 64 MiB (refused before they are read) and SG-1000 file names.
- `--trace first-fault` prints `fault frame=0 line=192 dot=329 pc=$0002 op=$DB addr=$0000 width=8 dev=ctrl kind=unmapped_read` for the `in-a-00.sms` image.
- `tests/fuzz/seeds.c` writes 31 images into `build/<preset>/tests/seeds`. There is one for each probe branch: all three header positions, the Codemasters board, another board, the Game Gear cheat device, the boundary sizes, and the small programs. The 64 MiB + 1 file goes into `tests/oversize`, outside the seeds directory.
- HOSTS-AND-INTEGRATION section 4 now uses the phase-2 kinds and devices in its first-fault line.

## Task Commits

1. **Task 1 (tracer): --probe through cogmaster_probe and the digests**: `0d9f662` (feat)
2. **Task 2: loading, machine and region, refusals and exit 77**: `8d0d28c` (test, RED), `c96e513` (feat, GREEN)
3. **Task 3: --trace first-fault**: `3ebc432` (test, RED), `69b5421` (feat, GREEN)

## Decisions Made

See the frontmatter `decisions`. The ones scripts depend on: header fields print `none` when there is no ROM header; `machine_guess` and `region_guess` report what `auto` resolves to; `--probe` exits 77 for an unsupported size as well as an unsupported board.

## Deviations from Plan

### Ordering

**1. Exit 77 and its docs table line landed in Task 1, not Task 2**
- **Found during:** Task 1
- **Issue:** the plan's Task 1 has `--probe` exit 77 for an unsupported board. `runner.exit.table` fails unless `--help` and `docs/runner.md` list the same statuses.
- **Fix:** Task 1 added `COGMASTER__EXIT_REFUSED` and the docs line. Task 2 added the load refusals and the rest of the docs.
- **Commit:** `0d9f662`

**2. Some Task 2 tests passed at RED**
- `runner.probe.headerless`, `runner.probe.unsupported`, `runner.load.machine_auto` and `runner.load.region` already passed after Task 1. Task 1 had to load a file to make `runner.usage.positional` meaningful, so it already resolved the machine. The RED commit failed on the three refusal tests.

### Additions

**3. [Rule 2] `size-16k.sms` seed**
- This is the body of `size-16k-512.sms` without its copier header. With it, `runner.probe.headerless` checks the headerless digests against CMake's own digests of the same bytes, not just that they differ from the whole-file digests.
- **Commit:** `8d0d28c`

## Issues Encountered

None. The wasm32 lane could not be built locally because there is no WASI toolchain. CI runs the generator and the runner under wasmtime, using the preset's emulator, which covers the source tree.

## Known Stubs

None. With no input, sound or video drawing yet, a loaded cartridge's picture is black. `docs/runner.md` says so. Later phases add those features.

## Verification

- `cmake --workflow --preset ci`: exit 0, 164 tests plus 4 release tests.
- `cmake --workflow --preset hygiene`: exit 0, 23 tests.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- The plan's acceptance checks also pass: `main.c` has fewer than 800 lines; no header magic text in `runner` or `tests`; the `$3FF0` and `$1FF0` seeds exist; `--help` shows a 77 line; the oversize file exists and nothing over 64 MiB is in the seeds directory; the old fault vocabulary is gone from HOSTS; and no planning IDs appear in `src`, `runner`, `tests`, `docs` or `include`.

## Next Phase Readiness

CPU-05 is still open: its fuzz clause (nightly loader fuzzing and the per-PR `fuzz.run` replay) lands in plan 16, which reads `build/<preset>/tests/seeds`.

## Self-Check: PASSED
