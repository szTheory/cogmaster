# Testing

Every check CI runs, a contributor can run with one command. This page covers that command, the lanes, the test
inventories, the negative controls, the pinned hashes and how to reproduce a CI failure. The hosted RetroArch check
is in [hosted.md](hosted.md), the performance job and fuzzing in [perf-fuzz.md](perf-fuzz.md), and the save tests in
[saves.md](saves.md). Releases and repository settings are in [maintaining.md](maintaining.md).

## The entry point

For private dumps, put a literal directory in `COGMASTER_ROM_DIR` in ignored `.env.local`, then run
`sh scripts/library.sh` from the repository root. This invokes the local `library` configure/build/test
workflow; an exported variable takes precedence. Direct `cmake --workflow --preset library` needs the
variable already exported. CI runs `library.synthetic` with owned temporary images, never `library.local`.
No library build directory is uploaded. Local per-file results stay in ignored `build/library/results/`.
The run path refuses, with exit status 2, a tree with tracked changes outside `.planning`, so the ledger's `core=`
stamp always names a commit.

`sh scripts/library.sh diagnose` groups the failing carts by cause (see [conformance.md](conformance.md)). It reads
`COGMASTER_ROM_DIR` as above, writes only `build/library/diag/diagnose.txt` (`COGMASTER_LIBRARY_OUT` moves that
directory, but only inside `build/`) and keeps its scratch files in one directory that traps remove on exit, on
SIGINT, SIGTERM and SIGHUP; only `SIGKILL` leaves that directory behind. CI runs the `library.diagnose` tests over owned or generated
images: `library.diagnose` plants a name, a path fragment, a directory and tail bytes into synthetic images and scans
stdout, stderr and every kept file for them, for the image digests, and for any byte, slash or hex run the closed
grammar does not allow; `library.diagnose.taxonomy` pins every group, threshold and order over hand-written runner
lines; `library.diagnose.leak_name`, `.leak_path`, `.leak_bytes`, `.leak_digest` and `.leak_title` plant each kind of
leak and must fail the scan with its own message; `library.diagnose.hostile` gives carts a markup title and an escape
and right-to-left title (withheld, and excluded before it is read) and a runner that prints a path and image bytes, and expects only `malformed` to change.

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


**Reconfigure after committing.** Some checks read git's list of tracked files (the tree scan, the notices check,
the single version source), and some copy a tracked file into the build tree when CMake configures (the `CI
required` fixture copies `ci.yml`). A new file is seen only once git tracks it, and a configure-time copy is only
as fresh as the last configure. After adding or committing files, rerun the whole workflow preset, which
reconfigures, rather than `ctest` alone.

Tree hygiene scans text for private paths, addresses and forbidden licences. Binary images still need a
manifest entry and pass the image-name/content checks; their arbitrary bytes are not interpreted as text.
`hygiene.tree.binary_manifest` and `hygiene.tree.binary_unlisted` prove both sides with synthetic binary data.
MSVC builds retain ISO C stdio through `_CRT_SECURE_NO_WARNINGS`, with all other selected warnings still fatal.

The Linux x64 build lane first checks out the pull request's exact base SHA and copies its ledger, public
behaviour revision, ratchet and adjacent shared parser into the runner's temporary directory. After checking
out and testing the candidate, that copied judge compares base passes to the committed ledger and compares
the ledger's eligible own-fixture cases to freshly executed results in both directions. Improvements must be
committed. The step prints the actual base/head SHAs and run URL; its failure fails the existing build job and
`CI required`. `compat.trusted_judge` proves that candidate judge/helper replacements cannot change the copied
judge's decision, and tests missing/extra results and status/value disagreements. Results exclude unevaluated
and non-fixture metadata. `compat.ratchet` exercises deletion, aliases, adjacent cases, script digests,
profiles, devices and dated revision exceptions. Both run in CI and hygiene.

When the base has no ratchet, CI explicitly reports first introduction, validates candidate tests and fresh
results with the candidate judge, and establishes protected-baseline comparison after merge. This case does
not claim that an absent baseline judge ran. To reproduce a protected comparison locally, use
`sh scripts/compat-ratchet.sh BASE.csv HEAD.csv ALLOW.txt YYYY-MM-DD BASE_REV HEAD_REV RESULTS.csv`.
While the base ledger has the format 1 header and the head has the format 2 header, the step reports a format
migration and runs the head's judge, which reads both formats; once the headers match this never applies. A pull
request runs its own ci.yml, so this adds no power beyond a ci.yml edit, which already forces the rehearsal, and the
judge reads the catalog and reasons of the checkout it judges. The comparison guards against accidental or unreviewed
flips, not against an author who can edit those or ci.yml; the merge itself is the owner's.

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
| `fuzz` | The nightly libFuzzer runs of the loader, two-frame execution, raw save import and the boot ROM image under clang-21's ASan and UBSan (see [Fuzzing](perf-fuzz.md#fuzzing)). No fixtures, no libretro core. | `fuzz` | `fuzz` |
| `vectors-full` | The nightly run of all 1,604,000 Z80 vectors, and that the committed sample is what the converter writes from the pinned archive (see [conformance.md](conformance.md#the-nightly-full-run)). Fetch the archive first. | `vectors-full` | `vectors` |
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

Playing rows cover the owned boot fixture with released input, each pad separately,
both pads together and native GG Start. SMS1/SMS2 run NTSC export, NTSC Japan and PAL;
native GG and GG SMS mode run the two NTSC regions. The three line fixtures cover
192/224/240-line output on those profiles. PAL Game Gear profiles remain refused.
Every `.sms` and `.gg` fixture also contributes its image SHA-256. Hashes serialize
the crop's RGB444 words little-endian; host aspect ratios and colour conversion do
not enter them.
`core.no_alloc_after_load` uses loaded content across supported machine/TV/video
profiles with input, VRAM/CRAM/register writes, interleaved instances, bounded sprite
scans and state round trips. State-field refusal sweeps also exercise enabled,
populated video. Fuzz replay checks allocation counts, raster bounds and the fixed
deadline with at most 69 ticks of instruction overshoot.

`runner.hashes_file` (`tests/cmake/hashes.cmake`, mode `write`) writes `build/<preset>/hashes.txt`: the frame 60
hash line of the Master System card and of the Game Gear card, then a `fixture <name> sha256 <hex>` line per
assembled fixture image, with Unix newlines on every host. CI collects the
file from each lane, and the `hash-equality` job compares them:

```sh
cmake "-DFILES=<file>;<file>;..." -DCOUNT=8 -DMODE=compare -P tests/cmake/hashes.cmake
```

It passes only for exactly `COUNT` files, all present and byte-identical, and otherwise names the first file that
differs. The fixed count means a lane that stops uploading fails the job instead of silently dropping out. The
eight files are the six `build` legs, `be32` and `wasm32`.

The V counter and Pause fixtures paint nothing, so a `mailbox` row carries the SHA-256 of their printed records on each
TV and the same bytes are judged against `tests/diag/expected.csv`. A device row may end in `|<region>`; the Pause images
use `pal`.

The file also carries the sound rows; what they hold and how to regenerate their pins is in [audio.md](audio.md#the-frozen-sound-hashes).

`ctest --preset ci -R <regex>` reruns part of the suite after a build; `ctest --test-dir build/ci -L <label>`
selects by label.

## The inventories

`tests/inventory/ci.txt` lists every registered test with its labels, one `name<TAB>labels` line each, sorted,
labels comma-separated (`cmake/CogmasterTests.cmake`). Configure fails when the registered tests differ from the
file in either direction, so a test cannot be dropped, renamed or registered on one platform only without the file
changing in the same commit. Every leg registers the same tests; a check only one platform can perform runs there
as a CI step instead. Five lanes have their own list: `hygiene.txt`, `nofp.txt` (the `abi` tests plus the nofp
fixture), `wasm32.txt` (the `ci` list without the `libretro` tests), `fuzz.txt` (the timed libFuzzer runs) and
`vectors-full.txt` (the seven full vector shards and the sample check).

| Label | Tests |
|---|---|
| `core` | The library's behaviour: the API contract, sizes, allocation, faults, state, the frame and the test card. |
| `bus` | Images run through the bus: the frame deadline and overshoot bound, the frame origin and the CPU's first fault. |
| `portable` | Tests that hold on every platform, including the frame hashes; the emulated lanes run these. |
| `abi` | The public header compiled alone as C17 and C++17, the API tests, and the core library's symbols. |
| `host` | The palette helper and the anaglyph mixer shared by the runner and the libretro core. |
| `ports` | The controller ports: the pin layer, `set_input`, the Sports Pad, the Paddle, the 3-D eye, and the saved device state ([controllers.md](controllers.md)). |
| `runner` | `cogmaster-run` options, output, exit statuses, signals and the PNG writer. |
| `bench` | The runner's bench report. |
| `libretro` | The libretro core in an in-process host, its `.info` file and its vendored header. |
| `install` | The library installed and found by an outside project, and embedded with `add_subdirectory`. |
| `perf` | Allocation-free frames and the instructions-per-frame gate's own logic. |
| `policy` | Rules about the repository, the documents and the tests themselves. |
| `z80` | Z80 behaviour the vectors do not cover: interrupts, NMI, the EI delay, HALT, prefix runs and the undefined ED opcodes. |
| `vectors` | The Z80 vector sample, one shard per prefix group, and the converter's and harness's own checks; on the nightly `vectors-full` lane, every vector. |
| `vdp` | The VDP's control and data ports, their decoding, the status byte, the H and V counters and the Pause NMI. |
| `mapper` | The Sega and Codemasters mappers and the mapperless layout: banking, wrap, padding, mirrors and cartridge RAM. |
| `cart` | The image probe: its decision branches, the size rules, and that it changes nothing. |
| `io` | The I/O ports: decoding and mirrors, `$3E`, the region bits of `$3F`, `$DC` and `$DD`, open bus and the Game Gear's ports. |
| `fuzz` | Every generated seed and committed regression replayed through the fuzz entries; on the nightly `fuzz` lane, the timed libFuzzer runs. |
| `fixtures` | The images assembled from `tests/roms/src` with the pinned WLA-DX, their mailbox records judged against `tests/diag/expected.csv`, and the negative controls of the pin and of the judge. |
| `release` | The packaged zips, run after the package step. |
| `hygiene` | The hygiene lane's checks. |

### The JUnit check

A zero exit from CTest does not prove that each test ran. After each test step CI runs, for example:

```sh
cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release \
  -DCHECK=check_junit -P tests/cmake/repository.cmake
cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release \
  -DCHECK=check_junit -P tests/cmake/repository.cmake
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
| `hygiene.timeout_comments.self_test` | Five timeouts that are bare, a guess, provisional, under twice their run or a floor under 5 minutes. |
| `policy.pins.self_test` | A pins row whose digest does not match its file; `fetch-pin.sh` must stop before touching the destination. |
| `policy.title.self_test` | The title `Phase 1: A test frame in RetroArch`. |
| `policy.publish_set.self_test` | 17 of the 18 release zips; `publish.sh prepare` must name the missing one. |
| `policy.ci_required.self_test` | A copy of `ci.yml` whose roll-up ignores `heavy`, so a skipped build would pass. |
| `policy.ci_required.actor_self_test` | A copy of `ci.yml` whose roll-up accepts a skipped rehearsal from any author. |
| `policy.action_pins.self_test` | An action referenced by a tag. |
| `policy.e2e_budget.self_test` | Two files totaling 301 lines under the RetroArch test budget. |
| `policy.firmware_scan.*` | A zip holding a file with an injected SHA-1, a copier-headered member, a cartdb or manifest row, an empty directory and a zero-row table fail; a clean zip, a loose headered file and a misaligned `00` hex match pass. |
| `policy.firmware_window.*`, `hygiene.byte_run.*` | A planted 32-byte window fails (also in a C array, a non-ASCII file name, a libretro module and a release zip), a 31-byte one passes; an unreadable target, a zip that will not open, an unusable or empty dump directory fail; 300 byte literals fail, 255 pass. |
| `cart.generator.catalog` | A bidi, control-byte, leading `=` or invalid title, a pin without its licence, and a count that is only a claim. |
| `compat.reasons` | A catalog reason the vocabulary lacks or has retired; a vocabulary that drifts from the slots with no board. |
| `compat.whole_list.count` | One identity removed from the ledger, a typed count one low, an id the catalog lacks. |
| `compat.whole_list.fresh` | A catalog line deleted, an `unknown` hand-flipped to `unsupported`, a stale derived field. |
| `compat.whole_list.fresh.self_test` | The four faults above fed to the freshness check, which must name each. |
| `compat.whole_list.reason` | An empty or unlisted reason, `unsupported` on a supported board, `pass` on a board with none, a catalog line unlike its fact, or a fact with no line. |
| `compat.whole_list.runnable` | A seeded profile the core refuses (a Game Gear with a paddle exits 77). |
| `compat.hardening` | A title that spoofs direction, hides characters or opens as a formula; a markdown title that becomes a link; 8,193 rows. |
| `compat.page_bound` | A page bounded only because today's ledger is small. |
| `compat.page_bound.self_test` | A copy with its caps raised, which must exceed the page budget. |
| `compat.exit` | A ledger no parser reads, an `unsupported` row with no reason, a fixture `fail`, a floor that drops, a flag that stays or goes against the rows. |
| `compat.exit.self_test` | A met verdict nobody has taken, a tie that passes both ways, 19 passes, 20 fixture or compound passes. |
| `policy.claims_scoped` | An over-claim in the README, the `.info` or the page, a planning label (`v2.0`) in user text, a leftover of the removed status label. |
| `policy.claims_scoped.self_test` | One tree per fault above, beside a passing flag-absent tree. |
| `docs.budgets.ratio_equal` | One physical check line against ten core lines, including unterminated final lines. |
| `docs.budgets.unclassified` | A checker omitted from the responsibility inventory. |
| `docs.budgets.helper` | An additional counted compatibility helper exceeds an otherwise passing ratio. |
| `docs.budgets.helper_unclassified` | An unclassified compatibility parser helper beside a classified checker. |

The window controls run only in the Linux `hygiene` lane; `local.firmware_window_scan` runs in the `library` preset and says it compared nothing when no dump is set. The window scan misses base64, decimal lists, XOR, compression, `xxd` and `hexdump -C` layouts and a dump kept as a zip. A nested archive in a release zip is not re-extracted (`release.archive_contents` pins each member set); a padded dump of the same BIOS passes the release scan and only the window scan catches it.

`docs.budgets.ratio_below` supplies the passing boundary: one check line against eleven core lines. Both `ci`
and `hygiene` execute these cases and `docs.budgets`; the equality and omission cases require the intended
nonzero status and diagnostic. The hosted-size negative control totals two files, including a nested helper
whose last line has no newline, to prove the complete helper tree counts.

`tests/policy/repository-checks.txt` classifies every implementation under `tests/cmake`, `scripts`, `cmake`
and `.githooks` by responsibility. The `check` rows include the complete shared CMake/POSIX scanner, core ABI,
JUnit, inventory, negative-control and hook helpers. Every physical line counts, including comments, blanks
and an unterminated last line. Their sum multiplied by ten must be strictly less than the physical C/H lines
under `src`; neither rounding nor an exemption for an older checker applies. Unclassified code fails first,
so new compatibility parser and judge scripts cannot silently escape the numerator.

Build/bootstrap operations, remote administration and release publication, fixture execution, packaged-output
integration tests and the CI job router have individual reasons for exclusion in that file. The archive/floor
tests still execute their positive and negative cases in `ci-release`; their responsibility is installed
deliverable contents and loader compatibility. Registration and fixture construction are test cases rather
than checker implementations. Core ABI policy and every shared assertion helper remain counted in full.
`repository.cmake` dispatches the source policy checks to one POSIX scanner; established test names identify
their faults. `policy.e2e_budget` counts every file under `tests/retroarch` and its shared pixel oracle
`tests/fixtures/play.h`, at most 300 lines. Its shared-helper control rejects 280 driver plus 21 oracle lines.

The `perf.gate.*` tests feed the instructions-per-frame comparison its edge cases: identical, exactly +1.00 %,
+1.01 %, an allowed exception, unstable bases, no base and a missing measurement. `policy.publish_ref` and
`policy.publish_signer` show that a release verifies only from `refs/heads/main` and only as signed by
`release.yml`, using logging `gh` and `curl` stubs.

The controller path's tests and what each guards are listed in [controllers.md](controllers.md#tests).

## Pinned card hashes

`tests/testcard_pins.cmake` holds the SHA-256 of the Master System and Game Gear test cards' frame 60 crop and of
the runner's PNG of the Master System card. They were computed from the card's rules by an independent script, not
by this code, and every test reads them from that file. They are never re-pinned to whatever the code outputs: a
pin taken from the code would pass any bug the code has. A changed card changes the pins only together with the
behaviour it describes, and the new values come from the same independent computation.

## The core library's symbols

`tests/cmake/repository.cmake` with `CHECK=abi` reads the built `cogmaster` library with `nm -P` (`llvm-nm` for MSVC's archives):

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
[CI speed](maintaining.md#ci-speed) says where a run's time goes and what to do about a failed runner setup step.

| Job | Runner | Runs |
|---|---|---|
| `changes` | `ubuntu-26.04` | `scripts/classify-changes.sh` on the pull request's paths: `heavy`, and `release_inputs` |
| `hygiene` | `ubuntu-26.04` | preset `hygiene` |
| `title` | `ubuntu-26.04` | pull requests only: `scripts/check-title.sh` on the live title and body |
| `build` ×6 | Linux, macOS and Windows, x64 and arm64 | preset `ci` (`ci-msvc` on Windows); uploads `archives-<os>-<arch>` and `hashes-<os>-<arch>`; macOS arm64 also runs the README install line |
| `asan` | `ubuntu-26.04` | preset `asan` |
| `nofp` ×2 | `ubuntu-26.04`, `ubuntu-26.04-arm` | preset `nofp` |
| `be32` | `ubuntu-26.04` | preset `be32`; uploads `hashes-be32` |
| `hash-equality` | `ubuntu-26.04` | exactly eight `hashes.txt` files byte-identical |
| `wasm32` | `ubuntu-26.04` | preset `wasm32`; uploads `hashes-wasm32` for `hash-equality` |
| `retroarch` | `macos-15` | the built core in RetroArch ([hosted.md](hosted.md)) |
| `perf` | `ubuntu-26.04` | pull requests and dispatches: Cachegrind instructions per frame ([perf-fuzz.md](perf-fuzz.md)) |
| `rehearsal`, `cleanup` | `ubuntu-26.04` | every publish step against a throwaway prerelease ([maintaining.md](maintaining.md#the-rehearsal)) |
| `CI required` | `ubuntu-26.04` | the roll-up below |

**The roll-up.** The ruleset requires one check, `CI required`. It always runs and passes only when every job it
needs succeeded, except: the heavy jobs may be skipped when `changes` says `heavy=false`, which happens only when
every changed path is under `.planning/`; `title`, `perf`, `rehearsal` and `cleanup` may be skipped outside pull
requests; `perf`, `rehearsal` and `cleanup` in a nested call that passes `ref`; and `rehearsal` and `cleanup`
when no release input changed or the pull request's author is `dependabot[bot]`, whose read-only token cannot run
the rehearsal.

**Tests of CI itself** (hygiene lane). `policy.ci_required` runs the roll-up's `jq` program from `ci.yml` on
synthetic job results, and fails when any job is missing from its needs. `ci.classify` runs the
classifier on mixed, renamed, empty, planning-only and over-long path lists. `policy.action_pins` rejects an
action not pinned by SHA. `hygiene.timeout_comments` requires each `timeout-minutes` comment to read `2 x S s,
slowest of N runs, run <id>` with the timeout at least twice S, or `5-minute floor; no runs yet` at 5 minutes or
more, and rejects the word `provisional`. A timeout is twice the slowest of the last 40 successful runs, or all of
them if fewer, rounded up to minutes, with N stated (matrix lanes of one run count once). Get the numbers from
`sh scripts/ci-durations.sh <workflow>`, which lists the jobs slowest first and needs `gh` and `jq`. Network-bound
and roll-up jobs have a 5-minute floor. A workflow with no runs yet uses `5-minute floor; no runs yet` until its
first run is recorded.

**The title.** The squash commit on `main` takes the pull request's title and body, and release-please reads its
type. `title` passes a Conventional Commit title with a lower-case subject, such as `fix(runner): exit 2 on a
missing file`. It fails a scope that starts with a digit, a `Phase 1: Name` title, `feat`, `fix` or `perf` scoped
to pipeline work, and any CI skip token in the title or body. Editing the title reruns it. Try one locally:
`echo | sh scripts/check-title.sh 'fix: x'`.

**Concurrency.** A new push to a pull request cancels that pull request's run in progress. Runs on `main` are
never cancelled, and a call from `release.yml` does not queue behind the push run on the same commit.

Performance and fuzzing are in [perf-fuzz.md](perf-fuzz.md). The hosted RetroArch check and the content, rendering
and input tests are in [hosted.md](hosted.md).

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

`docs.readme_play` runs the README's first runner command against the built `boot_to_play.sms` and compares the
printed line with the README's text block. It also requires every ROM extension the README names to be in the
core's `supported_extensions` and the no-BIOS sentence to appear once; its `.self_test` names a refused `.sg`.

## One version source

`version.txt` is the only file a person or release-please edits to change the version. CMake generates the
library's version numbers, the core's `display_version` and `cogmaster-run --version` from it.
`release.single_version_source` fails, naming the file, when that version appears in any other tracked file;
`.next` checks the next minor and patch versions, which a release pull request writes. Out of scope are
`version.txt`, the release-please files, `CHANGELOG.md` and `.planning/`. To check one version by hand:
`cmake -DCHECK=single_version -DVERSION=<x.y.z> -P tests/cmake/repository.cmake`.

## Budgets

`docs.budgets` keeps `README.md` under 12 KB, `include/cogmaster.h` under 24 KB, each `docs/*.md` under the
`docs.max_bytes` ceiling in `quality.json` and `tests/fuzz` at most 512 KiB.
`policy.quality` checks the two test-count floors in that file against the inventories, with controls for each
failure and for the base-file ratchet; `policy.lessons_unique` fails a lesson ID that appears twice or not at all.
The zip and wasm size ceilings are checked by the build legs and the `wasm32` lane, see [maintaining.md](maintaining.md).
`docs.api_names` requires every public name in the header to appear in at least one of
[embedding.md](embedding.md), [saves.md](saves.md) and [boards.md](boards.md), and those guides to name nothing the
header lacks.

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

A transient network error (curl exit 6, 7, 18, 28, 35, 52, 55 or 56) is retried up to three calls, in `fetch-pin.sh` and in
the assembler download of `cmake/WlaDx.cmake`; any other error fails on the first. The fetch-retry and dirty-tree tests run in
the ci lane on every build leg against a refused local port, missing or tampered `file://` URLs, a fake `curl` and
throwaway repositories, so none of them downloads anything real.

The in-process presentation cases exercise RGB565 and XRGB8888, all active heights,
native GG and SMS compatibility, default and square pixels, full field, rejected
geometry notifications, content-load-only overrides and exact PAL audio totals.
They compare each delivered pixel against a separate public-library instance.

`libretro.host.audio.*` play `psg_sweep` and `boot_to_play` on both machines through the module in three sessions:
batch callback only, single-sample callback only and both (the single one must stay unused). The expected hash is
the runner's `--hash-audio` hash of the same image, taken in the same test. The host hashes little-endian bytes,
mono on the Master System and interleaved on the Game Gear, requires 801 or 802 frames each and running totals
equal to `floor(frames x 3,504,512 / 4,375)`, and the per-channel hashes equal across sessions.
`libretro.host.fm_option` compares the FM option's three values with the runner's hashes on each profile.

The save seeds include 0, 1, 32767, 32768 and 32769 bytes and each persistence refusal
class. The ordinary CI and ASan replay runs include them. `battery.import` also covers
0, 1, N-1, N, N+1 and SIZE_MAX for all supported 8/16/32 KiB spans, overlapping sources,
NULL arguments and byte-identical whole-state refusal. `battery.spans`, `battery.generation`
and `battery.excluded` cover physical aliases, same-value writes, reset/pointer stability
and generation exclusions, with exact executed inventory entries on native and WASI lanes.
