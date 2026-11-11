# Performance and fuzzing

The instructions-per-frame job, the nightly bench, the fuzz targets and how to add a regression. The test entry point and lanes are in [testing.md](testing.md).

## Performance

**Instructions per frame.** The `perf` job counts instructions while `cogmaster-run` executes the owned
`boot_to_play.sms` fixture under Cachegrind: Ir/frame = (Ir at 360 frames − Ir at 60) / 300, in integers, with `--cache-sim=no`, an
empty environment and runner paths of equal length, so the count repeats exactly. It measures the pull request's
head commit once and its merge base with the base branch twice, then `tests/cmake/perf_compare.cmake` prints one
line to the log and the job summary:

```text
perf boot_to_play head=2146896 base=2146896 delta_bp=0 verdict=PASS
```

`delta_bp` is the change in basis points. `PASS` means at most 1.00 % slower (exactly +1.00 % passes); `FAIL`
more than that or more than the allow file permits; `UNSTABLE` the two base measurements differ; `NO_BASE` the
merge base lacks comparable rendering/input support. An empty or non-numeric measurement is an input error, never a pass.
Head and base load the same head-built fixture and input files with explicit SMS2, SMS video and NTSC export
configuration. Input presses button 1 at frames 5, 6 and 8, then releases it. The comparison requires a base
with API at least 5, behaviour revision at least 8 and the playing fixture source; an older phase's card-only
path cannot provide a meaningful base. `perf.playing` checks this bench path against the independent pixel oracle.

**The gate.** The job runs with `COGMASTER_PERF_MODE: gate` in `ci.yml`: `FAIL`, `UNSTABLE` and `NO_BASE` fail it,
and so fail `CI required`. There is no stored baseline: the merge base is measured twice, so the comparison is
between two commits built on the same runner, and the limit stays +1.00 %. On `workflow_dispatch` the job measures
one commit three times and prints `perf control boot_to_play runs=<a>,<b>,<c> spread=<n>`, the control that a
repeat run gives the same count.

**Exceptions.** A pull request that must be slower adds a line to `tests/perf/allow.txt`:
`<rom> <max % × 100> <expiry YYYY-MM-DD> <reason>`. It raises that ROM's limit through its expiry, at most 60
days away, and is removed after the pull request merges.

**Nightly bench.** `.github/workflows/nightly.yml` runs the same owned fixture/profile/script with
`--bench 300000` every night on
`ubuntu-26.04`, `ubuntu-26.04-arm` and `macos-15`, and under wasmtime. Each report is the runner's nine lines
(see [runner.md](runner.md#bench-report)), the input lag count and the runner's name; WASI has no resident set size, so wasm32's
`max_rss_kib` reads 0. Reports are trend data in the job summaries and `bench-*` artifacts, never a gate and never
committed. A failed night opens or comments on the one open issue labelled `nightly`; a good night closes it.
Dispatching the workflow with `force_failure` fails a run on purpose, after the reports are uploaded. A pull
request that changes the bench or the workflow runs the bench jobs but writes no issue.

## Fuzzing

Four fuzz targets each define `LLVMFuzzerTestOneInput`: `tests/fuzz/fuzz_loader.c` probes the bytes and loads them
on the profile the probe guesses, and fails when the probe and the load disagree on accepting them;
`tests/fuzz/fuzz_run.c` loads them with fast boot and runs two frames.
`tests/fuzz/fuzz_save.c` imports the bytes against independently constructed physical RAM
with known, none, unknown and unsupported persistence. It checks FF padding, exact bytes,
generation-neutral repeated import and full-state equality after refusal, including SIZE_MAX
without a large source buffer. `battery.continuation` separately runs production image
classification and the owned guest: two same-size accepted inputs continue from different counters.
`tests/fuzz/fuzz_eeprom.c` drives the 93C46 on an EEPROM-board instance with pairs of writes to `$8000` and `$FFFC`,
checks the chip's phase, count and flag bounds after every write, and requires a saved state to load into a second
instance and save byte-equal.
Their starting inputs are the images
`tests/fuzz/seeds.c` writes into `build/<preset>/tests/seeds` at build time; no corpus is committed.

**Replay, every pull request.** `fuzz.regress.loader`, `fuzz.regress.run`, `fuzz.regress.save` and `fuzz.regress.eeprom` link each entry with a plain `main`
(`tests/fuzz/replay.c`) and replay every seed, and every file in `tests/fuzz/regress` when it exists, on every
lane that runs the `portable` label. Each prints `replayed N` and fails when N is 0. No libFuzzer is needed.

**libFuzzer, nightly.** The `fuzz` job in `nightly.yml` runs `cmake --workflow --preset fuzz` on `ubuntu-26.04`:
clang-21 with `-fsanitize=fuzzer-no-link,address,undefined`, the four targets linked with `-fsanitize=fuzzer`, each
run for 60 s over a fresh copy of the seeds with `-timeout=2 -max_len=1049089` (1 MiB, a copier header and one
byte, so the size-reject path is reachable). A crashing input is uploaded as the `fuzz-artifacts` artifact. The
job is `continue-on-error` until ten green nightly runs. A finding is a harness or core bug to triage, never a
release gate of its own.

**Adding a regression.** Minimise the input (`cogmaster-fuzz-<target> -minimize_crash=1 -runs=10000 <file>`), fix
the bug, commit the minimised file under `tests/fuzz/regress/` and add its `tests/roms/manifest.txt` row
(`scripts/hygiene.sh` rejects a binary file without one). Reconfigure: the replay tests then read the directory.
`docs.budgets` keeps `tests/fuzz` at most 512 KiB.
