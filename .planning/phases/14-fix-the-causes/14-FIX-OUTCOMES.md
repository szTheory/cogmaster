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
| 1 | fault_alive.mapper_range.mapper | 109 | pending | pending | pending |
| 2 | fault_static.unsupported_video_mode.vdp | 36 | pending | pending | pending |
| 3 | fault_alive.rom_write.cart | 32 | pending | pending | pending |
| 4 | fault_alive.unmapped_write.none | 8 | pending | pending | pending |
| 5 | fault_alive.unmapped_read.ctrl | 7 | pending | pending | pending |
| 6 | fault_alive.unsupported_video_mode.vdp | 6 | pending | pending | pending |
| 7 | fault_static.rom_write.cart | 4 | pending | pending | pending |
| 8 | fault_no_display.mapper_range.mapper | 1 | pending | pending | pending |
| 9 | dark_no_fault.never_draws | 1 | pending | pending | pending |
| 10 | static_no_fault.frozen | 1 | pending | pending | pending |
| 11 | input_mismatch | 0 | pending | pending | pending |

groups under 5: 5 groups, 7 rows (13 note)

Work order: code groups in rank order: ranks 1 and 8 (plan 03), rank 2's gates and decision (plan 04), ranks 3 and 7 (plan 05), ranks 4 and 5 (plan 06), rank 6 (plan 07); closures of rank 2 and the groups under 5, and the final counts, in plan 08 (the last plan writes the closures). Rows stay in rank order whatever plan fills them.

## Pins

The base hashes file `build/p14-base-hashes.txt` was taken from origin/main at commit 81a9cc9761873f20910738e393bed8b3558822b6, before any edit of the phase (plan 01).
