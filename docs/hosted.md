# Hosted RetroArch

The hosted frontend check that runs a real RetroArch against the core, and the in-process content, rendering and input tests beside it. The test entry point and lanes are in [testing.md](testing.md).

## Hosted RetroArch

The `retroarch` job proves that a real frontend shows the frame the runner hashes. It is a CI step, not a CTest,
so it never skips itself off macOS. On `macos-15` it:

1. unzips the core and the runner from the macOS arm64 `build` leg's archives;
2. fetches the pinned RetroArch Metal disk image with `scripts/fetch-pin.sh retroarch-dmg`, which checks its
   SHA-256 before anything opens it, copies the app out and checks its bundle version against `pins.txt`;
3. runs `tests/retroarch/run_retroarch.cmake`: RetroArch gets its own `HOME`, `CFFIXED_USER_HOME` and the
   configuration from `tests/retroarch/test.cfg.in`, and starts with `--verbose -L <core>`, no content and
   `--max-frames=60 --max-frames-ss`. The launch is bounded at 50 seconds and a failure prints its log;
4. writes the runner's frame with `cogmaster-run --frames 60 --png 60:runner.png`, converts both PNGs to BMP with
   `sips`, builds `tests/retroarch/compare.c`, and requires equality;
5. runs the comparator with `--flip` through `tests/cmake/expect_fail.cmake`: it inverts one channel of actual
   pixel (7,3) in memory and must exit 1 naming that pixel;
6. downloads only the owned fixture, runner input and replay-only module from the same build. A 14-frame
   baseline records a replay in the released frontend. `replay.c` checks the observed v2 event layout and
   changes only port 0 RetroPad B: press, hold, release, press again. The frontend plays that stream into
   the adapter's normal input callback in a fresh session. The final released frames remain at counter 2;
7. compares both screenshots with runner frames and, independently, every coordinate with `play_pixel`
   (baseline counter/scroll 0, input 2), runs the pixel control on the changed frame and uploads only the PNGs;
8. records sound with `--record` and `tests/retroarch/record.cfg` (mov, 16-bit PCM): the no-content session is a
   silent control, and `psg_sweep.sms` and `psg_sweep.gg` are the sound sessions. `tests/retroarch/audio.cmake`
   requires the movie, `afinfo` exit 0, 188,000 to 196,000 audio bytes, no non-zero byte in the control and at
   least 8,192 in each sound session. It reads no log text.

**Saves across a restart.** `battery.sms` and `eeprom.gg` each run twice in RetroArch on one directory, the second
time with `-DKEEP=1` so `ra-save-<name>/saves` survives. After each run the `.srm` must equal, byte for byte, the one
`cogmaster-run --save-dir` writes after the same number of runs from an empty directory. A control compares the
runner's first and second `.srm` and must fail through `expect_fail.cmake`: the images change their save on every
run, so equality after the second pass shows RetroArch handed the first save back to the core.

The shared driver, helpers, checker, synthetic comparator fixture and pixel oracle total at most 300 physical
lines; `policy.e2e_budget` counts them. The BMP reader accepts the uncompressed RGB24 output from `sips`.
The test-only replay module compiles the production adapter unchanged and forwards only its three state
entrypoints to the core state visitor, which enables the frontend's mandatory initial checkpoint; it supplies
no input and is never packaged. The production module still refuses save states.
`libretro.host.content` and `libretro.host.replay` run the same callback/pixel sequence against both modules.
The portable screenshot tests prove that matching pixels pass and that wrong scroll or a changed pixel fails. The replay layout was measured from the released 1.22.2 binary; no emulator source was read.
The hosted Game Gear case holds Start on frames 5, 6 and 8 and compares the native 160×144 crop at field
origin (48,24) with the shared coordinate oracle.

**Rerunning it.** Rerun the job from the run page, or `gh run rerun <run-id> --failed`. On a Mac with a desktop
session the same steps run locally from the job's commands, with `-DRETROARCH` pointing at an installed
`RetroArch.app/Contents/MacOS/RetroArch`.

## Content rendering and input

`fixtures.boot_to_play` compares every native crop coordinate with a hand-expanded
diamond and bar construction, then compares its independently calculated digest with
the runner. `fixtures.boot_to_play.wrong_hash` proves that a changed expected digest
fails. The image digest is checked after assembly. The mailbox test checks eight
released samples and their CRC. `fixtures.boot_to_play.input` checks press in frame 5,
hold in 6, release in 7 and another press in 8, with independently expected counter,
scroll and final pixels. `runner.input` exercises the bounded grammar, both newline
forms, optional final newline, missing/surplus records and invalid fields.

`libretro.host.content` loads those same bytes into the production module with
`need_fullpath=false`. Its frontend supplies the same script, checks one poll before
each frame, reconstructs every callback pixel as RGB444 and checks each coordinate
from frame 4 against the independent oracle. Its final digest equals the runner's.
It also checks descriptor and pad/none claims, disconnected pads and missing input
callbacks. This is in-process integration evidence; the real RetroArch job
described above remains the separate frontend check.
