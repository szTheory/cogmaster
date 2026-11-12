# Phase 10: BIOS Boot - Adversarial Pass

**Target:** `10-CONTEXT.md` at HEAD 202f107 on `phase/10-bios-boot`. **Method:** every cited code fact re-read; MAME (BSD-3) at
`1d7a454ed79e` and SMS Power / smstech text from the scratchpad copies; no BIOS byte and no GPL/LGPL source opened.
**Counts:** BLOCKING 3, SERIOUS 9, MINOR 14.

## Checked and held (no finding)

- The 13 SHA-1 values in ASSET_POLICY section 4 equal the 13 distinct `SHA1(...)` values in MAME `sms.cpp` at the pin (scripted, 13 of 13).
- 737 counted lines / 9,237 `src` lines / limit 923 reproduce with `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`; 737 + 110 = 847 fits.
- `vdp_reset` R2 `$0E` / R10 `$FF` equal MAME's `device_reset` (the hardware lens "all zero" claim was indeed wrong).
- `$E0` equals MAME `setup_bios` (`IO_EXPANSION|IO_CARTRIDGE|IO_CARD`); the SMS1 "cartridge wins" and SMS2 "ANDed" rows are verbatim in smstech section 4.
- `tests/api/v9.c:49`, `tests/core/test_core.c:884`, `src/internal.h:486`, `tests/core/test_state.c:843-883`, `tests/runner/tests.cmake:1087-1102`
  and `repository.sh:93` are the right shape-change sites; `tests/api/v5.c` pins only `video_mode` and is unaffected.
- State header: `H_RESERVED 53`, `H_RESERVED_SIZE 11`, each reserved byte is `fixed(io, "reserved", 0, 1)` so the loader zero-checks them
  (`state.c:136-138`); carving 53-57 is sound and `record_sweep` covers new `fixed` fields by name.
- `perf` on `workflow_dispatch` prints `perf control boot_to_play runs=H1,H2,H3 spread=N` (`ci.yml:648`), so the two-dispatch `delta_bp`
  computation is possible by hand; the PR run still applies `perf_compare.cmake` (limit +100 bp).
- The README guarded sentence and one extra sentence coexist with `readme.cmake` (it counts the exact sentence only).

---

## BLOCKING

### B1. The release scan's directory scope is undefined, and its natural registration fails on an owned fixture on every leg

- **Evidence.** Decision 8 scans "a directory recursively: every `.zip` extracted ..., plus every loose file" and applies the "firmware class
  plus 512 and a NUL in the first 4,096 bytes" rule to every file. `release.archive_contents` (the precedent this test is told to follow,
  `tests/release/tests.cmake:9`) passes `-DDIR=${PROJECT_BINARY_DIR}`, the whole build tree, where `build/<preset>/tests/seeds/size-16k-512.sms`
  lives. `tests/fuzz/seeds.c:134-137` writes it as `memset(image,0,n)`, `di; halt` at offset 512, 512 bytes of `$AA` in front: 16,384 + 512 =
  **16,896 bytes**, which is in the size class (16,896 is listed), with NULs from offset 514 on. So `release.firmware_scan` fails
  "copier-header class" on the project's own seed in the second test step of all six build legs (and `runner.probe_headerless` depends on that
  exact file, so it cannot be changed). The `clean` control passes because it uses a synthetic directory. D1 would be the first place it shows.
  Decision 8 also says the test "scans the 18 real zips"; one leg has 3 zips (`MIN_ZIPS` must be 3, not 18).
- **Fix.** Decide in CONTEXT: `-DDIR` is a staging directory holding exactly the leg's `cogmaster-*.zip` (copied by the test), the size-class rule applies
  to zip members and to `-DROOT` tracked/manifest files only, never to build-tree loose files; `MIN_ZIPS` default 1 (never 0), registration passes 3.
  Add a control: a loose 16,896-byte NUL-bearing file in a *loose-file* directory must NOT fail while the same bytes as a zip member DO.

### B2. A stale `mem_slow` breaks an existing, unlisted test, and the "exact" change table plus plan 4's 10-file count are wrong

- **Evidence.** `tests/core/test_core.c:1514` does `m->io.mem_ctrl = (uint8_t)(m->io.mem_ctrl | 0x50u);` and then asserts `cogmaster_peek` at `$0400` and
  `$C000` is `$FF` (`:1515-1516`). Decision 2 makes `cogmaster_peek` share `mem_slow_driven(m, a, &v)` with `mem_read_slow` and makes `mem_slow` a derived
  flag recomputed only by `bus_decode`. If the shared decoder consults the flag (the stated design), this test's direct write leaves it stale, peek
  returns driven data and `core.peek` (in `ci`) goes red in plan 4. `tests/core/test_core.c` is not in plan 4's list or in the "exactly this table";
  adding it makes plan 4 11 files (instance of the Phase 9 class: a test that models what a plan changes landing in another plan or past the cap).
  `tests/ports/test_glasses.c:305` also writes `mem_ctrl` directly but only calls `bus_write`, which the plan leaves flag-free, so it is safe.
- **Fix.** Say in decision 2: only `mem_read` (the hot path) tests `mem_slow`; `mem_slow_driven` and `cogmaster_peek` decode from `mem_ctrl` and `bios.size`
  directly (peek is cold). Then no existing test changes, `bios.slow_flag` still compares the flag, and the table stays exact. State this and add
  `tests/core/test_core.c:1514` to the "unchanged and must stay so" list with that reason. Also note the flag has only two of four recompute sites in plan 4
  (`io_out`, `io_reset`); `boot_power_on` and `state_load` arrive in plan 5, so `bios.slow_flag`'s "load / state load" legs must be added in plan 5, not 4.

### B3. Fuzz extension is self-contradictory and re-indexes every existing seed

- **Evidence.** Decision 10 says "`fuzz_run` and `fuzz_loader` are EXTENDED"; plan 5 says "the `fuzz_loader` size sweep is a seed-set addition in `seeds.c`, not an
  edit of `fuzz_loader.c`". `tests/fuzz/fuzz_loader.c:36-53` never sets `media.bios`, so no seed can send a BIOS size through it; it also `abort()`s when
  probe and load disagree, which a BIOS-caused refusal would trigger. For `fuzz_run` the input today IS the cartridge image (`fuzz_run.c:29-46`: probe,
  `data[0] & 1` picks PAL). "First byte selects the mode, next bytes are the BIOS" shifts every committed/generated seed by one byte and by the BIOS length:
  `size-1024.sms` becomes 1,023 (refused), every `TMR SEGA` header leaves its `$7FF0/$3FF0/$1FF0` offset, so the Phase 1-9 seeds silently stop covering what they
  were written for while `fuzz.regress.*` stays green; the nightly libFuzzer corpus is a fresh copy of the same seeds.
- **Fix.** Keep the cartridge layout unchanged and put the mode/BIOS selector at the tail, or in a new third harness entry whose inventory line is added
  (inventory cost is one line in `fuzz.txt`/`ci.txt`/`wasm32.txt`). Edit `fuzz_loader.c` (probe-equals-load only when no BIOS is attached) or drop its BIOS claim.
  Recount plan 5 (adding `fuzz_loader.c` = 11 files: split the fuzz work into a plan 5b or move `test_state.c` with the state work).

---

## SERIOUS

### S1. ROADMAP success criterion 1 is not met as designed, and is not on the amendment list

`ROADMAP.md:185`: "The release firmware scan and the local firmware-window scan **run in `ci`**". Decision 9 puts `local.firmware_window_scan` in `library` only and
"never in `ci`" (ASSET_POLICY section 4); only the controls run in `ci`. Plan 10 amends criteria 2 and 4 but not 1, so the verifier reads SC1 literally.
**Fix:** add to plan 10 (and REQUIREMENTS BIOS-01): "the window scan's negative controls run in `ci`; the scan itself runs in the local `library` preset and in the
pre-commit and pre-push hooks".

### S2. The seed's notice lands nine plans after the seed

Plan 1 commits `host/bios_table.inc` (facts from BSD-3 `sms.cpp`); the THIRD-PARTY-NOTICES section "MAME BIOS digests" is plan 10. AGENTS rule 1: reuse "with its
notice kept as PROVENANCE.md describes". The `notices` check will not catch it (it scans `*.c *.h` only, `repository.sh:135`). **Fix:** move the notices section
(and the `Applies to:` line, which the check existence-tests) into plan 1 (9 files, room for one).

### S3. A real BIOS probing empty slots will record first-fault noise, hiding the game's first fault

Port3E (SMS Power): "The BIOS runs code from RAM which disables the BIOS ROM and enables the three other slots in turn (card, cartridge, expansion)"; smstech
section 6: it "checks the cartridge, card, and expansion slots". Decision 2 leaves "cartridge off, BIOS off, card or expansion on" as undriven: `mem_read` records
`UNMAPPED_READ`, bumps `open_bus_reads`, and `cogmaster__fault_record` keeps the first fault for the whole run (`src/fault.c:7`, "the first fault wins"). So on a
real dump `--trace first-fault` (`runner/main.c:506`) stops inside the BIOS and any game fault is masked; hostile images are the only thing the plan tests.
**Fix:** decide it. Either an enabled-but-empty card/expansion slot (SMS1: b5 or b7 clear) reads open bus without a fault, or document that fault
reporting is meaningless before the hand-off in a BIOS run. Add a row to `bios.arbitration`. Needs one sentence of evidence (Port3E text above).

### S4. The ordering evidence for BIOS-01 is vacuous at plans 1 and 2 and unverifiable after the squash

`git merge-base --is-ancestor <scan commit> <first commit touching src ...>` cannot be run in plan 1 or 2 (no later commit exists), and the SHAs vanish at the squash
merge (they survive only as `refs/pull/N/head`; AGENTS rule 12 wants public pins). The pattern `host/bios.*` is also ambiguous against plan 1's own
`host/bios_table.inc`. **Fix:** plans 1 and 2 record only "`git diff --name-only origin/main...HEAD -- include src runner libretro` is empty" (exit status of `test -z`);
plan 3's first task records the real `is-ancestor` over `origin/main..HEAD` with commit subjects, not SHAs; name the glob `host/bios.[ch]`.

### S5. Evidence commands that report the wrong exit status

"`git diff origin/main -- tests/built/expected.txt ... | grep -c '^-[^-]'` prints 0": `grep -c` exits **1** when the count is 0, so the recorded "exit status" would read as
failure. The unchanged-proof also diffs against a moving `origin/main`. **Fix:** `! git diff ... | grep -q '^-[^-]'` (exit 0 = none removed) and `git diff --quiet $(git merge-base origin/main HEAD) -- ...`.

### S6. The release scan's NUL test, as the text describes it, is likely to be a misaligned hex search

`file(READ ... LIMIT 4096 HEX)` then "a NUL occurs" invites `string(FIND "${hex}" "00")`, which matches across byte boundaries (`20 0a` -> `200a`, `a0 0b`). The `text`
control "passes" only because it is "newline-free ASCII" (decision 8), which hides exactly this bug; a normal text file of class size would fail. **Fix:** require an aligned
match (`^([0-9a-f][0-9a-f])*00`) and make the `text` control contain newlines and tabs.

### S7. Window-scan controls run on every `ci` leg, not "Linux only"

Decision 9's risk list and decision 8 say the `od`/`awk` scan "runs locally and in the Linux policy lane only", but the controls are registered by
`firmware_cases.cmake` under `if(NOT COGMASTER_HYGIENE)`, and `tests/inventory/ci.txt` is one list for `ci`, `dev` and `ci-msvc` (plus `wasm32.txt`, asan). They therefore
run on macOS (BSD `od`, BWK awk), Windows Git Bash and the wasm32 host, with policy timeouts of 10-30 s. **Fix:** either register them in the `hygiene` preset (Linux only,
no build, devops lens 5.3) and keep them out of the shared inventories, or state the portability argument and let D1 prove it; correct the sentence.

### S8. Hot-path claim "zero Ir added" is plausible but unproved, and the perf procedure is not a gate

x86 `test byte [m+off], 0x50` and `cmp byte [m+off], 0` are one instruction each, so the count should hold; but the struct gains 8 KiB before `ram`/`vdp`, moving offsets, and
the only gate is a manual comparison of two dispatch lines that the PR run later repeats. **Fix:** record both `runs=` lines and the computed `delta_bp` in the plan 5
SUMMARY and add "if `delta_bp > 100`, move `bios` and `mem_slow` to the end of `struct cogmaster`" as the stated remedy; place `mem_slow` next to `io` regardless.

### S9. Budget honesty of `firmware_cases.cmake` and tight line allowances

Precedents classified `behavior` are 32 and 79 lines (`repository-checks.txt:34-35`); this file holds about 30 control registrations with fixture builders (~200+ lines of CMake)
that are check code by any honest reading and would break the one-tenth rule if counted (847 + 200 > 923). Also inconsistent: hygiene.sh "at most 14 (`byte_run` 8, hook 6)"
vs "stream ... at most 8 counted lines" for the same hook (16); 38 lines for `firmware_scan.cmake` and 40 for `firmware-window.sh` with the mandated "Fault prevented"
comments are tight. **Fix:** keep the classification but state the size cap (for example 120 lines) and that it holds no matching logic; reconcile 6 vs 8; admit the
spare 78 lines are the real margin.

---

## MINOR

- **M1.** `src/cartdb.inc` has **1,660** `.sha1` rows (grep), not "1,680" (1,680 includes 20 slot rows); ASSET_POLICY's CC0 lists total 1,690 entries, so the scan's
  "any ROM SHA-1 in MAME's CC0 lists" is a subset. State that in the rewritten section 5.
- **M2.** Validation order in decision 5 lists "resolved machine is a Game Gear -> `ERR_PROFILE_REJECTED`" before "every existing check", but the machine is only resolved after
  `cogmaster__cart_probe` (`instance.c:142-163`); say the BIOS-vs-GG check follows machine resolution and an unprobeable cartridge keeps its own status.
- **M3.** `docs/embedding.md:148` ("`cogmaster_media` is 56 bytes (32 on 32-bit, `fm` at 28)") and the version-history paragraph (lines 43-60) need edits; they are not in the
  600-byte allowance. Sum with the fault row and `:223` clause is about 1.1 KB: 36,636 of the 36,864 target. Plan 10 should budget 1.2 KB or trim.
- **M4.** `docs/bios.md` (new, plan 9) carries the arbitration table, yet "arbitration prose lands in plan 4" (Process). Plan 4 touches only SMS-SYSTEM and one embedding row.
  Fix the sentence; do not cite DEC.43 in user docs (planning ID).
- **M5.** `exit_status.h` / `docs/runner.md:386` say exit 1 = "a file could not be read"; a missing `--bios` path becomes 77. Amend the `failure` meaning too, not only `refused`, and say a missing cartridge is still 1.
- **M6.** The Game Gear refusal text "a Game Gear BIOS (planned)" misleads when the user gave an SMS BIOS with a `.gg` cartridge (auto machine resolves to GG); say "a BIOS cannot start a Game Gear cartridge".
- **M7.** AUTO machine resolves to SMS2, but every runnable 8 KiB image in the table is SMS1-class (MAME lists them under the SMS1/Japanese drivers); `docs/bios.md` should say `--machine sms1` and that the SMS2 VDP differs.
- **M8.** A European user with only `bios_E.sms` and the default (NTSC) TV gets fast boot plus one warning; no E/U fallback was kept. Say so in `docs/bios.md` and the option text ("PAL needs TV = PAL").
- **M9.** `bios.equivalence`: the image's stack traffic (`push/pop` for AF) and the RAM stub must live inside the excluded `$C100-$C1FF` window or RAM equality fails at `$DFEE-$DFF1`; the excluded list also needs `vdp_addr`, `vdp_code`, `vdp_buffer` (guest-visible, not "time bookkeeping"), so "excluded = beam/time only" is inaccurate. And stepping raw `z80_step` past two frame deadlines leaves `bus.deadline` behind `ticks`; confirm `cogmaster__state_record` accepts it or step with `run_frame`.
- **M10.** `bios.hostile`: "ticks advance by exactly one frame" is wrong by the overshoot (up to 68); use `overshoot <= 68` and `frame` advanced by 1.
- **M11.** `byte_run` runs in `--history` over `git grep -h` output that concatenates files with no boundary, so a literal run can span two files; reset the counter on any line that is not literal-only and note decimal and non-hex forms pass (the tree's own tables are decimal).
- **M12.** Nested archives inside a release zip are not re-extracted; the only guard is `release.archive_contents` pinning the member set. Say so in `docs/testing.md`. A padded (16/32 KiB) dump of the same BIOS passes the release scan (plain-size files are outside it) and is caught only by the local window scan.
- **M13.** `policy.bios_dir_reads` as described fails on its own implementation (`repository.sh` and `firmware_cases.cmake` contain the string); build the name in pieces as `policy.no_skip` does. `MIN_ZIPS` must not default to 0.
- **M14.** Plan 2's list reads 11 files if `repository.sh` and `bios_dir_reads` are counted (the text excludes it only by parenthesis); a FIFO `--bios` path blocks in `open` (same as a cartridge today), so "cannot cost more than 8,193 bytes" is about bytes, not time.

---

## Lens items the synthesis dropped (and whether it matters)

- Legal D8 P2 (the same window test over a battery save): dropped silently; harmless (a BIOS never writes the cartridge's save), but say so.
- Devops 5.3 (scan controls in the cheap `hygiene` lane, 42 s fail): dropped; see S7.
- Hardware "do not wire the Reset button to the CPU / `$0066` is the BIOS handler until the cartridge is on": not recorded; add one line to `docs/bios.md`.
- Product Q3 (`bios.sms` as libretro's first-class name): DEC.62 wins; recorded. Product E/U fallback: dropped on purpose (M8).
