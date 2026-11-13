# Compatibility

Generated from `tests/compat/ledger.csv` by `sh scripts/compat-common.sh docs tests/compat/ledger.csv`.

A row marked unknown has no accepted check; that says nothing about whether the game works.
Pass and fail describe accepted checks only (boots is a sampled display and first-fault smoke, audio a pinned hash of chip writes, never how it sounds), with no playable or completable claim. Unsupported means the runner refuses the dump with exit 77. Skipped means no check is run for the entry (MAME marks it not working or a bad dump, or its record is not one image this table can identify) and is never recorded as pass.
A 40-hex id of a compound entry is derived from the text `mame/<list>/<name>` and is not an image hash.
A row names its setting: machine, region, tv, mapper, fm and the check with its frame; tv is a setting of the run, not a fact about the cartridge.

Rows: 1986. Evaluated coverage: 1125. Own fixtures: 4.

- pass=1029 fail=96 infrastructure_error=0
- unevaluated: unknown=832 unsupported=5 skipped=24
- pal rows judged: 512; fm=on rows judged: 53
- predicates: frame=1 ram=1 mailbox=1 boots=1978 audio=5
- SG-1000 entries excluded: 214 (counted in the catalog; the ledger has no row for them)

## Groups

| Machine/mapper/status | Rows |
|---|---:|
| gg/sega/unknown | 755 |
| sms/none/pass | 93 |
| sms/sega/pass | 872 |
| sms/none/fail | 42 |
| sms/sega/unknown | 33 |
| sms/codemasters/pass | 8 |
| gg/none/unknown | 7 |
| sms/sega/fail | 22 |
| sms/codemasters/skipped | 1 |
| gg/compound/skipped | 7 |
| sms/korean/pass | 8 |
| gg/none/pass | 1 |
| sms/compound/skipped | 12 |
| gg/sega_eeprom/unknown | 7 |
| sms/none/unknown | 14 |
| gg/sega/pass | 27 |
| sms/korean_nb/fail | 10 |
| gg/codemasters/unknown | 16 |
| sms/hes_4pak/pass | 2 |
| sms/unsupported/skipped | 1 |
| gg/unsupported/skipped | 1 |
| sms/zemina/fail | 16 |
| sms/hicom/pass | 18 |
| sms/unsupported/unsupported | 3 |
| gg/sega/skipped | 1 |
| gg/unsupported/unsupported | 2 |
| sms/none/skipped | 1 |
| sms/korean/fail | 4 |
| sms/nemesis/fail | 2 |

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

| ID | Title | Profile | Status | Date |
|---|---|---|---|---|
| 021c05f23e7d | The Micro Xevious &#40;Korea&#41; | sms/export/ntsc/none fm=off boots@600 | fail | 2026-11-13 |
| 021c05f23e7d | The Micro Xevious &#40;Korea&#41; | sms/export/pal/none fm=off boots@600 | fail | 2026-11-13 |
| 07bc7896efc9 | Wimbledon &#40;Europe&#41; | sms/export/ntsc/sega fm=off boots@600 | fail | 2026-11-13 |
| 07bc7896efc9 | Wimbledon &#40;Europe&#41; | sms/export/pal/sega fm=off boots@600 | fail | 2026-11-13 |
| 0ddd7448f5bd | Loretta no Shouzou &#40;Japan&#41; | sms/japan/ntsc/sega fm=on boots@600 | fail | 2026-11-13 |
| 202cc058ba46 | Xyzolog &#40;Korea&#41; | sms/export/ntsc/korean_nb fm=off boots@600 | fail | 2026-11-13 |
| 202cc058ba46 | Xyzolog &#40;Korea&#41; | sms/export/pal/korean_nb fm=off boots@600 | fail | 2026-11-13 |
| 26afbc7a049e | Gangcheol RoboCop &#40;Korea&#41; | sms/export/ntsc/sega fm=off boots@600 | fail | 2026-11-13 |
| 26afbc7a049e | Gangcheol RoboCop &#40;Korea&#41; | sms/export/pal/sega fm=off boots@600 | fail | 2026-11-13 |
| 2fb857fc59cc | Comic Bakery &#40;Korea&#41; | sms/export/ntsc/none fm=off boots@600 | fail | 2026-11-13 |
| 2fb857fc59cc | Comic Bakery &#40;Korea&#41; | sms/export/pal/none fm=off boots@600 | fail | 2026-11-13 |
| 3149e453387c | New Boggle Boggle 2 &#40;Korea&#41; | sms/export/ntsc/none fm=off boots@600 | fail | 2026-11-13 |
| 3149e453387c | New Boggle Boggle 2 &#40;Korea&#41; | sms/export/pal/none fm=off boots@600 | fail | 2026-11-13 |
| 31605352efae | Super Bioman 1 &#40;Korea&#41; | sms/export/ntsc/sega fm=off boots@600 | fail | 2026-11-13 |
| 31605352efae | Super Bioman 1 &#40;Korea&#41; | sms/export/pal/sega fm=off boots@600 | fail | 2026-11-13 |
| 49f9d62f121d | Street Master &#40;Korea&#41; | sms/export/ntsc/zemina fm=off boots@600 | fail | 2026-11-13 |
| 49f9d62f121d | Street Master &#40;Korea&#41; | sms/export/pal/zemina fm=off boots@600 | fail | 2026-11-13 |
| 4b0c286448bd | Nemesis 2 &#40;Korea&#41; | sms/export/ntsc/zemina fm=off boots@600 | fail | 2026-11-13 |
| 4b0c286448bd | Nemesis 2 &#40;Korea&#41; | sms/export/pal/zemina fm=off boots@600 | fail | 2026-11-13 |
| 4e35c8375aba | FA Tetris &#40;Korea&#41; | sms/export/ntsc/korean_nb fm=off boots@600 | fail | 2026-11-13 |
| 4e35c8375aba | FA Tetris &#40;Korea&#41; | sms/export/pal/korean_nb fm=off boots@600 | fail | 2026-11-13 |
| 507bfa2b21fd | Super Bubble Bobble &#40;Korea&#41; | sms/export/ntsc/none fm=off boots@600 | fail | 2026-11-13 |
| 507bfa2b21fd | Super Bubble Bobble &#40;Korea&#41; | sms/export/pal/none fm=off boots@600 | fail | 2026-11-13 |
| 55dd4aaa92de | Wimbledon II &#40;Europe, Brazil&#41; | sms/export/ntsc/sega fm=off boots@600 | fail | 2026-11-13 |
| 55dd4aaa92de | Wimbledon II &#40;Europe, Brazil&#41; | sms/export/pal/sega fm=off boots@600 | fail | 2026-11-13 |
| 564b1505fcbc | Bubble Bobble &#40;Korea, Clover&#41; | sms/export/ntsc/none fm=off boots@600 | fail | 2026-11-13 |
| 564b1505fcbc | Bubble Bobble &#40;Korea, Clover&#41; | sms/export/pal/none fm=off boots@600 | fail | 2026-11-13 |
| 592d60e50a63 | King &amp; Balloon &#40;Korea&#41; | sms/export/ntsc/none fm=off boots@600 | fail | 2026-11-13 |
| 592d60e50a63 | King &amp; Balloon &#40;Korea&#41; | sms/export/pal/none fm=off boots@600 | fail | 2026-11-13 |
| 5d4fbf3873af | Spider-Man - Return of the Sinister Six &#40;Europe, Brazil&#41; | sms/export/ntsc/sega fm=off boots@600 | fail | 2026-11-13 |
| 5d4fbf3873af | Spider-Man - Return of the Sinister Six &#40;Europe, Brazil&#41; | sms/export/pal/sega fm=off boots@600 | fail | 2026-11-13 |
| 5d65a308bb9e | Super Boy 3 &#40;Korea&#41; | sms/export/ntsc/zemina fm=off boots@600 | fail | 2026-11-13 |
| 5d65a308bb9e | Super Boy 3 &#40;Korea&#41; | sms/export/pal/zemina fm=off boots@600 | fail | 2026-11-13 |
| 5df1b4c00adb | F-1 Spirit - The way to Formula-1 &#40;Korea&#41; | sms/export/ntsc/zemina fm=off boots@600 | fail | 2026-11-13 |
| 5df1b4c00adb | F-1 Spirit - The way to Formula-1 &#40;Korea&#41; | sms/export/pal/zemina fm=off boots@600 | fail | 2026-11-13 |
| 6ce290d59f14 | Sky Jaguar &#40;Korea&#41; | sms/export/ntsc/none fm=off boots@600 | fail | 2026-11-13 |
| 6ce290d59f14 | Sky Jaguar &#40;Korea&#41; | sms/export/pal/none fm=off boots@600 | fail | 2026-11-13 |
| 7d67dd38fea5 | Back to the Future Part III &#40;Europe&#41; | sms/export/ntsc/sega fm=off boots@600 | fail | 2026-11-13 |
| 7eb9e296b674 | Sky Jaguar &#40;Korea, Clover&#41; | sms/export/ntsc/none fm=off boots@600 | fail | 2026-11-13 |
| 7eb9e296b674 | Sky Jaguar &#40;Korea, Clover&#41; | sms/export/pal/none fm=off boots@600 | fail | 2026-11-13 |

+56 more fail rows; the rest is in [the ledger](../tests/compat/ledger.csv)

## Other evaluated rows

| ID | Title | Profile | Status | Date |
|---|---|---|---|---|
| 0180c9b26c5a | Comical Machine Gun Joe &#40;Korea&#41; | sms/export/ntsc/none fm=off boots@600 | pass | 2026-11-13 |
| 0180c9b26c5a | Comical Machine Gun Joe &#40;Korea&#41; | sms/export/pal/none fm=off boots@600 | pass | 2026-11-13 |
| 0199c62afb5c | Chuck Rock &#40;Europe, Brazil&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 0199c62afb5c | Chuck Rock &#40;Europe, Brazil&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 021c6983fdab | Dragon Crystal &#40;Europe, Brazil&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 021c6983fdab | Dragon Crystal &#40;Europe, Brazil&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 0236c5239c92 | Dead Angle &#40;Europe, USA&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 0236c5239c92 | Dead Angle &#40;Europe, USA&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 0275f34127af | Missile Defense 3-D &#40;prototype&#41; | sms/export/ntsc/sega fm=off boots@600 port1=lphaser | pass | 2026-11-13 |
| 0278cd120dc3 | Fantasy Zone &#40;World, v2&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 0278cd120dc3 | Fantasy Zone &#40;World, v2&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 02ebee891d88 | Spider-Man vs. The Kingpin &#40;Europe, USA, Brazil&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 02ebee891d88 | Spider-Man vs. The Kingpin &#40;Europe, USA, Brazil&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 038920e63437 | The Simpsons - Bart vs. The World &#40;Europe, Brazil&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 038920e63437 | The Simpsons - Bart vs. The World &#40;Europe, Brazil&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 051e72c8ffec | Strider &#40;Europe, USA, Brazil, Korea&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 051e72c8ffec | Strider &#40;Europe, USA, Brazil, Korea&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 051e74c499c6 | Ghost House &#40;Japan, MyCard&#41; | sms/export/ntsc/none fm=off boots@600 | pass | 2026-11-13 |
| 051e74c499c6 | Ghost House &#40;Japan, MyCard&#41; | sms/export/pal/none fm=off boots@600 | pass | 2026-11-13 |
| 05b4f23e33ad | Dinobasher Starring Bignose the Caveman &#40;Europe, prototype&# | sms/export/ntsc/codemasters fm=off boots@600 | pass | 2026-11-13 |
| 05b4f23e33ad | Dinobasher Starring Bignose the Caveman &#40;Europe, prototype&# | sms/export/pal/codemasters fm=off boots@600 | pass | 2026-11-13 |
| 064040452b6b | Operation Wolf &#40;Europe, Brazil&#41; | sms/export/ntsc/sega fm=off boots@600 port1=lphaser | pass | 2026-11-13 |
| 06664daf208f | Parlour Games &#40;Europe, USA&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 06664daf208f | Parlour Games &#40;Europe, USA&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 075297d2f3a8 | E-SWAT - City Under Siege &#40;Europe, USA, easy version&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 075297d2f3a8 | E-SWAT - City Under Siege &#40;Europe, USA, easy version&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 07d8f300b3a3 | Baku Baku Animal &#40;Brazil&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 07d8f300b3a3 | Baku Baku Animal &#40;Brazil&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 07fcf297be4f | Phantasy Star &#40;Europe, USA, v3&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 07fcf297be4f | Phantasy Star &#40;Europe, USA, v3&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 0854e36d3bb0 | Battlemaniacs &#40;Brazil&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 0854e36d3bb0 | Battlemaniacs &#40;Brazil&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 08a3484e862a | Quartet &#40;Europe, USA&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 08a3484e862a | Quartet &#40;Europe, USA&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 08a79905767b | Ace of Aces &#40;Europe&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 08a79905767b | Ace of Aces &#40;Europe&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 08b4f5b8096b | Sensible Soccer &#40;Europe&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 08b4f5b8096b | Sensible Soccer &#40;Europe&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |
| 08ec70a2cd98 | R-Type &#40;World&#41; | sms/export/ntsc/sega fm=off boots@600 | pass | 2026-11-13 |
| 08ec70a2cd98 | R-Type &#40;World&#41; | sms/export/pal/sega fm=off boots@600 | pass | 2026-11-13 |

+980 more other evaluated rows; the rest is in [the ledger](../tests/compat/ledger.csv)
