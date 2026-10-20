# Provenance

How code, data and hardware facts enter cogmaster, which upstream files may seed it, and which notices that
creates. Written on 2026-10-18, before any emulator code exists, so it governs the whole history. People and AI
assistants follow it for every commit. [ASSET_POLICY.md](ASSET_POLICY.md) covers ROM, BIOS and fixture bytes;
`THIRD-PARTY-NOTICES.md` (created with the first vendored file) holds the notices this file requires. `[PV.NN]`
IDs resolve in §10.

| Question | Section |
|---|---|
| May this code, table or fact enter? | 1 |
| Is this upstream file usable at its pin? | 2, 3 |
| What seeds each module, and how is the seed retired? | 4 |
| What must a derived file, the notices file and each release zip contain? | 5 |
| BIOS, boot, hash lists, palettes, measured constants? | 6, 7 |

## 1. Rules
- **Licence.** cogmaster is MIT ([LICENSE](LICENSE)). Inbound code or data must be MIT, BSD-2/3-Clause, ISC, Zlib,
  BSL-1.0, Unlicense or CC0-1.0 *at the exact commit cited*. Read the repository licence **and** the file's own
  header at that pin. Licences change and repositories mix: ares was CC BY-NC-ND before it became ISC [PV.02];
  MAME is GPL-2.0 as a whole with BSD-3 files [PV.01]; devkitSMS mixes public-domain libraries with a GPL-2
  `crt0` [PV.07].
- **Facts** (addresses, timings, measured values, test expectations) enter in this project's own words, with a
  citation giving source, tier and pin. Facts are free to use; prose, code, tables and file structure are not.
- **Permissive expression enters only through §4.** Code, tables, comments, names or structure from another
  project are copied or ported only as a §4 seed: one module, with its notice (§5), a replacement trigger and a
  parity proof. Other permissive sources are read to learn from; the behaviour is then written from
  documentation and shown by a test.
- **Copyleft, non-commercial and unlicensed source stays closed.** GPL/LGPL source, source under a non-commercial
  licence (Genesis Plus GX, PicoDrive) and source with no stated licence is never read past its licence header,
  never pasted into an assistant's context and never kept in the workspace (no clones, no vendored copies, no
  "reference" folders). Its documentation, changelogs, issue trackers and commit metadata may be read. It runs
  only as **released binaries**, used as oracles.
- **Suspect constants.** A constant, table or name that the cited sources do not explain is suspect. It is
  re-derived from a cited source, or removed, before it merges.
- **AI assistance is disclosed** in a co-author trailer or the PR description. An assistant writes from
  documentation, this repository and test results, and is never asked to port, translate or reproduce code
  outside §4. Why: in 2026 an AI-assisted MIT emulator was found to contain GPL code ported from emulators kept in
  its workspace and had to relicense [PV.09].
- **Firmware and game bytes never enter** (§6, ASSET_POLICY.md), whatever licence the file that carries them claims.
- **Pins are public** (AGENTS rule 12): every pin below answered 200 from the GitHub commits API on 2026-10-18.

## 2. Upstream audit (LICENSE and file headers read at the pin, 2026-10-18)
| Source @ pin | Licence at the pin | Usable |
|---|---|---|
| ares `9408cb43d494` [PV.02] | Root `LICENSE`: ISC for ares (© 2004–2025 ares team, Near et al.), followed by the notices of bundled third-party libraries (sljit BSD-2, others); source files carry no per-file header | ISC: `ares/ms/**` (cpu, vdp, psg, opll, cartridge, controller, expansion, system), `ares/component/processor/z80/**`, `ares/component/audio/*`. Third-party directories under `thirdparty/` keep their own licences and are not used |
| MAME `1d7a454ed79e` [PV.01] | Whole: GPL-2.0; each file: its own header; `hash/`: CC0 | BSD-3 (headers read): `src/mame/sega/sms.cpp`, `sms.h`, `sms_m.cpp`, `src/devices/video/315_5124.cpp/.h`, `src/devices/sound/sn76496.cpp/.h`, `src/devices/sound/ymfm/*` (vendored ymfm, BSD-3), `src/devices/bus/sega8/*` and `src/devices/bus/sms_ctrl/*` (check each header before reading past it). CC0: `hash/sms.xml`, `hash/gamegear.xml`, `hash/sg1000.xml` |
| ymfm `81aec25ccbb9` [PV.03] | BSD-3, © 2021 Aaron Giles | `src/ymfm_opl.*` (YM2413 is in the OPL family file), `ymfm.h`, `ymfm_fm.*` |
| SingleStepTests z80 `ebe1875d48f3` [PV.04] | MIT | `v1/*.json` vectors (data) |
| nesturbator `9f9386c661f6` [PV.05] | MIT; every commit by the owner's account or its release bot | All of it, except its third-party files (`libretro.h`, vectors and test ROMs keep their own notices) and NES facts |
| muhtari `952fce970117` [PV.06] | MIT; same owner | All of it, except its third-party files (`libretro.h`, the a7800 palette) and 7800 facts |
| devkitSMS `533ae572c897` [PV.07] | `LICENSES.txt`: SMSlib, PSGlib, SGlib, MBMlib, ihx2sms, assets2banks, folder2c public domain (Unlicense); `crt0/*.s` GPL-2 with a special exception; SMSlib embeds ZX7 and aPLib routines with their own terms | Read for facts about hardware use. Not linked into fixtures: fixtures are pure WLA-DX assembly so no runtime enters the image |
| WLA-DX `91c52b1f4ef3` (tag v10.7; `master` was `7ae11df1657d` on 2026-10-07) [PV.08] | GPL-2.0-or-later (tool) | As a tool only, built in CI from the pinned source tarball; its output belongs to the author of the source it assembles |
| MiSTer SMS core `8443a0d19a88` [PV.10] | No `LICENSE` file; `rtl/T80/*` BSD (Daniel Wallner), `rtl/jt89` GPL-3 (Jotego), `rtl/VM2413` and the rest unmarked; framework `sys/` GPL | **Facts only**: VDP, mapper and I/O behaviour read from docs and commit messages; no RTL is opened past its header except T80, which is not needed (the Z80 is own code against the vectors) |
| SMS Power! wiki and documents [PV.11] | No site licence stated; documents carry their authors' names and sometimes their own terms | Facts in our own words with a citation; a short quote with attribution; prose and tables are not copied |
| Sega official documents [PV.12] | Copyright Sega; archived scans | Facts in our own words with a citation |

## 3. Excluded or unknown (read only up to the licence header; never copied)
"Unknown" counts as excluded until the upstream adds a notice.
- **GPL/LGPL:** Gearsystem (GPL-3), SMS Plus and smsplus-gx (GPL), MEKA, blastem (GPL), Mednafen (GPL), Mesen2
  (GPL-3), Osmose, the GPL cores inside BizHawk, jt89 and the MiSTer framework, the ZEXALL exerciser source
  (GPL-2; built only as a fetched fixture, ASSET_POLICY §3), WLA-DX and SDCC as tools only.
- **Non-commercial:** Genesis Plus GX (`LICENSE.txt`: "may not be sold, nor used in a commercial product") [PV.13];
  PicoDrive. Never opened; run only as released libretro cores, used as oracles.
- **Closed:** Emulicious, Kega Fusion, MasterGear. Their changelogs and documentation are T4 facts; the
  binaries are oracles.
- **Unknown:** MiSTer SMS `rtl/*` files without headers, SMS Power! downloads with no licence statement, VGM
  music rips (the VGM format spec is open; the recorded music is not), Trebor-style palette files with no notice.

## 4. Strangler-fig seeds
A seed is the only route by which another project's expression reaches `src/`. Retiring a seed needs a
**zero-diff differential**: the seed and its replacement run in one test binary over the row's corpus, and the
frame, audio-transition, PCM and state hashes must match on all six platforms. The notice leaves in the same PR.

| Module | Seed @ pin | Licence | Must change | Attribution | Replace when | Parity corpus |
|---|---|---|---|---|---|---|
| Runner, libretro adapter, state visitor, CRC/SHA/PNG helpers, tests scaffolding, CI, hygiene, repo settings, publish | muhtari @`952fce970117` (which carries nesturbator @`9f9386c661f6`) | MIT, same owner | names, the test card, SMS/GG frame sizes and rates, exit-status table | commit-body line (§5a) | n/a (own code) | its own hash pins |
| Z80 CPU | none: own code written from the Zilog manual, "The Undocumented Z80 Documented" and the SingleStepTests vectors; ares `component/processor/z80` is read for structure only | — | — | lifting any table turns it into a seed row | — | SingleStepTests `z80/v1`, every cycle |
| VDP, PSG, mapper, ports, boot | none: own code from the documents in SMS-VDP, SMS-AUDIO, SMS-SYSTEM and SMS-CARTRIDGES-AND-PERIPHERALS, cross-checked against ares (ISC) and MAME's BSD-3 files, which may be read | — | — | — | — | diagnostics; hash-list ledger |
| YM2413 (v1.0 phase 4) | ymfm `src/ymfm_opl.*` @`81aec25ccbb9`, OPLL part only; or ares `ares/ms/opll` @`9408cb43d494` if smaller | BSD-3 / ISC | C++ to C17; integer only; (tick, channel, level) transitions | §5a header; notices section | only if size or speed on low-power targets demands it | ymfm built as a test oracle |
| Hash databases | MAME `hash/sms.xml`, `gamegear.xml`, `sg1000.xml` @`1d7a454ed79e`, converted by our own generator | CC0 | compact binary table of SHA-1 → mapper/region/size facts | notices section (CC0 asks nothing; attribution given anyway) | n/a | the generator's round-trip test |

Read, never copied: ares `ms/vdp`, `ms/psg`, `ms/cartridge`; MAME `315_5124.cpp`, `sn76496.cpp`, `sega8/*`,
`sms_ctrl/*`.

## 5. Notices
**(a) Source files.** Own files carry no header; the repository LICENSE covers them. Code taken from muhtari or
nesturbator gets no header either; the commit body says `Seeded from muhtari <sha> <path> (same author, MIT).`
A file derived from a seed starts with an SPDX line naming both licences, the upstream path and commit, the
copyright holders as the upstream header names them, and what changed; the full licence text lives in
`THIRD-PARTY-NOTICES.md` under a section of the same name.
- **Recommendation:** an SPDX line in derived files only. **Strongest alternative:** an SPDX line in every file
  (REUSE style). **Would change if:** a distributor asks for REUSE compliance.

**(b) `THIRD-PARTY-NOTICES.md`** has one section per upstream, each giving: the paths it applies to in this repo;
the upstream URL, path and 40-hex commit; the SHA-256 of any vendored unmodified file; what changed; and the
licence text verbatim, naming the holders as the upstream header does.

**(c) Release archives.** From the first release, the library, runner and libretro zips each hold `LICENSE` and
`THIRD-PARTY-NOTICES.md` at the root. `release.notices` (CMake `-P`) checks it in `cmake --workflow --preset ci`
and before attestation; its self-test builds a zip without them and must fail, naming the missing file.
`policy.notices` checks every SPDX expression is `MIT` or `MIT AND` one of the §1 licences, that each derived
file has a notices section, and that the libretro `.info` `license` field names the same licences.

## 6. BIOS and boot
- Every Sega BIOS is **user-supplied only**. They are never committed, packaged, typed into C arrays or
  reconstructed. Their identifying digests are in ASSET_POLICY.md §4.
- **Fast boot** (the default, redistributable) sets the post-BIOS state from documented facts (SMS-SYSTEM 6):
  the paging registers, port `$3E` with the cartridge enabled and the BIOS disabled, the VDP registers the BIOS
  leaves, and the Z80 registers and stack. It writes no BIOS code into RAM. The BIOS's own behaviour (header
  check, checksum, slot order) is documented from SMS Power! and MAME's driver notes, never from a disassembly
  of the BIOS kept in the workspace.
- **BIOS boot** (`--boot bios`) runs the user's own dump from `COGMASTER_BIOS_DIR`, recognised by digest.

## 7. Data licences
- **Hash lists.** MAME's three CC0 lists at `1d7a454ed79e` may be converted by our own generator and bundled in
  the core or runner (§4). SHA-256 of each file is recorded in `tests/built/expected.txt` when the generator lands.
- **Palettes** are host helpers outside the determinism hash: the SMS 6-bit CRAM maps to RGB by the documented
  2-bit-per-channel rule and the GG 12-bit CRAM by its 4-bit rule; no third-party palette table is needed.
- **Measured constants** (crystals, line and frame lengths, counter tables, PSG volume curve, LFSR taps) are
  facts. They are cited and restated in our own tables. A table copied verbatim from an MIT, ISC or BSD file is
  expression and becomes a §4 seed.
- **Test vectors.** A SingleStepTests sample is committed with its MIT notice; the full set is fetched nightly at
  the pin.

## 8. Vendored files
Vendored files are never edited. The CTest `libretro.vendored` checks each SHA-256 in this table, and each notice
lives in `THIRD-PARTY-NOTICES.md`. The table is filled in v1.0 phase 1 with `libretro/libretro.h` (MIT, RetroArch
team) at the pin HOSTS-AND-INTEGRATION 1 records.

| File | Upstream | Upstream path | Commit | Licence | SHA-256 |
|---|---|---|---|---|---|
| `libretro/libretro.h` | https://github.com/libretro/RetroArch (tag v1.22.2) | `libretro-common/include/libretro.h` | `69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576` | MIT | `bd3398d29c3763d18617087020ff56b5450a48a63123403a4b674f8e79947acb` |

## 9. What this file does not do
It keeps the project's own licence clean. It is not legal advice, and it is no defence against a rights holder's
claim; the asset policy handles that side by keeping game and firmware bytes out of the repository entirely.

## 10. Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| PV.01 | T1 | github.com/mamedev/mame: `COPYING`, file headers of `src/mame/sega/sms.cpp` (BSD-3, holders Pol, MacDonald, Rosenhauer, Oliver, Luong, Priuli, Land), `src/devices/video/315_5124.cpp` (BSD-3, Pol, Land), `hash/sms.xml` L4 (`license:CC0-1.0`) | `1d7a454ed79e` | whole GPL, per-file BSD-3, `hash/` CC0 |
| PV.02 | T1 | github.com/ares-emulator/ares `LICENSE` (ISC text read here (tested)); GitHub API reports NOASSERTION because the file also carries bundled libraries' notices; `ares/ms/`, `ares/component/processor/z80/` directory listings | `9408cb43d494` | ISC scope |
| PV.03 | T1 | github.com/aaronsgiles/ymfm `LICENSE`, `src/` listing | `81aec25ccbb9` | BSD-3; OPLL lives in `ymfm_opl.*` |
| PV.04 | T1 | github.com/SingleStepTests/z80 `LICENSE`, `README.md` | `ebe1875d48f3` | MIT; vector format |
| PV.05 | T1 | github.com/szTheory/nesturbator `PROVENANCE.md`, `THIRD-PARTY-NOTICES.md`, git authors | `9f9386c661f6` | same-owner seeds |
| PV.06 | T1 | github.com/szTheory/muhtari `PROVENANCE.md`, `AGENTS.md`, git authors | `952fce970117` | template; same-owner seeds |
| PV.07 | T1 | github.com/sverx/devkitSMS `LICENSES.txt` | `533ae572c897` | per-component licences |
| PV.08 | T1 | github.com/vhelin/wla-dx `LICENSE` (GPL-2.0-or-later), release `v10.7` (tag object `6632a12acf46` → commit `91c52b1f4ef3`; tarball SHA-256 in SMSPOWER-EXTRACTS 7) | `91c52b1f4ef3` (v10.7); `master` `7ae11df1657d` | tool licence and pin |
| PV.09 | T1 | nesturbator `.planning/preparation/DECISIONS.md` DEC.04 (relicensing precedent) | `8ae2c6e32e7f` | AI-assistance rule |
| PV.10 | T1 | github.com/MiSTer-devel/SMS_MiSTer: no `LICENSE`; `rtl/` listing (T80, jt89, VM2413); github.com/jotego/jt89 licence GPL-3 | `8443a0d19a88` | facts-only status |
| PV.11 | T2 | smspower.org `Development/Documents`, `Development/Index` | accessed 2026-10-18 | document inventory; no site licence |
| PV.12 | T1 | Sega "SMS/GG Official Documentation" as hosted on smspower.org | accessed 2026-10-18 | official facts |
| PV.13 | T1 | github.com/ekeeke/Genesis-Plus-GX `LICENSE.txt` | accessed 2026-10-18 | non-commercial terms |
| PV.14 | T1 | GitHub API `license` field of drhelius/Gearsystem (GPL-3.0) | accessed 2026-10-18 | GPL status |
