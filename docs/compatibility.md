# Compatibility

Generated from `tests/compat/ledger.csv` by `sh scripts/compat-common.sh docs tests/compat/ledger.csv`.

Experimental. A row marked unknown has no accepted check; that says nothing about whether the game works.
Pass and fail describe accepted checks only (boots is a sampled display and first-fault smoke, audio a pinned hash of chip writes, never how it sounds), with no playable or completable claim. Unsupported means the runner refuses the dump with exit 77. Skipped is the MAME verdict about the dump and is never recorded as pass.
A 40-hex id of a compound entry is derived from the text `mame/<list>/<name>` and is not an image hash. Experimental leaves only on the own-library evidence of the maintainer, never on a pass rate.
A row names its setting: machine, region, tv, mapper, fm and the check with its frame; tv is a setting of the run, not a fact about the cartridge.

Rows: 10. Evaluated coverage: 9. Own fixtures: 4.

- pass=9 fail=0 infrastructure_error=0
- unevaluated: unknown=1 unsupported=0 skipped=0
- pal rows judged: 1; fm=on rows judged: 2
- predicates: frame=1 ram=1 mailbox=1 boots=2 audio=5
- SG-1000 entries excluded: 214 (counted in the catalog; the ledger has no row for them)

## Groups

| Machine/mapper/status | Rows |
|---|---:|
| gg/sega/unknown | 1 |
| sms/none/pass | 9 |

## Reasons

| token | status | rows |
|---|---|---:|

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

none

## Fail rows

none

## Other evaluated rows

none
