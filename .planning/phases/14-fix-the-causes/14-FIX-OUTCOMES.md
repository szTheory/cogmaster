# Phase 14 fix outcomes

Counts only.

- core commit: a4ad03abfdad612304b2953a084767e38926ca9f (formatting and one comment only since 4e7f825f6cbe2a91c3e6a543eacae7ecb044f729, the last commit that changed core behaviour; the final diagnose ran at head 230cbc9, whose core behaviour equals it)
- baseline core commit: faf9cc788dbfea6973234e50c2861f157c763af7
- behaviour revision: 18
- date (UTC): 2026-11-13
- command: `sh scripts/library.sh diagnose`, exit status 0 (the baseline run and the final run)
- ledger read: tests/compat/ledger.csv at commit 032ebabc340b56b1123bb8745cc91d8c7a635189
- caps: verdict at frame 600, view to frame 1800
- phase 13 note: .planning/phases/13-why-carts-fail/13-CAUSE-GROUPS.md at commit 17375f726b7452e6fd3d9049a42915574602f7cc

The baseline run was made on a clean tree at the baseline core commit above, with the mode nibble in the fault record and before any benign fault kind was dropped. Its totals: run_fail 205 and run_fail_not_in_ledger 0 (compared as numbers), the same 205 failing rows as the Phase 13 note.

## Decision table

One line per raise. A source is a document section, never a row count. "Benign by hardware" means the sourced documents describe no fault for the event; where the only support is the absence of a documented fault, the line says it is an inference.

| kind and site | decision | source |
| ------------- | -------- | ------ |
| `mapper_range`, a write to `$FFFC` with bit 7, bit 4 or bits 1-0 set | benign by hardware, selected model | SMS-CARTRIDGES 2.1 (the register table: no software uses these bits); SMS-SYSTEM section 3 |
| `mapper_range`, a bank number past the end of a non-power-of-two image | kept as the core's marker; the bank still wraps | DEC.37; SMS-CARTRIDGES 2.4 (over-range is harmless on power-of-two parts and names nothing a fault) |
| `rom_write` to an address that is not a mapper register | benign by hardware | SMS-CARTRIDGES 2.1 and 2.2 (the tables name exact registers only; no document lists a fault) |
| `unmapped_write` to a port nothing decodes | benign by hardware, an inference | SMS-SYSTEM section 4 (the mirrors do not exist on full-decode machines; the write's effect is undocumented, so this is the absence of any documented fault) |
| `unmapped_read` of `$00`-`$3F` | benign by hardware | SMS-SYSTEM C5 and section 4 (open bus or `$FF`); docs/embedding.md (shipped-game behaviour is never a fault) |
| `unsupported_video_mode` with M4 = 0 | emulation gap, not emulated by design | SMS-VDP section 7; DEC.03 |
| `unsupported_video_mode` with M4 = 1 | decode question | SMS-VDP section 2.5 |
| `rom_write` to a foreign board's exact register | D-04 trigger threshold: 5 rows in one reg-* class of ranks 3 and 7 (assumption; the owner can change the figure) | SMS-CARTRIDGES 2.2 (the exact register addresses) |
| memory-side `unmapped_read` and `unmapped_write`, and `ed_undefined` | genuine fault, unchanged | DEC.39; DEC.40 |

## Baseline classes

Counts of failing rows by class, per leaf, as printed by the reducer: `leaf | class label=key | rows`. Keys join their parts with a colon; the pasted lines are the reducer's own output for the labels faddr, vdpmode, wclass, pclass and size.

| leaf | class | rows |
| ---- | ----- | ---- |
| fault_alive.mapper_range.mapper | faddr=fffc:y | 109 |
| fault_alive.mapper_range.mapper | size=le128k:none:sega | 4 |
| fault_alive.mapper_range.mapper | size=le128k:valid:sega | 13 |
| fault_alive.mapper_range.mapper | size=le256k:bad:sega | 4 |
| fault_alive.mapper_range.mapper | size=le256k:valid:sega | 66 |
| fault_alive.mapper_range.mapper | size=le512k:none:sega | 1 |
| fault_alive.mapper_range.mapper | size=le512k:valid:sega | 20 |
| fault_alive.mapper_range.mapper | size=le64k:valid:sega | 1 |
| fault_static.unsupported_video_mode.vdp | vdpmode=graphics1:sms:korean | 2 |
| fault_static.unsupported_video_mode.vdp | vdpmode=graphics1:sms:korean_nb | 1 |
| fault_static.unsupported_video_mode.vdp | vdpmode=graphics1:sms:none | 3 |
| fault_static.unsupported_video_mode.vdp | vdpmode=graphics1:sms:zemina | 1 |
| fault_static.unsupported_video_mode.vdp | vdpmode=graphics2:sms:korean_nb | 2 |
| fault_static.unsupported_video_mode.vdp | vdpmode=graphics2:sms:nemesis | 1 |
| fault_static.unsupported_video_mode.vdp | vdpmode=graphics2:sms:none | 17 |
| fault_static.unsupported_video_mode.vdp | vdpmode=graphics2:sms:sega | 3 |
| fault_static.unsupported_video_mode.vdp | vdpmode=graphics2:sms:zemina | 6 |
| fault_static.unsupported_video_mode.vdp | size=le128k:none:nemesis | 1 |
| fault_static.unsupported_video_mode.vdp | size=le128k:none:sega | 1 |
| fault_static.unsupported_video_mode.vdp | size=le128k:none:zemina | 6 |
| fault_static.unsupported_video_mode.vdp | size=le128k:valid:zemina | 1 |
| fault_static.unsupported_video_mode.vdp | size=le32k:none:korean_nb | 2 |
| fault_static.unsupported_video_mode.vdp | size=le32k:none:none | 20 |
| fault_static.unsupported_video_mode.vdp | size=le48k:bad:korean | 1 |
| fault_static.unsupported_video_mode.vdp | size=le48k:none:korean | 1 |
| fault_static.unsupported_video_mode.vdp | size=le48k:none:korean_nb | 1 |
| fault_static.unsupported_video_mode.vdp | size=le64k:none:sega | 2 |
| fault_alive.rom_write.cart | wclass=lt100:sega | 8 |
| fault_alive.rom_write.cart | wclass=reg-0:sega | 8 |
| fault_alive.rom_write.cart | wclass=reg-cm:korean | 2 |
| fault_alive.rom_write.cart | wclass=reg-cm:sega | 2 |
| fault_alive.rom_write.cart | wclass=reg-zm:sega | 3 |
| fault_alive.rom_write.cart | wclass=rom:none | 1 |
| fault_alive.rom_write.cart | wclass=rom:sega | 4 |
| fault_alive.rom_write.cart | wclass=slot2:sega | 4 |
| fault_alive.rom_write.cart | size=gt512k:none:korean | 2 |
| fault_alive.rom_write.cart | size=le128k:bad:sega | 1 |
| fault_alive.rom_write.cart | size=le256k:bad:sega | 4 |
| fault_alive.rom_write.cart | size=le256k:valid:sega | 19 |
| fault_alive.rom_write.cart | size=le32k:valid:none | 1 |
| fault_alive.rom_write.cart | size=le512k:valid:sega | 5 |
| fault_alive.unmapped_write.none | pclass=3e-3f:gg:export | 8 |
| fault_alive.unmapped_write.none | size=le128k:bad:sega | 1 |
| fault_alive.unmapped_write.none | size=le128k:none:sega | 1 |
| fault_alive.unmapped_write.none | size=le256k:bad:sega | 4 |
| fault_alive.unmapped_write.none | size=le512k:bad:sega | 2 |
| fault_alive.unmapped_read.ctrl | pclass=00-06:sms:export | 6 |
| fault_alive.unmapped_read.ctrl | pclass=lt40:gg:export | 1 |
| fault_alive.unmapped_read.ctrl | size=le256k:valid:sega | 2 |
| fault_alive.unmapped_read.ctrl | size=le512k:valid:sega | 4 |
| fault_alive.unmapped_read.ctrl | size=le64k:none:sega | 1 |
| fault_alive.unsupported_video_mode.vdp | vdpmode=graphics1:sms:sega | 6 |
| fault_alive.unsupported_video_mode.vdp | size=le128k:valid:sega | 1 |
| fault_alive.unsupported_video_mode.vdp | size=le256k:valid:sega | 5 |
| fault_static.rom_write.cart | wclass=lt100:zemina | 1 |
| fault_static.rom_write.cart | wclass=reg-0:korean_nb | 2 |
| fault_static.rom_write.cart | wclass=reg-0:none | 1 |
| fault_static.rom_write.cart | size=le128k:none:zemina | 1 |
| fault_static.rom_write.cart | size=le32k:none:korean_nb | 2 |
| fault_static.rom_write.cart | size=le32k:none:none | 1 |
| fault_no_display.mapper_range.mapper | faddr=fffc:y | 1 |
| fault_no_display.mapper_range.mapper | size=le256k:valid:sega | 1 |
| dark_no_fault.never_draws | size=le256k:valid:sega | 1 |
| static_no_fault.frozen | size=le256k:valid:sega | 1 |

## Outcomes

| rank | leaf | rows (13 note) | outcome | evidence | residual leaf counts |
| ---- | ---- | -------------- | ------- | -------- | ------------------- |
| 1 | fault_alive.mapper_range.mapper | 109 | fixed in the core | D-05 stop not met: faddr=fffc:y 109, faddr=fffd-f:n 0, faddr=fffd-f:y 0 in the baseline (13 note rank 1); the `$FFFC` raise is gone and the value is stored. After: run_fail 205 to 104, run_fail_not_in_ledger 0, ledger_fail_now_pass 101. D-06: trace not taken (it needs the dump directory path, which this run did not read), shift left as it is | before 109, after 0 (the leaf no longer prints); residual faddr=fffd-f:* rows 0 |
| 2 | fault_static.unsupported_video_mode.vdp | 36 | closed: not emulated by design | class: not emulated by design. Source: SMS-VDP section 7 (the TMS9918 legacy modes 0-3 are a separate renderer); DEC.03 (the core renders mode 4 only); SG-1000 and SC-3000 out of scope in REQUIREMENTS. Final `vdpmode` sub-rows at the phase head, all on machine sms: graphics1 7 (korean 2, korean_nb 1, none 3, zemina 1) and graphics2 33 (korean_nb 4, nemesis 1, none 18, sega 3, zemina 7), 40 of 40; M4=1 classes (mode4, invalid, sms2only) 0 rows, so no decode question. Why `fail` is honest: in those modes the core draws only the backdrop, so the picture the boots check sees is not the game's, and a pass would claim a rendering the core does not do. Reopens when SEED-006 (the legacy modes) is promoted, or when a row of this leaf shows an M4=1 `vdpmode` class (D-07) | before 36, after 40 (4 rows of ranks 3 and 7 landed here on their next fault, plan 05; the leaf is unchanged by plans 06 and 07) |
| 3 | fault_alive.rom_write.cart | 32 | fixed in the core; open: D-04 needs new evidence | baseline wclass split, rank 3: lt100:sega 8, reg-0:sega 8, reg-cm:korean 2, reg-cm:sega 2, reg-zm:sega 3, rom:none 1, rom:sega 4, slot2:sega 4. D-04 trigger fired on reg-0 (11 rows of ranks 3 and 7 together, baseline) and plan 04 found no shared header property: board choice needs new evidence (all 36 rows source db), so the board question stays open for plan 08's gap round. The drop D-04 decides is landed: a write to a cartridge address that is no register and no enabled cartridge RAM records no fault on every board (Phase 13 note rank 3). Re-run: the leaf no longer prints, run_fail_not_in_ledger 0 | before 32 (36 after plan 03), after 0 (the leaf no longer prints) |
| 4 | fault_alive.unmapped_write.none | 8 | fixed in the core | D-11 trigger not met: the baseline pclass split of rank 4 is 3e-3f:gg:export 8, and no pclass sub-row has machine sms or sms1 with region japan (0 rows), so the write half is dropped (inference: no document lists a fault for a write to a port nothing decodes, SMS-SYSTEM section 4). A write to a port nothing decodes records no fault; memory-side unmapped writes still fault. Re-run: the leaf no longer prints, run_fail_not_in_ledger 0 | before 8 (11 after plan 03, 12 after plan 05), after 0 (the leaf no longer prints) |
| 5 | fault_alive.unmapped_read.ctrl | 7 | fixed in the core | baseline pclass split, rank 5: 00-06:sms:export 6, lt40:gg:export 1. A read of an undriven port in $00-$3F returns open bus, still counts in the open-bus counter and records no fault (SMS-SYSTEM C5 and section 4). Re-run: the leaf no longer prints, run_fail_not_in_ledger 0 | before 7 (8 after plan 03), after 0 (the leaf no longer prints) |
| 6 | fault_alive.unsupported_video_mode.vdp | 6 | closed: not emulated by design | class: not emulated by design. Source: SMS-VDP section 7 and DEC.03 (the same legacy-mode reason as rank 2; plan 04's D-07 gate found no M4=1 row); D-08 (the restart-transient shape needs 5 or more `order=after` rows). Counts at the phase head: `order=before` 6, `order=after` 0, `order=nopicture` 0; `vdpmode` graphics1 6 of 6 on machine sms (M4=0), size classes le128k:valid:sega 1 and le256k:valid:sega 5, 5 of the 6 at one program counter. Every raise came before the first non-blank picture, so none is a supported mode-4 picture followed by a transient. Why `fail` is honest: the first fault inside 600 frames is the boots check's own fail condition, and the raise marks a legacy mode the core does not render. Reopens when SEED-006 is promoted, or when 5 or more rows of this leaf show `order=after` (D-08) | before 6, after 6 (no code change in plans 04 to 07; the final diagnose at head 230cbc9 prints the same 6) |
| 7 | fault_static.rom_write.cart | 4 | fixed in the core | baseline wclass split, rank 7: lt100:zemina 1, reg-0:korean_nb 2, reg-0:none 1; counted in the reg-0 trigger above, whose board-choice part is rank 3's open item (all 4 rows source db). Same drop as rank 3 (Phase 13 note rank 7). Re-run: the leaf no longer prints, run_fail_not_in_ledger 0 | before 4, after 0 (the leaf no longer prints) |
| 8 | fault_no_display.mapper_range.mapper | 1 | fixed in the core | inherits rank 1: faddr=fffc:y 1 in the baseline (13 note rank 8); the leaf no longer prints in the re-run | before 1, after 0 |
| 9 | dark_no_fault.never_draws | 1 | closed: no evidence | class: no evidence. Source: Phase 13 note "For Phase 14" (the group never_draws keeps failing: a cartridge that never draws is a failure whatever its size); D-09 and D-14 (a `dark_no_fault` leaf keeps failing). Counts at the phase head: 2 rows, size le256k:valid:sega 2 (both source db); the leaf grew from 1 to 2 in plan 03, consistent with one row moving here from rank 1 (the per-cart link was not traced). Why `fail` is honest: the displayed frame is blank through 1800 frames with no fault, so the never-draws seed's reject holds. Reopens when a trace of one row names a documented hardware behaviour the core lacks (no document or trace names one now) | before 1, after 2 (under 5 rows, reported as printed) |
| 10 | static_no_fault.frozen | 1 | closed: no evidence | class: no evidence. Source: D-09 (a no-fault static row is closed by its own `distinct1800`) and D-14; the static-display seed is the intended reject for a picture that never changes. Counts at the phase head: 1 row, `distinct600` 1 and `distinct1800` 1, size le256k:valid:sega 1 (source db), no display fault. Why `fail` is honest: the displayed picture does not change in 1800 frames, so `boots@600` rejecting it is the predicate doing its job. The one pad-press run of D-14 was not taken: it needs the row's cart found among the dumps, which prints file names the agent may not read, so the closure rests on `distinct1800` alone and the run is a candidate for Phase 15's confirmation batch. Reopens if a pad press (button 1 on a machine sms row) makes the picture change inside the frame cap, which would show a cart waiting for input that the ledger's no-input profile cannot reach | before 1, after 1 (no `static_no_fault` leaf has 5 rows, and this is the only row, so no rerun campaign is deferred) |
| 11 | input_mismatch | 0 | closed: no evidence | class: no evidence (there is no row to explain). Source: Phase 13 note rank 11 (0 rows, listed so its emptiness is visible); the final diagnose prints the leaf at 0. Why nothing needs a fix: no row stays `fail` under this leaf, so nothing is dishonest. Reopens when the leaf prints a row (a row whose run board differs from the record's board) | before 0, after 0 (reported as printed) |

groups under 5: 5 groups, 7 rows (13 note); at the phase head 3 groups, 3 rows (rank 9 with 2, rank 10 with 1, rank 11 with 0), as printed by the final run below

Work order: code groups in rank order: ranks 1 and 8 (plan 03), rank 2's gates and decision (plan 04), ranks 3 and 7 (plan 05), ranks 4 and 5 (plan 06), rank 6 (plan 07); closures of rank 2 and the groups under 5, and the final counts, in plan 08 (the last plan writes the closures). Rows stay in rank order whatever plan fills them.

## Gates (plan 04)

The three gates read the baseline sub-rows above; the post-plan-03 counts of the same sub-rows (re-run at head 97f345c) are given beside them where they differ.

D-07 gate: no M4=1 rows. M4=0 by class over every unsupported_video_mode leaf (ranks 2 and 6; no other leaf prints one): graphics1 13 (rank 2: 7, rank 6: 6), graphics2 29 (rank 2); text, multicolor and mixed 0. M4=1 by class and machine: mode4 0, invalid 0, sms2only 0. All 42 rows are on the SMS 1 profile family (machine sms); no Game Gear row.

D-10 families: no family M4=1 rows. zemina: vdpmode graphics1 1 and graphics2 6 on rank 2, rank 7 lt100 write 1; korean_nb: graphics1 1 and graphics2 2 on rank 2, rank 7 reg-0 writes 2; korean: graphics1 2 on rank 2, rank 3 reg-cm writes 2. The decode and the family mapper are not in question, so no trace was taken: not taken, nothing to ask it.

D-04 trigger: fired on reg-0, worked in Task 3. Counts over ranks 3 and 7 together, baseline (post-plan-03 in brackets): reg-0 11 [12] (rank 3 sega 8 [9], rank 7 korean_nb 2 and none 1), reg-zm 3 [4] below threshold, reg-cm 4 [4] below threshold, reg-korean 0, reg-4pak 0. Threshold: 5 rows in one reg-* class, an assumption written in the Decision table.

D-04: no shared header property; board choice needs new evidence. Every rom_write row of ranks 3 and 7 has board source db (36 of 36 on rank 3 post-plan-03, 4 of 4 on rank 7): the database row decides before any header rule in `guess_board` runs, so a header rule in src/cart.c cannot change the board of one of these images. A table row is keyed on a digest and is out (D-04, D-13), and scan thresholds are untouched. The retained output joins no row's write class to its header state, and the leaf size and header sub-rows mix every write class. Routed to the single gap round (CLAUDE.md rule 7) by plan 08. Plan 05 still lands the drop that D-04 decides.

## Video mode decision (plan 07)

D-07 gate read first: plan 04's gate found no M4=1 row in any unsupported_video_mode leaf, and rank 6 is not open on D-07, so D-08 was evaluated.

D-08: shape absent, closed in plan 08. Counts, from `sh scripts/library.sh diagnose` from the repository root at head e6311f2 on a clean tree, exit status 0 (run_fail 49, run_fail_not_in_ledger 0): rank 6 rows 6, `order=before` 6, `order=after` 0, `order=nopicture` 0 (the shape needs 5 or more rows with `order=after`); vdpmode class graphics1 6 of 6 (M4=0, so that half of the shape holds). Every rank 6 fault came before the first non-blank picture, so no supported mode-4 picture was displayed ahead of it and the rows are not a restart transient. No core edit was made, and the raise stays where the renderer's schedule puts it.

## Pins

The base hashes file `build/p14-base-hashes.txt` was taken from origin/main at commit 81a9cc9761873f20910738e393bed8b3558822b6, before any edit of the phase (plan 01).

Plan 03: no pin moved. The Sega bank shift was left as it is (D-06 trace not taken), so no gate was added and the hashes of the 512 KiB shift fixtures are unchanged.

Plan 03 re-run: `sh scripts/library.sh diagnose` from the repository root at head 97f345c, whose core source equals the core commit of the fix (eb8dfee), exit status 0, on a clean tree. Totals: run_fail 205 to 104, run_fail_not_in_ledger 0 (compared as numbers), ledger_fail_now_pass 101. Leaves that grew: rank 3 32 to 36, rank 4 8 to 11, rank 5 7 to 8, rank 9 1 to 2; ranks 2, 6, 7 and 10 unchanged. The 9 added rows are consistent with 110 rows moving out of ranks 1 and 8 (101 passing, 9 landing on the next fault of the same cart); the per-cart link was not traced.

Plan 04: no code changed, so no pin moved and no diagnose re-run was taken; the counts of ranks 2 to 7 are the post-plan-03 re-run's above. `ctest` over the vdp, mapper, core, cart, runner.diagnose and docs groups exits 0 on the plan head.

Plan 05 re-run: `sh scripts/library.sh diagnose` from the repository root at head bd5bce4, whose core source is the plan 05 drop, exit status 0, on a clean tree. Totals: run_fail 104 to 69, run_fail_not_in_ledger 0 (compared as numbers), ledger_fail_now_pass 101 to 136. Leaves after: rank 2 40, rank 4 12, rank 5 8, rank 6 6, rank 9 2, rank 10 1; ranks 3 and 7 no longer print. Leaves that grew: rank 2 36 to 40 and rank 4 11 to 12; ranks 5, 6, 9 and 10 unchanged, so no dark_no_fault or static_no_fault leaf grew. The 40 rows moving out of ranks 3 and 7 split as 35 passing and 5 landing on the next fault of the same cart (4 in rank 2, 1 in rank 4); the per-cart link was not traced. Rank 2's and rank 4's outcome rows are filled by their own plans, with these counts as their start.

Plan 06 re-run: `sh scripts/library.sh diagnose` from the repository root at head 4e7f825, whose core source is the plan 06 drop of both port faults, exit status 0, on a clean tree. Totals: run_fail 69 to 49, run_fail_not_in_ledger 0 (compared as numbers), ledger_fail_now_pass 136 to 156. Leaves after: rank 2 40, rank 6 6, rank 9 2, rank 10 1; ranks 4 and 5 no longer print (12 and 8 before). No leaf grew: ranks 2, 6, 9 and 10 are unchanged from the plan 05 re-run, so all 20 rows leaving ranks 4 and 5 now pass and none landed on a next fault, and no dark_no_fault or static_no_fault leaf grew. Rank 6's outcome row is filled by its own plan, with these counts as its start.

## Final run (plan 08)

`sh scripts/library.sh diagnose` from the repository root at head 230cbc9, whose core source is the last core commit (4e7f825), exit status 0, on a clean tree (the runner prints `core=230cbc9`, no `+dirty`). Totals: judged 573, run_fail 205 to 49, run_fail_not_in_ledger 0 (compared as numbers), ledger_fail_now_pass 156. Leaves at the head, as printed: rank 2 40, rank 6 6, rank 9 2, rank 10 1, rank 11 0; ranks 1, 3, 4, 5, 7 and 8 no longer print. No leaf of ranks 7 to 11 holds 5 or more rows and no new leaf printed, so no leaf escalates. No M4=1 `vdpmode` class prints under either unsupported_video_mode leaf (D-07 re-check): graphics1 13 and graphics2 33, all on machine sms; the M4=0 total is 46.

Pass histogram at the head, first change by frame range (the whole distribution, rows): 1-60 207, 61-120 235, 121-180 68, 181-240 7, 241-300 5, 301-360 1, 361-420 1, 421-600 0.

## Families

D-10: the zemina, korean_nb and korean fail rows, read from the mapper x class cross-cut at the head crossed with the final run's `vdpmode` and `size` sub-rows (no `wclass` sub-row prints at the head: the rom_write leaves are gone, so the baseline `wclass` rows are quoted). No family row is M4=1 (plan 04's D-10 line), so no mapper or header fix was in question and none of the three is open.

| family | rows at head | cause | outcome | class | source | why fail is honest | reopens |
| ------ | ------------ | ----- | ------- | ----- | ------ | ------------------ | ------- |
| zemina | 8 (13 note 8, all fault_static) | legacy TMS mode, 128 KiB or less images: `vdpmode` graphics1 1 and graphics2 7, `size` le128k:none 7 and le128k:valid 1; cross-cut `mapper=zemina class=fault_static` 8; baseline `wclass=lt100:zemina` 1 (rank 7) no longer fails as a write, that row landed on rank 2 after plan 05 | closed | not emulated by design | SMS-VDP section 7; DEC.03; plan 04 D-10 line (no M4=1 family row) | the core draws only the backdrop in a legacy mode, so the picture is not the game's | SEED-006 is promoted, or a row shows an M4=1 `vdpmode` class |
| korean_nb | 5 (13 note 5, all fault_static) | legacy TMS mode, headerless images of 48 KiB or less: `vdpmode` graphics1 1 and graphics2 4, `size` le32k:none 4 and le48k:none 1; baseline `wclass=reg-0:korean_nb` 2 (rank 7) no longer fail as writes, those rows landed on rank 2 after plan 05 | closed | not emulated by design | SMS-VDP section 7; DEC.03; plan 04 D-10 line (no M4=1 family row) | the core draws only the backdrop in a legacy mode, so the picture is not the game's | SEED-006 is promoted, or a row shows an M4=1 `vdpmode` class |
| korean | 2 (13 note 4: 2 fault_static, 2 fault_alive) | the 2 static rows are legacy TMS mode graphics1, images of 48 KiB or less (`size` le48k:bad 1 and le48k:none 1); the 2 alive rows were rank 3's `wclass=reg-cm:korean` writes and no longer fail: the rank 3 drop passes both (korean first-change pass rows 2 to 4 in the histogram, 1-60 and 61-120), so they are counted once, in the passing group | closed | not emulated by design | SMS-VDP section 7; DEC.03; plan 05 (the write drop, rank 3's alive group) | the core draws only the backdrop in a legacy mode, so the picture is not the game's | SEED-006 is promoted, or a row shows an M4=1 `vdpmode` class |

## Gap round

The single gap round of CLAUDE.md rule 7. One entry per open cell, then the escalated leaves, then what was not taken.

- rank 3 fault_alive.rom_write.cart: open: D-04 needs new evidence; 32 rows (13 note), 0 rows at head. Plan 04 found no shared header property: every rom_write row of ranks 3 and 7 has board source db, so a header rule in the board guess cannot move one, and a table row is keyed on a digest (out under D-04 and D-13). The drop D-04 decides landed in plan 05 and the leaf no longer prints; the reg-0 board question (11 rows of ranks 3 and 7 at the baseline, 12 after plan 03) is a seed question about whether the board choice is right for those carts, not a failing row. The reg-zm (3) and reg-cm (4) classes stayed below the threshold of 5 and are SEED-016 candidates.
- leaf escalations: none (no leaf of ranks 7 to 11 holds 5 or more rows at the head and no new leaf printed).
- D-07 re-check: no M4=1 `vdpmode` class under either unsupported_video_mode leaf.
- not taken: the D-06 trace of the Sega bank shift (it needs a dump path the agent may not read); a candidate for Phase 15's confirmation run.
- not taken: the D-10 mapper traces (plan 04: nothing to ask them).

## Boots check

Boots check: sound. Read from the final run only (head 230cbc9), counts only.

No-fault leaves at the head, with their `size=<class>:<header>:<mapper>` sub-rows, as printed: dark_no_fault.never_draws 2 rows (size=le256k:valid:sega 2); static_no_fault.frozen 1 row (size=le256k:valid:sega 1). No dark_no_fault.latch_miss, dark_no_fault.late, static_no_fault.animates_late or static_no_fault.alive_still row prints. No leaf holds 5 rows, so no leaf goes to the Gap round.

Late-picture rows over every failing row (49 at the head, 46 fault and 3 no-fault): first non-blank frame above 600: 0 fault, 0 no-fault; static at 600 and changing by 1800: 0 fault, 0 no-fault. The rows whose faults had masked a late first picture before the fixes did not turn out to have one: none of the 46 fault rows draws late.

Reasons (D-09). The passing first-change distribution ends at 361-420 (1 row) with nothing from 421 to 600; at the head the one interior gap of the Phase 13 note (301-360) is filled by 1 row, so the only wide gap is the one after 361-420. The no-fault rows are 3, under 5 per leaf, too few to show a gap, so a new frame cap would be chosen by how many rows it flips, which the Phase 13 note forbids. `boots@600` is the ledger key (`scripts/compat-common.sh`, `scripts/library.cmake`, `scripts/library.sh`), so moving it migrates every row. The never-draws and static-display seeds stay. No fixture is added because the cap does not move. Each no-fault row is closed by its own leaf: the 2 `dark_no_fault.never_draws` rows keep failing (rank 9) and the 1 `static_no_fault.frozen` row is a static picture (`distinct1800` 1) the static-display seed is meant to reject (rank 10).

Boundary check, run unchanged: `ctest --test-dir build/ci -R '^(runner\.expect\.predicates|runner\.diagnose)$'` exit status 0 (the `boots@20` fixture passes with `distinct_at_least=2`, the static-display seed fails with `distinct_at_least=1`, the run with no image fails with `display=0`). `git diff --quiet origin/main -- runner/expect.c scripts/compat-common.sh scripts/library.cmake` exit status 0: the cap and the distinct rule did not move.

Pad press (D-14): not run; see rank 10. With 1 `static_no_fault` row at the head there is no rerun campaign to defer.

## Pins (plan 08)

Base-versus-head compare: `ctest --test-dir build/ci -R '^runner\.hashes_file$' --output-on-failure --no-tests=error` on the committed head, exit status 0 (the hashes file was removed first and rewritten by the test), then `cmake "-DFILES=build/p14-base-hashes.txt;build/ci/hashes.txt" -DCOUNT=2 -DMODE=compare -P tests/cmake/hashes.cmake`, exit status 0 ("2 identical files"). No pin moved across the phase, so no line needs a justification, and the behaviour revision stays 18. The base file was taken from origin/main (commit 81a9cc9, plan 01), so the worktree form was not needed.

Test files (D-13): `tests/core/test_state.c` and `tests/diag/expected.csv` assert no dropped fault kind (RESEARCH premise check 9), and `tests/mapper/test_mapper.c` asserts only memory-side cartridge and RAM `unmapped_write` faults, which stay, so none of the three needed an edit. The docs/embedding.md revision-18 sentence and Faults rows and the tests/core/test_core.c revision comment already list exactly the changes that landed (the mode nibble, the unused `$FFFC` bits, writes to a cartridge address that is no register, reads of undriven ports, writes to ports nothing decodes), so no drop was held back and neither file changed in this plan.

Gates (plan 08): `cmake --workflow --preset ci` exit status 0 (676 tests and 5 release tests pass) and `cmake --workflow --preset hygiene` exit status 0 (84 tests) on the phase head. The first hygiene run failed: clang-format on the plan 01 diagnose class code and its test, and the no-float scan on a comment in `src/mapper.c` that wrote a section number as a decimal; both were fixed in the files at fault (formatting and comment wording only, no behaviour or hash change). Fences: `git diff --exit-code origin/main -- tests/compat/ledger.csv docs/compatibility.md .github/workflows/ci.yml` exit status 0; no `TIMEOUT` line changed in `tests` or `CMakeLists.txt`; no file added under `tests/cmake`, `scripts` or `tests/compat`; `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit status 0.

