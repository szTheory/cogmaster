# Phase 13 cause groups

Counts only. The failing default-profile rows of the owner library, grouped by cause by the diagnostic.

- core commit: 470b0583bf45fa9eb206deb63b89f69dae2b2665
- behaviour revision: 17
- date (UTC): 2026-11-13
- command: `sh scripts/library.sh diagnose`, exit status 0
- ledger read: tests/compat/ledger.csv at commit 032ebabc340b56b1123bb8745cc91d8c7a635189
- caps: verdict at frame 600, view to frame 1800

## Cause table

| rank | group | fault kind x device | rows | sub-rows |
| ---- | ----- | ------------------- | ---- | -------- |
| 1 | fault_alive.mapper_range.mapper | mapper_range x mapper | 109 | pcslot 0:97 1:12; where mapper:109; pc 001E:5 0045:5 006C:6 0083:5 008B:6 008F:5 |
| 2 | fault_static.unsupported_video_mode.vdp | unsupported_video_mode x vdp | 36 | pcslot 0:6 1:14 2:12 3:2 4:2; where vdp:36 |
| 3 | fault_alive.rom_write.cart | rom_write x cart | 32 | pcslot 0:3 1:25 2:4; where rom:28 slot2:4 |
| 4 | fault_alive.unmapped_write.none | unmapped_write x none | 8 | pcslot 0:5 1:3; where port:8 |
| 5 | fault_alive.unmapped_read.ctrl | unmapped_read x ctrl | 7 | pcslot 1:6 2:1; where port:7 |
| 6 | fault_alive.unsupported_video_mode.vdp | unsupported_video_mode x vdp | 6 | pcslot 0:5 1:1; where vdp:6; pc 000D:5 |
| 7 | fault_static.rom_write.cart | rom_write x cart | 4 | pcslot 0:2 1:2; where rom:4 |
| 8 | fault_no_display.mapper_range.mapper | mapper_range x mapper | 1 | pcslot 0:1; where mapper:1 |
| 9 | dark_no_fault.never_draws | - | 1 | - |
| 10 | static_no_fault.frozen | - | 1 | - |
| 11 | input_mismatch | - | 0 | - |

groups under 5: 5 groups, 7 rows

The count includes the input_mismatch group at 0 rows, which the table lists so that its emptiness is visible. A group of exactly 5 rows is not under 5.

The pc entries list only program counters that account for 5 or more rows of a group; the sub-rows are aggregates, never per-row lists.

## Mapper x class cross-cut

FIX-03 reads the zemina, korean_nb and korean rows here whatever their group sizes.

| mapper | class | rows |
| ------ | ----- | ---- |
| korean | fault_static | 2 |
| korean | fault_alive | 2 |
| korean_nb | fault_static | 5 |
| nemesis | fault_static | 1 |
| none | fault_static | 21 |
| none | fault_alive | 1 |
| sega | fault_no_display | 1 |
| sega | fault_static | 3 |
| sega | fault_alive | 159 |
| sega | dark_no_fault | 1 |
| sega | static_no_fault | 1 |
| zemina | fault_static | 8 |

## Passing rows by first picture

Rows that pass, by mapper and by the frame range in which the first changing picture appears.

| mapper | first change (frames) | rows |
| ------ | --------------------- | ---- |
| codemasters | 1-60 | 4 |
| hes_4pak | 1-60 | 1 |
| hicom | 61-120 | 9 |
| korean | 1-60 | 1 |
| korean | 61-120 | 1 |
| none | 1-60 | 5 |
| none | 61-120 | 30 |
| none | 121-180 | 2 |
| none | 181-240 | 3 |
| none | 361-420 | 1 |
| sega | 1-60 | 121 |
| sega | 61-120 | 143 |
| sega | 121-180 | 39 |
| sega | 181-240 | 3 |
| sega | 241-300 | 5 |

## Totals

```
diag-other: candidates=696 attempted=573 evaluated=573 malformed=0 malformed_io=0 uncatalogued=82 skipped_excluded=30 noprofile=0 unsupported=11 infrastructure_error=0 title_withheld=1
diag-other: judged=573 run_fail=205 ledger_fail=209 matched=205 ledger_fail_not_in_run=4 ledger_fail_now_pass=0 run_fail_not_in_ledger=0 groups_total=205 fail_total=205
```

The difference between the candidate-level fail count (ledger_fail) and the row-level fail count (run_fail) is explained by ledger_fail_not_in_run and run_fail_not_in_ledger, as printed above. That groups_total equals fail_total is asserted by the diagnostic command exiting 0, not by comparing printed numbers; a unit test pins that the command exits 1 when they differ.

## For Phase 14

- Group size is not cause validity. A large group may have several causes behind one fault kind, and a small group may be the whole story for a mapper.
- Any benign fault kind is justified per kind from hardware documentation, never from how many rows it would clear.
- The late-first-picture frame is chosen where a gap shows in the distribution of first-change frames, never by how many rows would flip.
- The group never_draws keeps failing: a cartridge that never draws is a failure whatever its size.
- Leaves are reported as printed. The taxonomy was not tuned after the run, no leaf was merged, and no leaf was re-split to push it under 5.
- Reproducing one leaf needs the dumps again: the owner passes COGMASTER_ROM_DIR to the agent's shell.
- Titles for a leaf come from filtering the group= field of the diagnostic's diag lines in the agent's terminal; they are never written down. About 3 percent of catalog titles print as a dash under the title charset, so a leaf's count can exceed its named rows.
