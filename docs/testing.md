# Testing

Every check CI runs, a contributor can run with one command. This page covers that command, the lanes, the test
inventories, the negative controls, the pinned hashes, the hosted RetroArch check, the performance job and how to
reproduce a CI failure. Releases and repository settings are in [maintaining.md](maintaining.md).

## The entry point

```sh
cmake --workflow --preset ci
```

It needs CMake 4.3 or later (the presets' minimum; the library alone configures with 3.25), Ninja and a C and
C++ compiler. The workflow runs five steps:

1. configure into `build/ci` (Release, warnings as errors, macOS deployment target 10.13);
2. build;
3. test every test except those labelled `release`, writing `build/ci/junit.xml`;
4. package the three release zips into `build/ci`;
5. test the `release` label against those zips, writing `build/ci/junit-release.xml`.

On a recent Mac a clean run takes a couple of seconds.

**Reconfigure after committing.** Some checks read git's list of tracked files (the tree scan, the notices check,
the single version source), and some copy a tracked file into the build tree when CMake configures (the `CI
required` fixture copies `ci.yml`). A new file is seen only once git tracks it, and a configure-time copy is only
as fresh as the last configure. After adding or committing files, rerun the whole workflow preset, which
reconfigures, rather than `ctest` alone.

## Lanes

Each lane is one workflow preset, `cmake --workflow --preset <name>`, building into `build/<name>`.

| Preset | What it proves | Inventory | Tests run |
|---|---|---|---|
| `dev` | A Debug build passes, for day-to-day work. No packaging. | `ci` | all but `release` |
| `ci` | The workflow above: build, tests and release zips. Every Linux and macOS leg runs it. | `ci` | all but `release`, then `release` |
| `ci-msvc` | The same five steps with MSVC and the static C runtime. | `ci` | all but `release`, then `release` |
| `asan` | No memory or undefined-behaviour fault, with clang-21's AddressSanitizer and UndefinedBehaviorSanitizer; any finding stops the test. | `ci` | all but `abi` and `release`, whose binary-shape checks the instrumentation changes |
| `nofp` | The core uses no floating point: GCC's `-mgeneral-regs-only` rejects any floating-point type. | `nofp` | `abi` |
| `hygiene` | The repository is clean: tree scan, formatting, the integer-only text scan, the analyzer, the pins file, and the tests of CI itself. | `hygiene` | `hygiene` |
| `be32` | The same hashes on a big-endian, 32-bit target with unsigned `char`: 32-bit PowerPC (`cmake/toolchains/ppc-linux-gnu.cmake`) under `qemu-ppc`. | `ci` | `portable` |
| `wasm32` | The same hashes in WebAssembly: clang-21 for `wasm32-wasi` (`cmake/toolchains/wasm32-wasi.cmake`) under the pinned wasmtime. WASI cannot load a libretro core, so this lane builds none. | `wasm32` | `portable` |
| `fuzz` | The nightly libFuzzer runs of the loader and a two-frame run under clang-21's ASan and UBSan (see [Fuzzing](#fuzzing)). No fixtures, no libretro core. | `fuzz` | `fuzz` |
| `bench` | A Release runner for the bench and the instructions-per-frame job. Configure and build only, with no tests and no fixtures. | none | none |

On the two emulated lanes CMake runs each test binary through `CMAKE_CROSSCOMPILING_EMULATOR`, and every script
that starts `cogmaster-run` takes the same emulator as `-DEMULATOR=`, so a test never runs a host binary in place
of the guest one. The wasm32 emulator is `build/pins/wasmtime/wasmtime run --dir=<source>::<source>`: the build
tree sits under the source tree, so one preopen covers both at the same paths. Fetch the runtime first with
`scripts/fetch-pin.sh wasmtime-<arch>-linux build/pins/wasmtime`. The wasm32 lane takes clang-21, lld-21,
`wasi-libc` and `libclang-rt-21-dev-wasm32` from the distribution rather than a separate WASI SDK.

`COGMASTER_FIXTURES` (on by default; on in `ci`, `ci-msvc`, `asan`, `be32` and `wasm32`) builds WLA-DX and the
fixtures, described in [conformance.md](conformance.md#assembled-fixtures).

`COGMASTER_TIMEOUT_SCALE` multiplies every test timeout: 3 for `asan`, 8 for `be32` and 5 for `wasm32`. The
emulator scales are measured on the hosted runners: the tests a lane shares with the Linux x64 leg took, in total,
6.7 times as long under `qemu-ppc` (the slowest, the libretro host tests, up to 7.3 times) and 4.3 times as long
under wasmtime. Each scale is the largest ratio rounded up; `CMakePresets.json` cites the runs.

### Comparing hashes across lanes

`runner.hashes_file` (`tests/cmake/hashes.cmake`, mode `write`) writes `build/<preset>/hashes.txt`: the frame 60
hash line of the Master System card and of the Game Gear card, then a `fixture <name> sha256 <hex>` line per
assembled fixture image, with Unix newlines on every host. CI collects the
file from each lane, and the `hash-equality` job compares them:

```sh
cmake "-DFILES=<file>;<file>;..." -DCOUNT=7 -DMODE=compare -P tests/cmake/hashes.cmake
```

It passes only for exactly `COUNT` files, all present and byte-identical, and otherwise names the first file that
differs. The fixed count means a lane that stops uploading fails the job instead of silently dropping out. The
seven files are the six `build` legs and `be32`. wasm32 compares its own file with Linux x64's and only reports;
once it is promoted, the count becomes eight.

`ctest --preset ci -R <regex>` reruns part of the suite after a build; `ctest --test-dir build/ci -L <label>`
selects by label.

## The inventories

`tests/inventory/ci.txt` lists every registered test with its labels, one `name<TAB>labels` line each, sorted,
labels comma-separated (`cmake/CogmasterTests.cmake`). Configure fails when the registered tests differ from the
file in either direction, so a test cannot be dropped, renamed or registered on one platform only without the file
changing in the same commit. Every leg registers the same tests; a check only one platform can perform runs there
as a CI step instead. Four lanes have their own list: `hygiene.txt`, `nofp.txt` (the `abi` tests plus the nofp
fixture), `wasm32.txt` (the `ci` list without the `libretro` tests) and `fuzz.txt` (the two timed libFuzzer runs).

| Label | Tests |
|---|---|
| `core` | The library's behaviour: the API contract, sizes, allocation, faults, state, the frame and the test card. |
| `bus` | Images run through the bus: the frame deadline and overshoot bound, the frame origin and the CPU's first fault. |
| `portable` | Tests that hold on every platform, including the frame hashes; the emulated lanes run these. |
| `abi` | The public header compiled alone as C17 and C++17, the API tests, and the core library's symbols. |
| `host` | The palette helper shared by the runner and the core. |
| `runner` | `cogmaster-run` options, output, exit statuses, signals and the PNG writer. |
| `bench` | The runner's bench report. |
| `libretro` | The libretro core in an in-process host, its `.info` file and its vendored header. |
| `install` | The library installed and found by an outside project, and embedded with `add_subdirectory`. |
| `perf` | Allocation-free frames and the instructions-per-frame gate's own logic. |
| `policy` | Rules about the repository, the documents and the tests themselves. |
| `fuzz` | Every generated seed and committed regression replayed through the fuzz entries; on the nightly `fuzz` lane, the timed libFuzzer runs. |
| `fixtures` | The images assembled from `tests/roms/src` with the pinned WLA-DX, their mailbox records judged against `tests/diag/expected.csv`, and the negative controls of the pin and of the judge. |
| `release` | The packaged zips, run after the package step. |
| `hygiene` | The hygiene lane's checks. |

### The JUnit check

A zero exit from CTest does not prove that each test ran. After each test step CI runs, for example:

```sh
cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release \
  -P tests/cmake/check_junit.cmake
cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release \
  -P tests/cmake/check_junit.cmake
```

Each fails unless every selected inventory test appears exactly once, ran, and did not fail, error or skip, with at
least one test in all. `policy.no_skip` fails when any test file sets a skip return code or disables a test.

## Negative controls

A check that cannot fail proves nothing, so each repository check has a self test that feeds it a known fault.
`tests/cmake/expect_fail.cmake` runs a command and passes only when it exits with the expected code and its output
matches the expected message; a crash or a missing file does not count as the expected failure.

| Self test | Fault fed to the check |
|---|---|
| `abi.undefined.self_test` | A library that calls `abort`. |
| `abi.globals.self_test` | A library with a writable file-scope counter. |
| `abi.api_version.self_test` | A copy of the header with one extra function and no API version bump. |
| `abi.nofp_fixture` | On the `nofp` lane, one `double` division, which must fail to compile with GCC's own diagnostic. |
| `docs.api_names.self_test` | A header declaring a call its guide omits, beside a guide naming a call the header lacks. |
| `runner.hash.self_test` | The Master System card's hash checked against the Game Gear pin. |
| `runner.hashes_compare.count.self_test` | Fewer `hashes.txt` files than required. |
| `runner.hashes_compare.byte.self_test` | Two `hashes.txt` files one byte apart. |
| `libretro.info_claims.self_test` | An `.info` file claiming a capability no test covers. |
| `policy.no_skip.self_test` | A test file that sets a skip property. |
| `release.archive_contents.self_test` | A libretro zip without `THIRD-PARTY-NOTICES.md`. |
| `release.floor.self_test` | An impossible floor for the leg's system. |
| `release.single_version_source.self_test` | A repository whose second tracked file repeats its version. |
| `hygiene.tree.self_test` | An untracked file holding a home directory path. |
| `hygiene.no_float.self_test` | Six floating-point lines beside two that only look like them. |
| `hygiene.timeout_comments.self_test` | A workflow with a bare timeout and one whose comment is a guess. |
| `policy.pins.self_test` | A pins row whose digest does not match its file; `fetch-pin.sh` must stop before touching the destination. |
| `policy.title.self_test` | The title `Phase 1: A test frame in RetroArch`. |
| `policy.publish_set.self_test` | 17 of the 18 release zips; `publish.sh prepare` must name the missing one. |
| `policy.ci_required.self_test` | A copy of `ci.yml` whose roll-up ignores `heavy`, so a skipped build would pass. |
| `policy.action_pins.self_test` | An action referenced by a tag. |
| `policy.e2e_budget.self_test` | A 301-line file under the RetroArch test budget. |

The `perf.gate.*` tests feed the instructions-per-frame comparison its edge cases: identical, exactly +1.00 %,
+1.01 %, an allowed exception, unstable bases, no base and a missing measurement. `policy.publish_ref` and
`policy.publish_signer` show that a release verifies only from `refs/heads/main` and only as signed by
`release.yml`, using logging `gh` and `curl` stubs.

## Pinned card hashes

`tests/testcard_pins.cmake` holds the SHA-256 of the Master System and Game Gear test cards' frame 60 crop and of
the runner's PNG of the Master System card. They were computed from the card's rules by an independent script, not
by this code, and every test reads them from that file. They are never re-pinned to whatever the code outputs: a
pin taken from the code would pass any bug the code has. A changed card changes the pins only together with the
behaviour it describes, and the new values come from the same independent computation.

## The core library's symbols

`tests/cmake/abi.cmake` reads the built `cogmaster` library with `nm -P` (`llvm-nm` for MSVC's archives):

- `abi.undefined`: every symbol the library needs and does not define is a C memory function or a compiler helper
  listed in the script for that system;
- `abi.globals`: no writable data symbol, so all state lives in the instance;
- `abi.symbols`: the exported `cogmaster_` names are exactly the functions `include/cogmaster.h` declares, and
  every other exported name is internal.

MSVC's own read-only constant names (string literals and vector constants) and the x64 vectorizer's CPU-check pair
are recognised on Windows only. `abi.api_version` requires every name in the header to appear in some
`tests/api/vK.c` with K at most the header's API version.

## CI

`.github/workflows/ci.yml` runs on every pull request, on each push to `main`, by hand, and when `release.yml`
calls it with a `ref`. Each job runs the same workflow preset you run locally, then the JUnit check with its
lane's inventory and labels. Every action is pinned by full commit SHA.

| Job | Runner | Runs |
|---|---|---|
| `changes` | `ubuntu-26.04` | `scripts/classify-changes.sh` on the pull request's paths: `heavy`, and `release_inputs` |
| `hygiene` | `ubuntu-26.04` | preset `hygiene` |
| `title` | `ubuntu-26.04` | pull requests only: `scripts/check-title.sh` on the live title and body |
| `build` ×6 | Linux, macOS and Windows, x64 and arm64 | preset `ci` (`ci-msvc` on Windows); uploads `archives-<os>-<arch>` and `hashes-<os>-<arch>`; macOS arm64 also runs the README install line |
| `asan` | `ubuntu-26.04` | preset `asan` |
| `nofp` ×2 | `ubuntu-26.04`, `ubuntu-26.04-arm` | preset `nofp` |
| `be32` | `ubuntu-26.04` | preset `be32`; uploads `hashes-be32` |
| `hash-equality` | `ubuntu-26.04` | exactly seven `hashes.txt` files byte-identical |
| `wasm32` | `ubuntu-26.04` | preset `wasm32`; reports whether its `hashes.txt` equals Linux x64's |
| `retroarch` | `macos-15` | the built core in RetroArch (below) |
| `perf` | `ubuntu-26.04` | pull requests and dispatches: Cachegrind instructions per frame (below) |
| `rehearsal`, `cleanup` | `ubuntu-26.04` | every publish step against a throwaway prerelease ([maintaining.md](maintaining.md#the-rehearsal)) |
| `CI required` | `ubuntu-26.04` | the roll-up below |

**The roll-up.** The ruleset requires one check, `CI required`. It always runs and passes only when every job it
needs succeeded, except: the heavy jobs may be skipped when `changes` says `heavy=false`, which happens only when
every changed path is under `.planning/`; `title`, `perf`, `rehearsal` and `cleanup` may be skipped outside pull
requests; `perf`, `rehearsal` and `cleanup` in a nested call that passes `ref`; and `rehearsal` and `cleanup`
when no release input changed. `wasm32` is not among its needs.

**Tests of CI itself** (hygiene lane). `policy.ci_required` runs the roll-up's `jq` program from `ci.yml` on
synthetic job results, and fails when a job other than `wasm32` is missing from its needs. `ci.classify` runs the
classifier on mixed, renamed, empty, planning-only and over-long path lists. `policy.action_pins` rejects an
action not pinned by SHA. `hygiene.timeout_comments` requires each `timeout-minutes` to carry a run ID or the word
`provisional`. A timeout is twice the job's slowest successful run on this repository, rounded up to minutes,
with a 5-minute floor for network-bound and roll-up jobs; its comment names the run and the sample count, and it
stays `provisional` until ten successful runs exist.

**The title.** The squash commit on `main` takes the pull request's title and body, and release-please reads its
type. `title` passes a Conventional Commit title with a lower-case subject, such as `fix(runner): exit 2 on a
missing file`. It fails a scope that starts with a digit, a `Phase 1: Name` title, `feat`, `fix` or `perf` scoped
to pipeline work, and any CI skip token in the title or body. Editing the title reruns it. Try one locally:
`echo | sh scripts/check-title.sh 'fix: x'`.

**Concurrency.** A new push to a pull request cancels that pull request's run in progress. Runs on `main` are
never cancelled, and a call from `release.yml` does not queue behind the push run on the same commit.

## Performance

**Instructions per frame.** The `perf` job counts the instructions `cogmaster-run` executes per frame of the test
card under Cachegrind: Ir/frame = (Ir at 360 frames − Ir at 60) / 300, in integers, with `--cache-sim=no`, an
empty environment and runner paths of equal length, so the count repeats exactly. It measures the pull request's
head commit once and its merge base with the base branch twice, then `tests/cmake/perf_compare.cmake` prints one
line to the log and the job summary:

```text
perf testcard head=2146896 base=2146896 delta_bp=0 verdict=PASS
```

`delta_bp` is the change in basis points. `PASS` means at most 1.00 % slower (exactly +1.00 % passes); `FAIL`
more than that or more than the allow file permits; `UNSTABLE` the two base measurements differ; `NO_BASE` the
merge base has no `bench` preset. An empty or non-numeric measurement is an input error, never a pass.

**Report mode.** The job runs with `COGMASTER_PERF_MODE: report` in `ci.yml`: it prints the verdict and succeeds
whatever the verdict. It is armed by one `ci:` pull request that sets the mode to `gate`, after two control runs
dispatched on `main` show zero spread; on `workflow_dispatch` the job measures one commit three times and prints
`perf control testcard runs=<a>,<b>,<c> spread=<n>`. Once armed, `FAIL`, `UNSTABLE` and `NO_BASE` fail it.

**Exceptions.** A pull request that must be slower adds a line to `tests/perf/allow.txt`:
`<rom> <max % × 100> <expiry YYYY-MM-DD> <reason>`. It raises that ROM's limit through its expiry, at most 60
days away, and is removed after the pull request merges.

**Nightly bench.** `.github/workflows/nightly.yml` runs `cogmaster-run --bench 300000` every night on
`ubuntu-26.04`, `ubuntu-26.04-arm` and `macos-15`, and under wasmtime. Each report is the runner's nine lines
(see [runner.md](runner.md#bench-report)) plus the runner's name; WASI has no resident set size, so wasm32's
`max_rss_kib` reads 0. Reports are trend data in the job summaries and `bench-*` artifacts, never a gate and never
committed. A failed night opens or comments on the one open issue labelled `nightly`; a good night closes it.
Dispatching the workflow with `force_failure` fails a run on purpose, after the reports are uploaded. A pull
request that changes the bench or the workflow runs the bench jobs but writes no issue.

## Fuzzing

Two fuzz targets each define `LLVMFuzzerTestOneInput`: `tests/fuzz/fuzz_loader.c` probes the bytes and loads them
on the profile the probe guesses, and fails when the probe and the load disagree on accepting them;
`tests/fuzz/fuzz_run.c` loads them with fast boot and runs two frames. Their starting inputs are the images
`tests/fuzz/seeds.c` writes into `build/<preset>/tests/seeds` at build time; no corpus is committed.

**Replay, every pull request.** `fuzz.regress.loader` and `fuzz.regress.run` link each entry with a plain `main`
(`tests/fuzz/replay.c`) and replay every seed, and every file in `tests/fuzz/regress` when it exists, on every
lane that runs the `portable` label. Each prints `replayed N` and fails when N is 0. No libFuzzer is needed.

**libFuzzer, nightly.** The `fuzz` job in `nightly.yml` runs `cmake --workflow --preset fuzz` on `ubuntu-26.04`:
clang-21 with `-fsanitize=fuzzer-no-link,address,undefined`, the two targets linked with `-fsanitize=fuzzer`, each
run for 60 s over a fresh copy of the seeds with `-timeout=2 -max_len=1049089` (1 MiB, a copier header and one
byte, so the size-reject path is reachable). A crashing input is uploaded as the `fuzz-artifacts` artifact. The
job is `continue-on-error` until ten green nightly runs. A finding is a harness or core bug to triage, never a
release gate of its own.

**Adding a regression.** Minimise the input (`cogmaster-fuzz-<target> -minimize_crash=1 -runs=10000 <file>`), fix
the bug, commit the minimised file under `tests/fuzz/regress/` and add its `tests/roms/manifest.txt` row
(`scripts/hygiene.sh` rejects a binary file without one). Reconfigure: the replay tests then read the directory.
`docs.budgets` keeps `tests/fuzz` at most 512 KiB.

## Hosted RetroArch

The `retroarch` job proves that a real frontend shows the frame the runner hashes. It is a CI step, not a CTest,
so it never skips itself off macOS. On `macos-15` it:

1. unzips the core and the runner from the macOS arm64 `build` leg's `archives-macos-arm64` artifact;
2. fetches the pinned RetroArch Metal disk image with `scripts/fetch-pin.sh retroarch-dmg`, which checks its
   SHA-256 before anything opens it, copies the app out and checks its bundle version against `pins.txt`;
3. runs `tests/retroarch/run_retroarch.cmake`: RetroArch gets its own `HOME`, `CFFIXED_USER_HOME` and the
   configuration from `tests/retroarch/test.cfg.in`, and starts with `--verbose -L <core>`, no content and
   `--max-frames=60 --max-frames-ss`. The launch is bounded at 50 seconds; a failure prints RetroArch's log;
4. writes the runner's frame with `cogmaster-run --frames 60 --png 60:runner.png`, converts both PNGs to BMP with
   `sips`, builds `tests/retroarch/compare.c`, and requires `256x192, every pixel equal`;
5. runs the negative control `tests/retroarch/flip.cmake`: one byte of pixel (7,3) inverted in a copy, and the
   compare must exit 1 naming that pixel.

Everything under `tests/retroarch` stays within 300 lines in total; `policy.e2e_budget` counts them.

**Rerunning it.** Rerun the job from the run page, or `gh run rerun <run-id> --failed`. On a Mac with a desktop
session the same steps run locally from the job's commands, with `-DRETROARCH` pointing at an installed
`RetroArch.app/Contents/MacOS/RetroArch`.

## Release archives

`release.archive_contents` unpacks the three zips and requires the names `cogmaster-{library,runner,libretro}-
<os>-<arch>.zip` with no version, exactly the expected members with no top-level folder, `LICENSE` and
`THIRD-PARTY-NOTICES.md` byte-identical to the source tree's, and no file containing the source or build
directory. `release.floor` reads the oldest systems the README lists from the binaries (`tests/cmake/floor.cmake`).
On macOS arm64 the README's install line runs verbatim against the locally built zip, and with `-DFLIP=1` it must
refuse a copy with one byte changed:

```sh
cmake -DREADME=README.md -DBASE_URL=file://$PWD/build/ci/ -P tests/cmake/install_line.cmake
```

## One version source

`version.txt` is the only file a person or release-please edits to change the version. CMake generates the
library's version numbers, the core's `display_version` and `cogmaster-run --version` from it.
`release.single_version_source` fails, naming the file, when that version appears in any other tracked file;
`.next` checks the next minor and patch versions, which a release pull request writes. Out of scope are
`version.txt`, the release-please files, `CHANGELOG.md` and `.planning/`. To check one version by hand:
`cmake -DVERSION=<x.y.z> -P tests/cmake/single_version.cmake`.

## Budgets

`docs.budgets` keeps `README.md` under 12 KB, `include/cogmaster.h` under 24 KB, each `docs/*.md` under 40 KB
and `tests/fuzz` at most 512 KiB.
`docs.api_names` requires every public name in the header to appear in [embedding.md](embedding.md), and the
guide to name nothing the header lacks.

## Reproducing a CI failure locally

Open the red job and its first red step:

- **A preset step fails.** The CTest summary at the end names each failed test, with its output above it. Run the
  same preset locally, then `ctest --test-dir build/<preset> -R <test> --output-on-failure`.
- **An "Every test ran" step fails.** A test was missing, ran twice, or failed in the JUnit file. Compare
  `tests/inventory/<lane>.txt` with what the preset registers.
- **`hash-equality` fails.** The message names the first file that differs; the artifact name names the lane.
  Run that lane locally and compare its `build/<preset>/hashes.txt` with `build/ci/hashes.txt`.
- **`CI required` fails with every other job green.** A job was skipped where the roll-up does not allow it; its
  log prints each job's result.

The Linux lanes run on any Linux host, or in a container on a Mac, with CMake 4.3 or later, Ninja and the
lane's packages. The presets need CMake 4.3, which is newer than the `cmake` package of `ubuntu:26.04`, so take
CMake from its project's own release there. On Ubuntu the extra packages per lane are:

- `asan`: `clang-21 libclang-rt-21-dev`;
- `nofp`: `gcc g++`;
- `be32`: `gcc-powerpc-linux-gnu g++-powerpc-linux-gnu qemu-user`;
- `wasm32`: `clang-21 lld-21 wasi-libc libclang-rt-21-dev-wasm32`, and
  `scripts/fetch-pin.sh wasmtime-<arch>-linux build/pins/wasmtime` before the workflow (`<arch>` is `x86_64` or
  `aarch64`).

In a container, mount the source tree and put an anonymous volume over `build`, so the container's build stays
apart from the host's: `docker run --rm -v "$PWD:/src" -v /src/build -w /src ubuntu:26.04 ...`.

## Pinned downloads

`pins.txt` lists every file a build or a CI job downloads, one `name version url sha256` row each.
`scripts/fetch-pin.sh NAME DEST` downloads row `NAME` into a temporary directory and checks its SHA-256 before
anything else happens; only then does it unpack an archive into `DEST` or copy the file there. `policy.pins`
requires four fields per row, a unique name, an `https://` URL and a 64-digit digest. Packages installed with apt
stay unpinned: they are signed, and drift fails loudly as a build failure or a hash change.
