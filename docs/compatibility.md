# Compatibility

Generated from `tests/compat/ledger.csv`; regenerate with `sh scripts/compat-common.sh docs tests/compat/ledger.csv`.

Rows: 10. Evaluated coverage: 9. Own fixtures: 4.

Pass/fail describe accepted predicates only. Unknown has no accepted evaluation; unsupported names an unavailable capability; infrastructure_error describes a runner/judge failure. Metadata membership proves no support. No playable, completable or sound claim; boots is sampled display/distinct-frame/first-fault smoke and may reject static screens. HALT is not a rejection. Experimental through v2.0.

Each row names its setting: machine, ntsc or pal, export or japan, and fm=on or off. TV is a setting of the run, not a fact about the cartridge. audio@F=HASH checks the stream of changes at the chips against a pinned hash, never how it sounds. Format 2: region was split into tv and region; fm joined the run configuration.

- pass: 9
- fail: 0
- unknown: 1
- unsupported: 0
- infrastructure_error: 0
- skipped: 0

- frame predicates: 1
- ram predicates: 1
- mailbox predicates: 1
- boots predicates: 2
- audio predicates: 5
- pal rows judged: 1

| Machine/mapper/status | Rows |
|---|---:|
| gg/sega/unknown | 1 |
| sms/none/pass | 9 |

| ID | Title | Profile | Predicate | Status | Evidence | Date |
|---|---|---|---|---|---|---|
| 00cfce623709b8d9bc9e6bbb96feb9faf9a81f84 | Crazy Faces &#40;Europe, prototype&#41; | gg/export/ntsc/sega fm=off | cfg:video=gg;fm=off;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/boots@600 | unknown | CC0 metadata only src/cartdb.inc | 2026-10-20 |
| fixture/boot_to_play | boot&#95;to&#95;play | sms/export/ntsc/none fm=off | cfg:video=sms;fm=off;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/frame@14=6d28fda91027e139c37717ca73ad48e1f120f1d42eb33b1e029b3fc09f5a50d5 | pass | independent coordinate oracle tests/fixtures/play.h | 2026-10-20 |
| fixture/boot_to_play | boot&#95;to&#95;play | sms/export/ntsc/none fm=off | cfg:video=sms;fm=off;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/mailbox@20 | pass | structural CRC completion only | 2026-10-20 |
| fixture/boot_to_play | boot&#95;to&#95;play | sms/export/ntsc/none fm=off | cfg:video=sms;fm=off;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/ram@14:$C200=00 | pass | input edge counter tests/fixtures/play.c | 2026-10-20 |
| fixture/fm_sweep | fm&#95;sweep | sms/export/ntsc/none fm=off | cfg:video=sms;fm=off;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/audio@300=f2c7712fc71a52769e56e4a8a20bd2a9b840797bc25cb77455be0c52b3d5e13e | pass | regression pin over a transition stream, no sound claim | 2026-11-11 |
| fixture/fm_sweep | fm&#95;sweep | sms/japan/ntsc/none fm=on | cfg:video=sms;fm=on;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/audio@300=add14027f0b06e7ea35755babfc83913080969ce69f91c10459c35978b6feb15 | pass | regression pin over a transition stream, no sound claim | 2026-11-11 |
| fixture/psg_sweep | psg&#95;sweep | sms/export/ntsc/none fm=off | cfg:video=sms;fm=off;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/audio@300=d195b6d2e1de6850d92cb18b027d72af0f6017eb308bac917f9e8588060f6071 | pass | regression pin over a transition stream, no sound claim | 2026-11-11 |
| fixture/psg_sweep | psg&#95;sweep | sms/export/pal/none fm=off | cfg:video=sms;fm=off;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/audio@300=7f0dec6a7065d9f3fc4b37f88480d32b470495b2db926111112cfb464929a064 | pass | regression pin over a transition stream, no sound claim | 2026-11-11 |
| fixture/psg_sweep | psg&#95;sweep | sms/japan/ntsc/none fm=on | cfg:video=sms;fm=on;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/audio@300=55627b9fb8a0bfe8818e16c02f2683ae1fae910137baa9b3853c8988f87f7c03 | pass | regression pin over a transition stream, no sound claim | 2026-11-11 |
| fixture/vdp_lines_192 | vdp&#95;lines&#95;192 | sms/export/ntsc/none fm=off | cfg:video=sms;fm=off;port1=pad;port2=pad;script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/boots@20 | pass | display distinct frames no first fault only | 2026-10-20 |
