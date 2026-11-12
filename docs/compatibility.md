# Compatibility

Generated from `tests/compat/ledger.csv` by `sh scripts/compat-common.sh docs tests/compat/ledger.csv`.

Experimental. A row marked unknown has no accepted check; that says nothing about whether the game works.
Pass and fail describe accepted checks only (boots is a sampled display and first-fault smoke, audio a pinned hash of chip writes, never how it sounds), with no playable or completable claim. Unsupported means the runner refuses the dump with exit 77. Skipped means no check is run for the entry (MAME marks it not working or a bad dump, or its record is not one image this table can identify) and is never recorded as pass.
A 40-hex id of a compound entry is derived from the text `mame/<list>/<name>` and is not an image hash. Experimental leaves only on the own-library evidence of the maintainer, never on a pass rate.
A row names its setting: machine, region, tv, mapper, fm and the check with its frame; tv is a setting of the run, not a fact about the cartridge.

Rows: 1472. Evaluated coverage: 9. Own fixtures: 4.

- pass=9 fail=0 infrastructure_error=0
- unevaluated: unknown=1434 unsupported=5 skipped=24
- pal rows judged: 1; fm=on rows judged: 2
- predicates: frame=1 ram=1 mailbox=1 boots=1464 audio=5
- SG-1000 entries excluded: 214 (counted in the catalog; the ledger has no row for them)

## Groups

| Machine/mapper/status | Rows |
|---|---:|
| gg/sega/unknown | 780 |
| sms/none/unknown | 78 |
| sms/sega/unknown | 511 |
| sms/codemasters/unknown | 4 |
| gg/none/unknown | 8 |
| sms/codemasters/skipped | 1 |
| gg/compound/skipped | 7 |
| sms/korean/unknown | 6 |
| sms/compound/skipped | 12 |
| gg/sega_eeprom/unknown | 7 |
| sms/korean_nb/unknown | 5 |
| gg/codemasters/unknown | 16 |
| sms/hes_4pak/unknown | 1 |
| sms/unsupported/skipped | 1 |
| gg/unsupported/skipped | 1 |
| sms/zemina/unknown | 8 |
| sms/hicom/unknown | 9 |
| sms/unsupported/unsupported | 3 |
| gg/sega/skipped | 1 |
| gg/unsupported/unsupported | 2 |
| sms/none/skipped | 1 |
| sms/nemesis/unknown | 1 |
| sms/none/pass | 9 |

## Reasons

| token | status | rows |
|---|---|---:|
| mame_no | skipped | 6 |
| short_dump | skipped | 3 |
| multi_chip | skipped | 12 |
| bad_dump | skipped | 2 |
| seojin | unsupported | 1 |
| janggun | unsupported | 1 |
| xterminator | unsupported | 2 |
| graphic | unsupported | 1 |
| no_rom | skipped | 1 |

## Fixtures

| ID | Title | Profile | Status | Date |
|---|---|---|---|---|
| fixture/boot_to_play | boot&#95;to&#95;play | sms/export/ntsc/none fm=off frame@14 | pass | 2026-10-20 |
| fixture/boot_to_play | boot&#95;to&#95;play | sms/export/ntsc/none fm=off mailbox@20 | pass | 2026-10-20 |
| fixture/boot_to_play | boot&#95;to&#95;play | sms/export/ntsc/none fm=off ram@14 | pass | 2026-10-20 |
| fixture/fm_sweep | fm&#95;sweep | sms/export/ntsc/none fm=off audio@300 | pass | 2026-11-11 |
| fixture/fm_sweep | fm&#95;sweep | sms/japan/ntsc/none fm=on audio@300 | pass | 2026-11-11 |
| fixture/psg_sweep | psg&#95;sweep | sms/export/ntsc/none fm=off audio@300 | pass | 2026-11-11 |
| fixture/psg_sweep | psg&#95;sweep | sms/export/pal/none fm=off audio@300 | pass | 2026-11-11 |
| fixture/psg_sweep | psg&#95;sweep | sms/japan/ntsc/none fm=on audio@300 | pass | 2026-11-11 |
| fixture/vdp_lines_192 | vdp&#95;lines&#95;192 | sms/export/ntsc/none fm=off boots@20 | pass | 2026-10-20 |

## Unsupported rows

| ID | Title | Profile | Status | Date |
|---|---|---|---|---|
| 83d6f261471d | Super Game 11-in-1 &#40;Korea&#41; | sms/export/ntsc/unsupported fm=off boots@600 | unsupported | 2026-11-12 |
| a290e41e0218 | Janggun ui Adeul &#40;Korea&#41; | sms/export/ntsc/unsupported fm=off boots@600 | unsupported | 2026-11-12 |
| b6664c21a99a | X-Terminator v2.1 for Game Gear &#40;USA, Europe&#41; | gg/export/ntsc/unsupported fm=off boots@600 | unsupported | 2026-11-12 |
| d04ea11e1f67 | Sega Graphic Board &#40;Japan, prototype v2.0&#41; | sms/export/ntsc/unsupported fm=off boots@600 | unsupported | 2026-11-12 |
| ee2123cc2add | X-Terminator v2.1J for Game Gear &#40;Japan&#41; | gg/japan/ntsc/unsupported fm=off boots@600 | unsupported | 2026-11-12 |

## Fail rows

none

## Other evaluated rows

none
