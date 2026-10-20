---
phase: 02-the-cpu-matches-the-public-vectors
plan: 15
subsystem: fixtures
status: complete
tags:
  - fixtures
  - mailbox
  - io
  - ports
  - region
requires:
  - "02-08: VDP ports $7E/$7F/$BE/$BF and the Pause NMI"
  - "02-10: io.c, $3E/$3F/$DC/$DD per profile, GG $00-$06"
  - "02-13: mailbox.inc, cogmaster-run --mailbox, the CSV judge"
  - "02-14: the optional MACHINE variable of the mailbox case"
provides:
  - "tests/roms/src/ports.asm: 32 KiB, records 01-0C under every SMS profile; 0D-10 when FIXTURE_GG is defined"
  - "tests/roms/src/ports_gg.asm: the same source under a Game Gear export header (region code 6)"
  - "header.inc: FIXTURE_GG selects region code 6; other images unchanged (hello digest unchanged)"
  - "Mailbox case of tests/fixtures/tests.cmake takes optional REGION (--region) and ROM (the CSV rows' rom name)"
  - "fixtures.ports.sms-export, .sms-japan, .sms-pal, .gg (labels fixtures and portable)"
  - "31 ports rows in tests/diag/expected.csv"
  - "docs/conformance.md: the ports fixture section"
  - "ARCHITECTURE sections 3 and 11 and the CONFORMANCE section 2 matrix place ports in phase 2"
affects:
  - "Plan 17: measured timeouts for the four ports tests along with the other phase tests"
  - "Phase 3: the H-counter latch, the Pause NMI counter and the Pause movies join the ports fixture once --input exists"
actuals:
  tokens: 5900
  tasks: 2
  commits: 2
commits: 2
plan_head_before: a3d2e900e1d0b16e93d55ed15aecae646c4a9764
plan_head_after: 65c266de1d76e42ca66f5f8be0909d08496ade43
tech-stack:
  added: []
  patterns:
    - "A Game Gear image is the SMS source assembled again with FIXTURE_GG defined, judged against the same rom's rows under profile gg"
    - "One fixture test per profile from a foreach over profile names, each passing --machine and --region"
key-files:
  created:
    - tests/roms/src/ports.asm
    - tests/roms/src/ports.link
    - tests/roms/src/ports_gg.asm
    - tests/roms/src/ports_gg.link
  modified:
    - tests/roms/src/header.inc
    - tests/fixtures/tests.cmake
    - tests/built/expected.txt
    - tests/diag/expected.csv
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md
    - .planning/preparation/ARCHITECTURE.md
    - .planning/preparation/CONFORMANCE.md
decisions:
  - "tv_loop rows span the documented approximate count to the count the frame length implies, plus one loop for the poll phase: NTSC $0783-$0788 (1,923-1,928), PAL $08EF-$08FF (2,287-2,303); the core measures 1,927 and 2,301"
  - "The Game Gear runs every SMS record too; where it differs (region echo $C0C0 with no TH lines, work RAM staying on because $3E is not decoded) it has its own row"
  - "The H counter is judged as the step between two $7F reads 15 cycles apart ($0B-$0C), not as an absolute beam position"
  - "VRAM is written and read back with the display off (R1 = 0), so no access-slot timing applies"
requirements-completed:
  - CPU-04
coverage:
  - id: D1
    description: "fixtures.ports.sms-export: the region echo reads $C0 then $00 and every other record matches its row"
    requirement: CPU-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.ports\\.sms-export$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "fixtures.ports.sms-japan: the region echo reads $00 both times; NTSC tv_loop and 262 lines"
    requirement: CPU-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.ports\\.sms-japan$'"
        status: pass
    human_judgment: false
  - id: D3
    description: "fixtures.ports.sms-pal: tv_loop in the PAL range, the 2,048 verdict 1, and 313 V counter lines"
    requirement: CPU-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.ports\\.sms-pal$'"
        status: pass
    human_judgment: false
  - id: D4
    description: "fixtures.ports.gg: $00 reads $C0, $01-$06 read $7F $FF $00 $FF $00 $FF, and work RAM stays on through a $3E write"
    requirement: CPU-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.ports\\.gg$'"
        status: pass
    human_judgment: false
  - id: D5
    description: "Exactly four ports tests are registered, the junit check finds every inventory test ran once, and the fixtures label runs 11 tests"
    requirement: CPU-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.ports\\.' --show-only=json-v1 | jq -e '.tests | length == 4'"
        status: pass
      - kind: integration
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
    human_judgment: false
  - id: D6
    description: "No H-counter latch record exists in the fixture or the CSV"
    requirement: CPU-04
    verification:
      - kind: other
        ref: "grep -Eic '(h[-_ ]?counter|th)[-_ ]latch' tests/roms/src/ports.asm tests/diag/expected.csv"
        status: pass
    human_judgment: false
  - id: D7
    description: "Every CI lane builds and passes with the two new images, including be32, wasm32 and hash-equality"
    requirement: CPU-04
    verification:
      - kind: integration
        ref: "gh run view 39373821097 --json conclusion --jq .conclusion"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 15: The ports fixture under four profiles Summary

The `ports` fixture reads `$3E`, `$3F`, `$7E`, `$7F`, `$BE`, `$BF`, `$DC`, `$DD` and, on the Game Gear,
`$00`-`$06`, and reports through the RAM mailbox. Four tests judge it against `expected.csv`, one per profile:
`sms-export`, `sms-japan`, `sms-pal` and `gg`. The records cover the region echo, the World Soccer TV loop with its
2,048 verdict, and the line count from the V counter.

## What was built

- **Task 1 (tracer), `5e1e2a2`:** `ports.asm` with the region-echo record, `fixtures.ports.sms-export`, its CSV row
  and digest. The mailbox case gained optional `REGION` and `ROM`. Tracer gate: the verify command was re-run and
  passed, then the remaining work started.
- **Task 2, `65c266d`:** records 02-0C, plus 0D-10 for the Game Gear build. `ports_gg.asm` assembles the same source
  with `FIXTURE_GG`, and `header.inc` then writes region code 6. The other three profile tests and 30 more CSV rows
  were added, so the `ports` rows total 31. Both inventories were updated. `docs/conformance.md` now has a ports
  section, and the ARCHITECTURE and CONFORMANCE matrices put `ports` in phase 2.

Measured values, all inside their rows: region echo `00C0` (export, PAL), `0000` (Japan), `C0C0` (GG). `tv_loop`
is 1,927 on NTSC and 2,301 on PAL, with verdicts 0 and 1. `vc_lines` is 262 and 313, and the H step is 11.

Run once by the agent: `cmake --workflow --preset ci` exit 0, `cmake --workflow --preset hygiene` exit 0, and
`ctest --test-dir build/ci -L fixtures` 11 of 11 passed. CI run https://github.com/szTheory/cogmaster/actions/runs/39373821097
concluded success.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] header.inc needed a Game Gear region code**
- **Found during:** Task 2
- **Issue:** `header.inc` hard-codes region code 4, but the plan wants the GG image's header to agree with the machine.
- **Fix:** `.IFDEF FIXTURE_GG` selects region code 6. All other images assemble to the same bytes, and the `hello`
  digest is unchanged.
- **Files modified:** tests/roms/src/header.inc
- **Commit:** 65c266d

**2. [Rule 3 - Blocking] The mailbox case could not pass a region or judge one image under another rom's rows**
- **Found during:** Task 1 (REGION) and Task 2 (ROM)
- **Fix:** optional `-DREGION` adds `--region`, and optional `-DROM` overrides the CSV rom name, so `ports_gg` is
  judged against the `ports` rows with profile `gg`.
- **Files modified:** tests/fixtures/tests.cmake
- **Commits:** 5e1e2a2, 65c266d

**3. [Rule 1 - Plan acceptance text] CSV test column carries the record id**
- **Issue:** the acceptance greps `^ports,region_echo,` and `^ports,(tv_loop|vc_lines),sms-pal,` do not include
  the `<id>:` prefix. Plan 13 made that prefix the judge's way of mapping rows to records.
- **Fix:** kept the established format. The equivalent checks
  `grep -Eq '^ports,[0-9A-F]{2}:region_echo,sms-export,'` exits 0 and
  `grep -Ec '^ports,[0-9A-F]{2}:(tv_loop|vc_lines),sms-pal,'` prints 2.

**4. [Rule 1 - Range] tv_loop ranges are not strictly centred on 1,924 and 2,288**
- **Issue:** SMS-SYSTEM section 5 gives approximate counts (≈ 1,924 and ≈ 2,288). The frame lengths in section 1
  (59,736 and 71,364 cycles) divided by 31 give 1,927 and 2,302. A ±1 window centred on the approximate values would
  reject a correct core.
- **Fix:** each range runs from the documented approximation to the frame-length count, with one extra loop at each
  end for the poll phase. Both ranges stay far from 2,048 and from each other.

## Known Stubs

None.

## Self-Check: PASSED
