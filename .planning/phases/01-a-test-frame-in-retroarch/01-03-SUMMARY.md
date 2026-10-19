---
phase: 01-a-test-frame-in-retroarch
plan: 03
subsystem: runner
tags:
  - png
  - palette
  - host-helpers
  - runner
requires:
  - "01-01: cogmaster-run, cogmaster_get_frame crop, cogmaster__testcard_pixel, cogmaster_add_test and the inventory check"
  - "01-02: the core's cogmaster__crc32, which the runner must not redefine"
provides:
  - "host/palette.h, host/palette.c: cogmaster__rgb444_to_rgb888 (times 0x11 per channel) and cogmaster__rgb444_to_rgb565, built as the static library cogmaster-host"
  - "runner/png.h, runner/png.c: cogmaster__adler32, cogmaster__png_size, cogmaster__png_write (stored-deflate RGB, whole file in memory)"
  - "runner/crc32.h, runner/crc32.c: cogmaster__runner_crc32"
  - "cogmaster-run --png N:FILE, repeatable, 1 <= N <= --frames"
  - "COGMASTER_TESTCARD_PNG_SHA256_SMS in tests/testcard_pins.cmake"
  - "cogmaster-test-host and cogmaster-test-runner test binaries; tests/runner/tests.cmake also runs as a -P script with CASE=png_file or png_syntax"
affects:
  - libretro adapter (reuses cogmaster-host)
  - hosted RetroArch screenshot compare (reference PNG)
  - runner help and bench
actuals:
  tokens: 10500
  tasks: 1
  commits: 2
plan_head_before: 9fe0dea42dad71a6c220852a7978e578427cd8d7
plan_head_after: cce96b6a57bb37fe952c85855506f3d7f7aa292d
tech-stack:
  added: []
  patterns:
    - "Host helpers live in host/ as the cogmaster-host static library, outside the core's determinism boundary"
    - "A runner test file doubles as a cmake -P script (CMAKE_SCRIPT_MODE_FILE guard) for multi-step checks"
    - "A pinned file digest is compared only after an independent decode of the same file passes"
key-files:
  created:
    - host/palette.h
    - host/palette.c
    - runner/png.h
    - runner/png.c
    - runner/crc32.h
    - runner/crc32.c
    - tests/host/tests.cmake
    - tests/host/test_palette.c
    - tests/runner/test_png.c
  modified:
    - CMakeLists.txt
    - runner/main.c
    - tests/tests.cmake
    - tests/runner/tests.cmake
    - tests/testcard_pins.cmake
    - tests/inventory/ci.txt
decisions:
  - "The PNG encoder takes a packed RGB buffer and returns the whole file in memory (cogmaster__png_size, cogmaster__png_write) instead of the seed's streaming FILE writer, so the runner writes each PNG with one fwrite after its frame"
  - "A --png file that cannot be written in full is removed and the runner exits 1 with cannot write FILE"
  - "cogmaster-host is defined inside the COGMASTER_BUILD_RUNNER block; the libretro plan may widen that condition"
patterns-established:
  - "cogmaster-test-runner <case>: runner-side unit cases (png, png_file PATH), labels portable;runner"
  - "cogmaster-test-host <case>: host helper cases, labels host;portable"
requirements-completed:
  - FRAME-07
  - FRAME-02
coverage:
  - id: D1
    description: "Every one of the 4,096 RGB444 values maps to RGB888 with each channel times 0x11 and back through c >> 4; RGB565 keeps each channel's 4 bits on top, white is 0xFFFF and black 0x0000"
    requirement: FRAME-07
    verification:
      - kind: unit
        ref: "tests/host/test_palette.c#host.palette"
        status: pass
    human_judgment: false
  - id: D2
    description: "The encoder's CRC-32 and Adler-32 match published check values; a 2x2 image encodes to 82 bytes whose chunk CRCs, stored block and Adler-32 an independent parse accepts and which decode to the input; zero, oversized and one-byte-short requests are refused"
    requirement: FRAME-07
    verification:
      - kind: unit
        ref: "tests/runner/test_png.c#runner.png"
        status: pass
    human_judgment: false
  - id: D3
    description: "cogmaster-run --frames 60 --png 60:FILE writes a 256x192 depth-8 RGB PNG that decodes, pixel for pixel, to the SMS card through c >> 4, and only then matches the pinned SHA-256; an unwritable path exits 1 with cannot write"
    requirement: FRAME-07
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.png_file"
        status: pass
      - kind: other
        ref: "build/ci/cogmaster-run --frames 60 --png 60:build/ci/card.png && xxd -p -s 16 -l 10 build/ci/card.png | grep -qx 00000100000000c00802 (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "--png with no value, no N, no FILE, N = 0, N not a number, N above --frames, or N above a later --frames exits 2 with the usage text and writes nothing"
    requirement: FRAME-07
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.usage.png_syntax"
        status: pass
    human_judgment: false
  - id: D5
    description: "The runner's PNG CRC is cogmaster__runner_crc32, so no runner object defines the core's cogmaster__crc32 and the static library's CRC is never displaced"
    requirement: FRAME-02
    verification:
      - kind: other
        ref: "grep -rqw cogmaster__crc32 runner/ (exit 1) and grep -q cogmaster__runner_crc32 runner/crc32.h (exit 0)"
        status: pass
    human_judgment: false
duration: 5min
completed: 2026-10-19
status: complete
---

# Phase 1 Plan 03: PNG reference through a lossless palette Summary

**`cogmaster-run --png N:FILE` now writes frame N's crop as a stored-deflate RGB PNG. The colours come from an own palette helper in `host/` that multiplies each RGB444 channel by 0x11, so `c >> 4` gives the original back. The SMS card's PNG has a pinned SHA-256, and the test compares that digest only after an independent decoder has matched every pixel to the card.**

## Performance

- **Duration:** about 5 min
- **Started:** 2026-10-19T21:23Z
- **Completed:** 2026-10-19T21:28Z
- **Tasks:** 1
- **Files created:** 9, modified: 6

## Accomplishments

- **Palette.** `cogmaster__rgb444_to_rgb888` and `cogmaster__rgb444_to_rgb565` are own code, with no third-party table. Both widen a channel by repeating its bits. They build as the `cogmaster-host` static library, which the libretro adapter will reuse.
- **Encoder.** The seed's streaming writer became `cogmaster__png_size` plus `cogmaster__png_write`, which fill a caller buffer. The framing is unchanged: signature, IHDR, one IDAT of stored blocks of at most 65,535 bytes, then IEND. Byte-by-byte big-endian fields keep the output independent of host byte order.
- **Runner.** `--png N:FILE` can be repeated and is checked against `--frames` after all options are parsed, so option order does not matter. After frame N completes, the runner builds the whole file in memory and writes it with one `fwrite`. A failed write removes the file, prints `cogmaster-run: cannot write FILE`, and exits 1.
- **No name collision.** The runner's CRC is `cogmaster__runner_crc32`. `nm build/ci/cogmaster-run` shows only that name.
- **Pin.** `COGMASTER_TESTCARD_PNG_SHA256_SMS` is `c4d1160f…7a5460`. It was pinned only after `cogmaster-test-runner png_file` decoded the file to the card. Python's `zlib` and `sips` also read the file as 256 × 192, as a one-off check, and viewing the image showed the card.

## Task Commits

1. **Task 1: --png through an own palette helper, runner-only CRC name**: `7a18a4c` (test, RED), `cce96b6` (feat, GREEN)

## Evidence (command, exit status)

- `cmake --workflow --preset ci`: exit 0, 27 tests.
- `ctest --test-dir build/ci --no-tests=error -R '^(host\.palette|runner\.png|runner\.png_file|runner\.usage\.png_syntax)$' --output-on-failure && ctest --test-dir build/ci -R '<same>' --show-only=json-v1 | jq -e '.tests | length == 4'`: exit 0.
- `build/ci/cogmaster-run --frames 60 --png 60:build/ci/card.png && xxd -p -s 16 -l 10 build/ci/card.png | grep -qx '00000100000000c00802'`: exit 0.
- `grep -rEq 'a7800|BSD-3' host/`: exit 1.
- `grep -rqw 'cogmaster__crc32' runner/`: exit 1. `grep -q 'cogmaster__runner_crc32' runner/crc32.h`: exit 0.
- `build/ci/cogmaster-run --frames 5 --png 6:build/ci/x.png`: exit 2.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- `grep -rEn '\b(D-[0-9]{2}|FRAME-[0-9]{2})\b' runner host tests`: exit 1, nothing printed.
- `sh scripts/hygiene.sh --tree`: exit 0.
- Negative control, run once: `cmake -DCASE=png_file ... -DEXPECTED=00 -P tests/runner/tests.cmake` failed with the mismatch message.
- A one-off build outside the tree with `-fsanitize=address,undefined -fno-sanitize-recover=all` (Apple clang) passed all 27 tests: `ctest`, exit 0.

## TDD Gate Compliance

- RED `7a18a4c` came before GREEN `cce96b6`. RED was recorded as `ctest --test-dir build/ci -R '^runner\.png$' --output-junit build/ci/red-junit.xml` with exit 8. `gsd-tools check tdd-red-evidence` returned `RED_EVIDENCE_OK` (target_test_failed).
- Semantic assessment: in the RED commit the palette and encoder bodies were stubs returning 0, so everything compiled. `host.palette` failed on its channel assertions. `runner.png` failed on Adler-32 and `png_size`/`png_write` (0 where 82 was expected). `runner.png_file` failed because the runner refused the unknown `--png` option. `runner.usage.png_syntax` already passed in RED, because the old runner rejected any `--png` as an unknown option with exit 2. It pins the exit status, not new behaviour that was missing. The PNG pin was a placeholder of zeros until GREEN.

## Deviations from Plan

### Interpretations and additions

- **Encoder API.** Assumption A-03-1 held in substance, but the adaptation was larger than a row callback. The seed streamed to a `FILE *`. The plan requires building the whole file in a heap buffer, so the encoder now writes into memory and the runner does the one `fwrite`. This change stays inside `runner/png.c` and `png.h`.
- **Write failure test.** The plan names the `cannot write` behaviour but no test for it. It is checked at the end of the `runner.png_file` case.
- **Extra syntax cases.** `runner.usage.png_syntax` also covers a missing value, a missing N, an empty FILE, a non-numeric N, and N above a `--frames` given later. It also checks that a refused `--png` creates no file.
- **Test layout.** `tests/runner/tests.cmake` also runs as a `-P` script, as the seed's does. There was no separate script file in the plan's file list.
- **Docs.** The runner's usage text and the header comment in `runner/main.c` describe `--png`. No runner page exists in `docs/` yet; the help and bench plan owns that.

## Requirement status

`requirements-completed` lists FRAME-07 and FRAME-02 because this plan delivers its share of each. Neither box in `.planning/REQUIREMENTS.md` is ticked, because later plans in this phase still carry them. The RetroArch screenshot compare, the install line and the cross-platform runs come later.

## Known Stubs

None.

## Issues Encountered

None.

## Next Phase Readiness

The libretro adapter can link `cogmaster-host` for XRGB8888 and RGB565 output. The hosted RetroArch compare can use `cogmaster-run --frames 60 --png 60:FILE` as its reference, and that file's digest is `COGMASTER_TESTCARD_PNG_SHA256_SMS`.

## Self-Check: PASSED

- host/palette.h, host/palette.c, runner/png.h, runner/png.c, runner/crc32.h, runner/crc32.c, tests/host/tests.cmake, tests/host/test_palette.c and tests/runner/test_png.c exist on disk.
- Commits 7a18a4c and cce96b6 are ancestors of HEAD.
