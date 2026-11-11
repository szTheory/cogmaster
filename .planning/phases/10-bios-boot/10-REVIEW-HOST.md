---
phase: 10-bios-boot
reviewed: 2026-11-11T00:00:00Z
depth: standard
files_reviewed: 22
files_reviewed_list:
  - host/bios.c
  - host/bios.h
  - host/bios_table.inc
  - runner/args.c
  - runner/args.h
  - runner/exit_status.h
  - runner/main.c
  - libretro/libretro.c
  - libretro/options.h
  - libretro/cogmaster_libretro.info.in
  - scripts/firmware-window.sh
  - scripts/hygiene.sh
  - tests/cmake/firmware_scan.cmake
  - tests/cmake/repository.sh
  - tests/host/test_bios.c
  - tests/runner/tests.cmake
  - tests/libretro/bios.h
  - tests/libretro/host.c
  - tests/policy/firmware_release_cases.cmake
  - tests/policy/firmware_window_cases.cmake
  - docs/bios.md
  - docs/runner.md
findings:
  critical: 2
  warning: 7
  info: 3
  total: 12
  necessity: 2
status: issues_found
---

# Phase 10 (host half): Code Review Report

**Reviewed:** 2026-11-11
**Depth:** standard
**Files Reviewed:** 22
**Status:** issues_found

## Summary

The runner, libretro adapter and `host/bios.c` are in good shape. The option grammar, exit mapping and no-echo rules hold up under the cases I traced:

- Repeated options, empty values, a missing value, `--probe` with `--boot` or `--bios`, and `--expect` with `--boot bios` are all exit 2.
- The BIOS read is one bounded `fread` of at most 8,193 bytes. There is no stat, so there is no TOCTOU window.
- The system-directory pointer is used synchronously and copied into a local buffer, so its lifetime is not a problem.
- Directories, `/dev/zero`, ENOENT/ENOTDIR and path-below-a-file map as documented.
- No error path prints the path or a byte. The no-echo test has a working control.
- `.info`, the option tables and the three `LR_OPTION_COUNT` asserts are consistent.
- The exit table text, `--help` and `docs/runner.md` agree.

The defects are concentrated in the local window scan (`scripts/firmware-window.sh`) and its hygiene hook. These are the backstop that is supposed to stop an owner's dump reaching a commit or a release. I reproduced the following in a scratch repository with a self-authored synthetic dump, never a real BIOS:

- Two ways a planted 64-byte slice is not reported while the scan exits 0 and prints `N compared`.
- A dead glob.
- An unreported tool dependency.
- Several detectable-but-undetected forms of text-encoded firmware.

The negative controls pass explicit targets only, so none of them exercises the no-argument `targets()` path where these defects live.

## Critical Issues

### CR-01: An unreadable target silently truncates the scan and it still exits 0

**File:** `scripts/firmware-window.sh:5, 17-28` (`od` at 19 and 24)
**Issue:** `set -eu` is active inside the `{ ... } | awk` group. If `od -An -v -tx1 "$t"` fails for any target (mode 000, a file owned by another user, a root-owned build output), the left-hand subshell exits at once. awk then sees truncated input, prints `firmware window: 1 compared, 0 ignored` and exits 0. Every target after the failing one is never scanned. There is no `pipefail` in POSIX sh to catch it.

Reproduced: an untracked `a_unreadable.txt` (mode 000), sorted before `s.bin`, which held a 64-byte slice of the synthetic dump.
- Output: `od: a_unreadable.txt: Permission denied`, then `firmware window: 1 compared, 0 ignored`, exit 0.
- After `chmod 644` the same scan reports the hit and exits 1.

The same applies to an unreadable dump file at line 19, which kills the group after `N` is printed. A silent pass on partial input is the failure the design names as the classic fault ("a scan that passes because it saw nothing").

**Fix:** Make a read failure loud and fatal for the whole scan. For example:
```sh
od -An -v -tx1 "$t" || { echo "firmware window: cannot read $t" >&2; echo E; }
```
and have awk exit 2 on an `E` line. Or write the stream to an fd-3 status marker and have awk print `incomplete` when the group did not finish (`... ; echo END` as the last line, and awk fails with no `END`). Add a control with a mode-000 target that expects exit 2.

### CR-02: Non-ASCII or special-character file names are skipped silently in the working-tree scan

**File:** `scripts/firmware-window.sh:13, 22`
**Issue:** `git ls-files` is run without `-c core.quotePath=false` or `-z`, so a path with a non-ASCII byte is emitted quoted and escaped, for example `"caf\303\251.bin"`. `[ -f "$t" ]` is then false and the file is silently skipped with `continue`. `scripts/hygiene.sh:7` does override `git` with `core.quotePath=false`, so the author knew about this. The override is not carried into this script.

Reproduced: `café.bin` (the 64-byte slice) in the tree gave `firmware window: 1 compared, 0 ignored`, exit 0. The same bytes under the name `plain.bin` gave a hit and exit 1.

A real BIOS dump copied to a name like `Sega BIOS (japón).bin` slips past `local.firmware_window_scan`.

**Fix:**
```sh
git -c core.quotePath=false ls-files -z --cached --others --exclude-standard | tr '\0' '\n' | grep ...
```
Even better, avoid the `tr` and iterate NUL-separated records with `xargs -0`. At minimum, count and print a skipped-unreadable target (`[ -f "$t" ] || { echo "firmware window: not a file: ..." >&2; ... }`) so a quoted path does not vanish silently.

## Warnings

### WR-01: The libretro module glob never matches, so the shipped core is not scanned directly

**File:** `scripts/firmware-window.sh:14`
**Issue:** The pattern `build/*/cogmaster_libretro.*` assumes the module sits directly under the preset directory. The real layout is `build/<preset>/libretro/cogmaster_libretro.dylib` (and `.so` and `.dll`); I checked `build/ci/libretro/`. Unmatched globs are printed literally and dropped by `[ -f ]`, so nothing fails. The only remaining cover is the extracted `cogmaster-libretro-*.zip`, and that depends on `unzip` (see WR-02).

**Fix:** Use `build/*/libretro/cogmaster_libretro.*`. Also decide whether the other module builds (`_gg`, `_rgb565`, `_replay`) are test-only, which they are, and leave them out. Add a control that plants a slice in a fake `build/x/libretro/cogmaster_libretro.so`.

### WR-02: A missing `unzip` or a failed extraction silently drops every release zip

**File:** `scripts/firmware-window.sh:15`
**Issue:** The chain `[ -f "$z" ] && mkdir -p ... && unzip -qo ... && find ... || true` swallows exit 127 (no `unzip`) and a corrupt archive alike. Git Bash on Windows and many minimal images have no `unzip`. The scan then reports success without ever opening a zip, and the script prints nothing about it.

**Fix:** Check for the tool and fail or warn when zips exist but cannot be opened:
```sh
command -v unzip >/dev/null || { [ -z "$(ls build/*/cogmaster-*.zip 2>/dev/null)" ] || { echo 'firmware window: unzip missing' >&2; exit 2; }; }
```
and let a failing `unzip` fail the scan rather than `|| true`.

### WR-03: A configured but unusable dump directory reports "no dump configured" and exits 0; zip-form dumps count as compared but cannot match

**File:** `scripts/firmware-window.sh:7-9, 18-19`
**Issue:** Three silent-pass shapes, each reproduced:
1. `COGMASTER_BIOS_DIR` set to a nonexistent path, or a `.env.local` value with surrounding quotes (`COGMASTER_BIOS_DIR="/x"`), reports `no dump configured, 0 compared`, exit 0. That text is "the truth of the window" only when nothing is configured. `.env.example` forbids quotes, but a typo or `~` gives the same silent result, and a relative path stops meaning what it did because of the `cd` to the toplevel.
2. An empty directory gives `0 compared, 1 ignored`, exit 0. The registered `local.firmware_window_scan` cannot tell this from a real pass.
3. A dump directory holding a MAME-style `sms.zip` (the most common form for BIOS sets) prints `1 compared, 0 ignored`, but a deflated archive has no usable 32-byte windows, so nothing could ever match. The docs' "not covered" list omits this.

**Fix:** When a value is configured (non-empty) but `[ -d ]` fails, print `firmware window: configured directory is not usable` and exit 2. When no regular file qualifies, exit 2 with `0 compared` (or at least a distinct message). Add `unzip -p` or a refusal for `.zip` dumps, or list the zip case under "not covered" in `docs/testing.md`.

### WR-04: No negative control reaches the no-argument path the real test uses

**File:** `tests/policy/firmware_window_cases.cmake:48-73`; `tests/tests.cmake` (the `local.firmware_window_scan` registration)
**Issue:** Every control passes explicit targets (`${fwin_scan} DIR FILE`) or a hook call. The code that `local.firmware_window_scan` actually runs is `targets()` with no argument: `git ls-files`, the build globs, zip extraction, the `bios|roms|private` exclusion and the dump-directory exclusion. That code has no control at all. CR-01, CR-02, WR-01 and WR-02 would all be caught by one control per target source: a planted slice in a tracked file, an untracked non-ASCII file, a fake `build/x/libretro/` module and a zip.

**Fix:** Add controls in the hygiene lane that build a throwaway repository, plant one slice per source, and run the script with no targets. Anchor each control's expected output to its target name.

### WR-05: Text reduction is defeated by the common hexdump layouts

**File:** `scripts/firmware-window.sh:25-26`
**Issue:** Hex tokens from a text target are fed to the window as one stream, so any non-literal token inside a 32-byte stretch breaks the window. The default `xxd` and `hexdump -C` layouts put an offset column and an ASCII column on every 16-byte line, and every 32-byte window spans a line break. Reproduced: `xxd slice.bin` and `xxd -g1 slice.bin` of the planted 64-byte slice produced no hit, while `xxd -i` did. `docs/testing.md:231` lists base64, decimal lists, XOR and compression as not covered, but not these two forms, which are the usual way someone pastes a dump into an issue or a note. English words that are even-length runs of `a-f` (`be`, `ca`, `fade`, `dead`) in a comment inside an array also break the stream.

**Fix:** Either add a second reduction for lines of the `OFFSET: bytes  ascii` shape (strip a leading `[0-9a-f]+:` and a trailing run after two spaces) before the token split, or add the two forms to the "not covered" sentence in `docs/testing.md`.

### WR-06: `byte_run` is evaded by CRLF line endings and by a trailing comment

**File:** `scripts/hygiene.sh:28-30`
**Issue:** The rule requires every line to be literals only. A trailing `\r` leaves `l` failing `$`, so a 512-byte array in a CRLF file never trips it. Reproduced with the same awk under macOS awk: LF gave `FOUND retyped bytes`; CRLF and `0x..,  // n` per line gave nothing. Pasting from a Windows editor, which is the common case for a retyped dump, is the one input the rule should see.

**Fix:** Strip `\r` in the first statement: `gsub(/\r|\.(db|byte)|[, \t]+/, "", l)`. A trailing `//` or `;` comment can be dropped the same way (`sub(/(\/\/|;).*$/, "", l)` before the gsub). The honest-limit note covers the other forms.

### WR-07: The staged and history hook can overwrite one blob with another

**File:** `scripts/hygiene.sh:63-65`
**Issue:** `git show "$r" > "$d/$(printf %s "$r" | tr '/:' '__')"` maps `/` and `:` to `_`. Two staged paths such as `a/b.rom` and `a_b.rom`, or the same path in a commit `c` and a path with `_` in its name, collapse to one file name. The second `git show` overwrites the first, so the first blob is never scanned. The temp path is also what `firmware-window.sh` prints as the target name, not a repo-relative path.

**Fix:** Name temp files by a counter (`n=$((n+1)); git show "$r" > "$d/$n"`) and keep a name map for the report. This also avoids ENAMETOOLONG on long paths.

## Info

### IN-01: A dead guard in `load_status` and a stale doc pointer in `bios.h`

**File:** `runner/main.c:252, 269-274, 372-373`; `host/bios.h:4`
**Category:** necessity
**Issue:** The `gg_bios` parameter and its branch can never run. `run` passes `bios != NULL && media.machine == COGMASTER_MACHINE_GG`, but `run` receives a non-NULL `bios` only after `bios_load` returned OK. `bios_load` (main.c:634-638) refuses the same condition first, with the same message and exit 77. `media.machine` in `run` comes from the same expressions (`opt->machine`, or `cogmaster__auto_machine` under auto). The second copy of the message is unreachable, and the extra parameter widens every call.

`host/bios.h:4` also says the behaviour "is described in docs/controllers.md". That file never mentions a BIOS; the document is `docs/bios.md`.

**Fix:** Drop the `gg_bios` parameter and its branch (the core's `ERR_PROFILE_REJECTED` text stays for other causes), and change the header comment to `docs/bios.md`.

### IN-02: The libretro module links the digest table and SHA-1 it never calls

**File:** `host/bios.c:14-55`; `libretro/libretro.c:687-723`
**Category:** necessity
**Issue:** `libretro.c` calls only `cogmaster__bios_read`. `lr_bios` carries a comment that "nothing here recognises or prints a digest". `bios.c` is one translation unit, so the 13-row table, `cogmaster__bios_identify` and the SHA-1 dependency ship in every libretro build as dead weight. This is harmless, but it contradicts the stated split of "digests table used by runner/scans only".

**Fix:** Split `cogmaster__bios_read` into its own object (`host/bios_read.c`), or leave it and say so in the CONTEXT decision. It is not a correctness issue.

### IN-03: Small consistency items

**File:** `runner/exit_status.h:34-36`; `host/bios.h:19`
**Issue:**
- The `refused` meaning is split into the literals `"BIOS "` and `"asked for ..."` by a formatter, which is awkward to grep. The text is correct.
- `COGMASTER_BIOS_SIZE 8192u` duplicates the core's accepted size. When the Game Gear 1 KiB image (the seed) is added, the host and core constants must change together.

**Fix:** Join the literal. Add a one-line static or runtime cross-check between `COGMASTER_BIOS_SIZE` and the core's constant when the seed lands.

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
