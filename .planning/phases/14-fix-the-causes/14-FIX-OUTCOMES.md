# Phase 14 fix outcomes

Counts only.

- core commit: faf9cc788dbfea6973234e50c2861f157c763af7
- behaviour revision: 18
- date (UTC): 2026-11-13
- command: `sh scripts/library.sh diagnose`, exit status 0
- ledger read: tests/compat/ledger.csv at commit 032ebabc340b56b1123bb8745cc91d8c7a635189
- caps: verdict at frame 600, view to frame 1800
- phase 13 note: .planning/phases/13-why-carts-fail/13-CAUSE-GROUPS.md at commit 17375f726b7452e6fd3d9049a42915574602f7cc

The baseline run was made on a clean tree at the core commit above, with the mode nibble in the fault record and before any benign fault kind was dropped. Its totals: run_fail 205 and run_fail_not_in_ledger 0 (compared as numbers), the same 205 failing rows as the Phase 13 note.

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
| 2 | fault_static.unsupported_video_mode.vdp | 36 | decided: close per D-07 | M4=0 by class: graphics1 7, graphics2 29 (36 of 36); M4=1 rows 0, so no decode question to check; closure written in plan 08 | before 36, after 36 (no code change in this plan) |
| 3 | fault_alive.rom_write.cart | 32 | fixed in the core; open: D-04 needs new evidence | baseline wclass split, rank 3: lt100:sega 8, reg-0:sega 8, reg-cm:korean 2, reg-cm:sega 2, reg-zm:sega 3, rom:none 1, rom:sega 4, slot2:sega 4. D-04 trigger fired on reg-0 (11 rows of ranks 3 and 7 together, baseline) and plan 04 found no shared header property: board choice needs new evidence (all 36 rows source db), so the board question stays open for plan 08's gap round. The drop D-04 decides is landed: a write to a cartridge address that is no register and no enabled cartridge RAM records no fault on every board (Phase 13 note rank 3). Re-run: the leaf no longer prints, run_fail_not_in_ledger 0 | before 32 (36 after plan 03), after 0 (the leaf no longer prints) |
| 4 | fault_alive.unmapped_write.none | 8 | fixed in the core | D-11 trigger not met: the baseline pclass split of rank 4 is 3e-3f:gg:export 8, and no pclass sub-row has machine sms or sms1 with region japan (0 rows), so the write half is dropped (inference: no document lists a fault for a write to a port nothing decodes, SMS-SYSTEM section 4). A write to a port nothing decodes records no fault; memory-side unmapped writes still fault. Re-run: the leaf no longer prints, run_fail_not_in_ledger 0 | before 8 (11 after plan 03, 12 after plan 05), after 0 (the leaf no longer prints) |
| 5 | fault_alive.unmapped_read.ctrl | 7 | fixed in the core | baseline pclass split, rank 5: 00-06:sms:export 6, lt40:gg:export 1. A read of an undriven port in $00-$3F returns open bus, still counts in the open-bus counter and records no fault (SMS-SYSTEM C5 and section 4). Re-run: the leaf no longer prints, run_fail_not_in_ledger 0 | before 7 (8 after plan 03), after 0 (the leaf no longer prints) |
| 6 | fault_alive.unsupported_video_mode.vdp | 6 | pending | pending | pending |
| 7 | fault_static.rom_write.cart | 4 | fixed in the core | baseline wclass split, rank 7: lt100:zemina 1, reg-0:korean_nb 2, reg-0:none 1; counted in the reg-0 trigger above, whose board-choice part is rank 3's open item (all 4 rows source db). Same drop as rank 3 (Phase 13 note rank 7). Re-run: the leaf no longer prints, run_fail_not_in_ledger 0 | before 4, after 0 (the leaf no longer prints) |
| 8 | fault_no_display.mapper_range.mapper | 1 | fixed in the core | inherits rank 1: faddr=fffc:y 1 in the baseline (13 note rank 8); the leaf no longer prints in the re-run | before 1, after 0 |
| 9 | dark_no_fault.never_draws | 1 | pending | pending | pending |
| 10 | static_no_fault.frozen | 1 | pending | pending | pending |
| 11 | input_mismatch | 0 | pending | pending | pending |

groups under 5: 5 groups, 7 rows (13 note)

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
