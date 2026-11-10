# Maintaining cogmaster

How a release is made, rehearsed, re-run and withdrawn, how the release App and the repository's settings are set
up and checked, and how the pinned downloads move. Tests and CI are in [testing.md](testing.md).

Execution hands off to code review before phase verification. Keep review fixes to at most two rounds and
allow one gap round only when new evidence warrants it. Test/tooling review findings become later seeds.
Automate owned-fixture and frontend checks; no routine owner UAT is required. Do not chain GSD steps.

## How a release happens

Nobody tags or uploads by hand. `.github/workflows/release.yml` runs on every push to `main` and does each step:

1. A pull request is squash-merged into `main`. Its title becomes the commit subject, and release-please reads
   that Conventional Commit to decide the next version: before 1.0, `feat` and a breaking change bump the minor
   version, `fix` and `perf` bump the patch, and `docs`, `chore`, `ci`, `test`, `build`, `refactor` and `style`
   cut no release. Write `ci:`, not `fix(ci):`, for pipeline work; the `title` check refuses the latter.
2. The `release-please` job mints a token for the release App, then opens or updates the release pull request,
   which edits only `version.txt`, `.release-please-manifest.json` and `CHANGELOG.md`. When it creates that pull
   request, the same job turns on squash auto-merge for it with the App's token.
3. The release pull request runs CI like any other, because the App opened it. GitHub merges it, as the App, once
   `CI required` passes.
4. That merge's push runs `release.yml` again. release-please now tags the merge commit and creates a draft
   release. The first release is the first minor version, whatever the commit types.
5. `release-ci` runs the whole of `ci.yml` on the release's commit, the `sha` release-please reports, never the
   commit that started the run.
6. `publish` runs `scripts/publish.sh` on the draft, found by its release ID:
   - `prepare`: exactly the 18 zips, every build leg's recorded commit equal to the release commit, `SHA256SUMS`;
   - `upload`, then `actions/attest` over `SHA256SUMS`;
   - `attested`: each zip's attestation verifies from `refs/heads/main`, and a copy with one byte flipped does not;
   - publish the draft as the latest release;
   - `verify`: `attested` again, `gh release verify`, 19 non-empty assets, and every zip downloaded from the
     release's `releases/download/<tag>/` URL matches `SHA256SUMS` and verifies as signed by `release.yml` from
     `refs/heads/main`.
7. `install-line-latest` runs the README's install line verbatim on `macos-15` against
   `releases/latest/download/`.

Only an attestation from `release.yml` on `refs/heads/main` is release provenance. A rehearsal signs from
`ci.yml` on a pull request's merge ref, so a check that a download is a release pins both:

```sh
gh attestation verify cogmaster-runner-linux-x64.zip --repo szTheory/cogmaster \
  --signer-workflow szTheory/cogmaster/.github/workflows/release.yml --source-ref refs/heads/main
```

Release runs queue behind each other and are never cancelled. A version moves to 1.0.0 only through a final
`Release-As: 1.0.0` line in the body of the pull request that should cut it.

## The rehearsal

A same-repository pull request that changes a release input (`release.yml`, `ci.yml`, `scripts/publish.sh`,
`release-please-config.json` or `cmake/packaging.cmake`) runs every publish step inside `ci.yml`, and so inside
`CI required`, before it can merge:

- `rehearsal` checks that the App is configured (it fails naming `COGMASTER_APP_CLIENT_ID` and
  `COGMASTER_APP_PRIVATE_KEY` otherwise), mints the App token, gathers the six build legs' archives from the
  pull request's merge commit, and runs `prepare`, `rehearsal-create rehearsal-<run id>-<attempt>`, `upload`,
  `actions/attest`, `attested`, a publish as a prerelease that is never latest, and `verify`;
- `cleanup` runs whatever the rehearsal did, once it started, and deletes the release and its tag.

Only the trigger, the signer workflow, the source ref and the `latest` flag differ from a real release, and
`policy.publish_ref` and `policy.publish_signer` keep a release from ever accepting a rehearsal's attestation. Each
attempt gets a new tag, because a deleted tag of a published immutable release can never be reused. A pull request
from a fork has no secrets, skips the rehearsal and so fails `CI required` when it changes a release input:
rehearse such a change from a branch in this repository.

After any rehearsal, `gh release list` shows no `rehearsal-*` release and
`git ls-remote --tags origin 'rehearsal-*'` prints nothing.

## Re-running a failed release

- **A real release:** open the failed run and choose "Re-run failed jobs". `publish` is safe to re-run: uploads
  replace the draft's assets, and on a release already published it only runs `verify`, which exits 0 when the
  release has its 19 verified assets. Do not choose "Re-run all jobs": release-please then finds the release
  already tagged and skips `publish`.
- **A failed rehearsal:** choose "Re-run all jobs", which takes a fresh `rehearsal-<run id>-<attempt>` tag, or
  push a new commit. If `cleanup` itself failed, delete the leftover by hand:
  `gh release delete rehearsal-<run id>-<attempt> --cleanup-tag --yes`. `publish.sh` deletes only `rehearsal-*`
  tags.

## Closing a phase

Every phase ends in a release, and the release pull request is never held. Before a phase is called done:

- no pull request labelled `autorelease: pending` is open (`gh pr list --label 'autorelease: pending'` prints
  nothing), so no release is stuck behind a red check;
- the latest release is the expected version with 19 assets:
  `gh release view --json tagName,assets --jq '.tagName, (.assets | length)'`.

## Withdrawing a release

A published release is immutable, so a bad one is withdrawn and fixed forward:

1. Point `latest` back at the last good release: `gh release edit v<good> --latest`.
2. Edit the bad release's notes to "Withdrawn: <reason>; use v<good>".
3. Fix forward with a `fix:` pull request, which cuts the next patch release.
4. Delete a release only when its content must not be distributed. Its tag name is then gone for good.

## Release App and secrets

The GitHub App `cogmaster-release`, installed on this repository only, mints the tokens that open and merge
release pull requests and publish releases. Events caused by the default `GITHUB_TOKEN` start no workflow runs,
so a release pull request opened with it would never get its required check, and its merge would never publish.
An App token does both, is scoped to this one repository and acts as its own bot. Its repository permissions are
Contents, Pull requests, Issues and Workflows read and write, and Metadata read; each workflow narrows its token
further.

CI holds one variable and one secret:

- `COGMASTER_APP_CLIENT_ID`, an Actions variable;
- `COGMASTER_APP_PRIVATE_KEY`, stored twice: as an Actions secret and as a Dependabot secret, because a
  Dependabot-triggered run sees only Dependabot secrets.

**Setup.** The owner runs `scripts/setup-release-app.sh` once. It needs `gh` with admin rights on the repository,
`jq`, `curl`, `openssl`, `python3` and a browser. It creates the App through GitHub's manifest flow, sets the
variable and both secrets straight from GitHub's response (the key is never printed or written to disk), opens the
installation page preselected to this repository, and waits until a token signed with the key reads the
installation. It stops early if the variable is already set; `--force` creates a new App.

**Rotation.** On the App's settings page generate a new private key, then run
`gh secret set COGMASTER_APP_PRIVATE_KEY < <key file>` and
`gh secret set COGMASTER_APP_PRIVATE_KEY --app dependabot < <key file>`, delete the old key on the settings page,
and delete the key file. Only the owner handles the key. To replace the App itself, rerun
`scripts/setup-release-app.sh --force` and uninstall the old App.

## Dependabot

`.github/dependabot.yml` updates the pinned GitHub Actions weekly, with a seven-day cooldown and the `ci` commit
prefix. Minor and patch updates arrive as one pull request in the `minor-and-patch` group; each major update is
its own pull request and waits for review.

`.github/workflows/dependabot-automerge.yml` runs on `pull_request` only, never checks code out, and acts only
for `dependabot[bot]`. It enables squash auto-merge for the `minor-and-patch` group and for a single update whose
old and new versions share a major; any other title is left for review, and the log says why. A required check
still has to pass before the merge. It reuses the release App key and client ID described above, so the owner has
nothing to set up. `policy.dependabot` fails a group that holds majors or is renamed, and a workflow that uses a
privileged trigger or a checkout.

## Weekly digest

`.github/workflows/digest.yml` runs on Monday and reports on CI without anyone asking. It reads the week's
workflow runs, each run's jobs and the earlier attempts of runs that later passed, and `scripts/digest.sh` turns
them into the digest: run count and job minutes per workflow, the slowest job, flaky jobs (a job that failed an
earlier attempt of a run that then passed), open `nightly` issues, and the attention items. An attention item is
an open nightly issue, a red or seven-day-idle pull request, or a job whose slowest run of the week is more than
half its `timeout-minutes`, which means the 2x margin is spent. The timeout join uses the display name
`scripts/ci-durations.sh` prints (a matrix suffix removed), so `CI required` is matched, not its key
`ci-required`.

The digest goes to the job summary and into the one open issue labelled `digest`, which is rewritten each week
and created when none is open. A comment is added only when the `attention:` count is above 0; a green week
stays silent. To see the digest without writing anything, dispatch the workflow with `dry_run`:
`gh workflow run digest.yml -f dry_run=true`. GitHub dispatches only a workflow that is on the default branch.

A red pull request is read from the workflow runs on its head commit, not from `statusCheckRollup`, which would
need the Checks read and Commit statuses read permissions. The job holds only contents read, actions read,
pull requests read and issues write, and uses no App token. `policy.digest` fails a widened permission, a
schedule other than one Monday cron, a missing `dry_run` input, an App token or a privileged trigger.

## Repository settings

`scripts/apply-repo-settings.sh` holds the repository's settings as code, and `.github/rulesets/main.json` holds
the branch ruleset. The script applies, idempotently:

- squash merging only, with the pull request's title and body as the commit message, auto-merge, branch deletion
  on merge, branch updates, and no wiki or projects. The body, not the list of commits, keeps a CI skip token in a
  branch commit from landing on `main` and suppressing its CI and release runs;
- secret scanning with push protection, private vulnerability reporting, Dependabot alerts and security updates;
- Actions pinned by full commit SHA, a read-only default token, and no pull request approval by Actions;
- immutable releases;
- the eight labels `bug`, `compatibility`, `enhancement`, `docs`, `performance`, `needs-info`, `nightly` and
  `digest`;
- the ruleset, replaced by name: no deletion or force push on the default branch, linear history, pull requests
  with no required approval and squash merging only, and the `CI required` check from GitHub Actions, with no
  bypass.

The owner runs it with admin rights, once after creating the repository and again at each milestone tune-up. The
ruleset blocks every pull request until a run has reported `CI required`, so before CI has ever reported, run it
with `--no-ruleset`, and run it again without that flag once a run has reported the check.

`scripts/apply-repo-settings.sh --check` only reads. It compares every setting above, each label's colour and
description, the App's variable and secrets, and the live ruleset with the committed file. It prints one `drift:`
line per difference and exits 1 if there was any (`no drift on <repository>` otherwise). Run it at each tune-up
and after changing anything on the repository's settings page. Change a setting in the script, not by hand, then
apply it.

## Quality ceilings and floors

`quality.json` holds the measured limits: ceilings `docs.max_bytes`, `size.zip.library`, `size.zip.runner`,
`size.zip.libretro` and `size.wasm`, floors `tests.ci` and `tests.wasm32`, and a `reasons` list of `{key, reason}`
entries. `tests/cmake/quality.cmake` checks a measured value against its key and fails naming it. Each limit is
enforced in one place:

- the `ci` preset checks `docs.max_bytes` (through `docs.budgets`) and the two test-count floors (`policy.quality`);
- each build leg's `Size ceilings` step checks its three zips after packaging;
- the `wasm32` lane's `Size ceiling` step checks the runner module.

Each size step also runs a control that an oversize value exits 1 naming the key. Numbers are tightened by hand at a
tune-up, to about 15 percent above the largest value measured over the six legs. On Linux x64 the build job compares
the file with the base branch's copy: a ceiling that rises or goes, or a floor that falls or goes, fails unless the
head's `reasons` holds an entry for that key whose text the base does not already hold. A pull request whose base
has no `quality.json` passes. Tightening needs no reason.

## Pinned downloads

Cartridge metadata comes only from the three per-file CC0 MAME hash lists at
`1d7a454ed79e97d35d2a9f127ac029ead94711d6`. Fetch the `mame-sms`, `mame-gamegear` and `mame-sg1000`
rows with `scripts/fetch-pin.sh`, then, from the repository root, run

```
cogmaster-cartdb --pin <commit> --catalog tests/compat/catalog.tsv sms.xml gamegear.xml sg1000.xml \
  --fixture tests/cart/fixture_facts.txt > src/cartdb.inc
```

The run writes `src/cartdb.inc` to stdout and the catalog to the named file. `--pin` is required and is
printed on the table's first line. The fixture input is a fixed owned fact, never the assembler output;
assembly checks its SHA-1 after linking, and no core target depends on fixture assembly. The bounded C17
converter never expands DTDs or external entities.

Every slot in the lists maps through the slot table in `tools/cartdb.c` to a board or to unsupported with a
stated reason; that table is the one place that maps a slot to a board. A slot the table does not name fails
the run, and `--list-slots` prints the names. Stderr prints one `cartdb: slot` line per slot with its seen and
emitted counts. Compound or incomplete records are counted as seen and not emitted. The pinned conversion has
1,670 candidates, 21 excluded records and 1,657 unique rows. Seen and emitted counts per slot:
4pak 1/1, cardcatcher 1/0, castle 1/1, codemasters 22/21, dahjee_typea 11/11, dahjee_typeb 5/5, eeprom 7/7,
hicom 9/9, janggun 1/1, korean 6/6, korean_188in1 1/1, korean_nb 5/5, level3 2/2, mgear 2/1, nemesis 1/1,
othello 2/2, seojin 1/1, terebi 1/1, xterminator 2/2, zemina 8/8.

A row is a binary SHA-1 with fixed-width facts and no title. Titles live in `tests/compat/catalog.tsv`
(five tab-separated fields ending in the title; public records only), which `scripts/compat-update.sh` reads. Identical
hardware facts with card/cart aliases coalesce. Conflicting board, size, persistence or video facts fail
conversion. The table identifies content; membership makes no gameplay claim. Generator tests use authored
XML, not commercial bytes, and the owned fixture row stays out of the catalog. Its public-identity flag is zero.
`cart.fixture_hash` renames the image and tests the production loader; `cart.fixture_wrong_hash` proves a
changed expected identity is refused.

Two gates guard the table. `size.cartdb` in `quality.json` bounds the table's object file, measured on Linux
x64 in the Size ceilings step. The `cart.generator` test also checks that the first line of `src/cartdb.inc`
names the commit `pins.txt` pins, so a pin bump that left the table stale fails.

When adding a repository check or a shared parser/judge helper, add its responsibility to
`tests/policy/repository-checks.txt`. Keep check implementations in CMake script mode or POSIX sh and count
shared helpers in full. Exclusions explain actual build, runtime or release operations individually; moving
a check to another directory does not change its responsibility. The recurring `docs.budgets` check rejects
unclassified implementations and a numerator at or above one tenth of `src` C/H physical lines. Run
`cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` after changing the inventory or helpers.
Keep the existing fault diagnostics and negative controls when consolidating checks. Hosted RetroArch code,
including any helper, stays under `tests/retroarch` so its independent 300-line check sees the whole driver.

`pins.txt` lists every pinned download as `name version url sha256`, and `scripts/fetch-pin.sh` checks the
SHA-256 before use. Take a long-term-support patch release when it fixes a security advisory, and move to the
next long-term-support line before the current one's end of life. Each bump is one `ci:` pull request whose
digest comes from the release API (the asset's `digest` field). Packages installed with apt stay unpinned: they
are signed, and drift fails loudly as a build failure or a hash change. Actions are pinned by commit SHA in the
workflows, with the tag in a comment; `policy.action_pins` rejects any other reference.

Two pins are test data and a tool rather than runtimes, and move on their own terms:

- **`sst-z80`**, the SingleStepTests Z80 vectors at one commit. Move it only when upstream fixes a vector the
  allow list excuses or adds coverage. In the same pull request, regenerate `tests/vectors/z80/sst-sample.bin`
  ([conformance.md](conformance.md#regenerating-the-sample)), update its SHA-256 in `tests/roms/manifest.txt`,
  and remove the allow lines the move settles. The nightly `vectors.sample_equal` fails if the sample is left
  behind. The digest is that of GitHub's archive of the commit.
- **`wla-dx`**, the assembler for the test fixtures. Move it when a release fixes a bug the fixtures meet. The
  fixtures' SHA-256 lines in `tests/built/expected.txt` should not change; if one does, find out why before
  accepting the new digest.
