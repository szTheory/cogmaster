---
title: "Master System and Game Gear emulators, users and personas"
summary: "Every SMS/GG emulator's licence, accuracy reputation, tooling and status; what players, developers, speedrunners and TASers complain about, mapped to cogmaster responses by horizon; persona needs; low-power and WASM budgets; how the ecosystem measures compatibility; the slot cogmaster fills."
read_when: "Before ranking features or horizons, choosing an accuracy oracle, writing persona or frontend requirements, planning a port, or answering 'why not emulator X'."
updated: 2026-10-18
---

# Master System and Game Gear emulators, users and personas

| Question | Section |
|---|---|
| Which SMS/GG emulators exist: licence, accuracy reputation, debugger, libretro core, status? | 1.1 |
| Which may be opened, and which is the accuracy reference today? | 1.2, 1.3 |
| What do players and developers dislike or wish for, and what does cogmaster do about each? | 2 |
| What does each persona need, by horizon? | 3.1 |
| Speedrun and TAS rules, movie formats, lag counter, AI environment, frontends | 3.2–3.5 |
| Can an exact-timing core hold full speed on DS, PSP, 3DS, ARM handhelds, Pi, WASM? | 4 |
| Library sizes, RetroAchievements IDs and hashing, how others report compatibility | 5 |
| What combination does nobody offer today? | 6 |

Licence rule applied: GPL, non-commercial and unlicensed emulator source was not opened (READMEs, LICENSE files,
changelogs, trackers, docs only); first-line licence headers were read only where the brief asks for a per-file check.
GitHub metadata came from the REST API, wiki text from the MediaWiki API (HTML is bot-blocked). SMS Power! answered 503
all day (2026-10-18); its counts are an open question (section 7).

## 1. Emulators
### 1.1 Landscape
Accuracy = Emulation General Wiki rating at rev 151020 unless noted [ECO.01]. D debugger, S states, R rewind, M movies,
L Lua, RA RetroAchievements, LR libretro core. "NOASSERTION" = GitHub detects no licence; the LICENSE file is named.

| Emulator | Language · licence (checked) | Accuracy · compat claim | D · LR · features | Platforms | Last release · last commit | Strengths · weaknesses |
|---|---|---|---|---|---|---|
| Emulicious | Java · closed freeware, personal use only [ECO.02] | Very high | D (stepping back, watchpoints, profiler, coverage, source-level, VS Code remote) · no LR · S | Win, Linux, macOS, Pi OS | 2026-03-19 (paddle, Sports Pad, Mega Mouse added) [ECO.02] | best homebrew tooling [ECO.03]; closed, Java, no libretro, no RA |
| ares | C++ · ISC (LICENSE: "any purpose with or without fee"; GitHub NOASSERTION) [ECO.04] | High; own list SMS 400/410, GG 462/468, SG-1000 178/179 completable [ECO.05] | tracer only · no LR · S R | Win x64/ARM, macOS, Linux, BSD | v148 2026-05-30 · 9408cb43d494 2026-10-06 | whole family (cpu, vdp, psg, opll, expansion) openable; "trade speed for code clarity" |
| Genesis Plus GX (GPGX) | C · custom non-commercial ("may not be sold, nor … commercial product") [ECO.06] | Very high; "100% compatibility … including unlicensed or pirate known dumps" [ECO.06][ECO.07] | no D · LR · S R RA, FM (MAME or Nuked), gun, paddle, Sports Pad, Graphic Board | libretro everywhere, Wii/GC, BizHawk, OpenEmu, Provenance | no tags; 49c584764893 2026-10-06 | most embedded SMS/GG core; NC licence, no debugger, several open timing issues (section 2) |
| Gearsystem | C++ · GPL-3.0 [ECO.08] | High; "very accurate Z80, VDP, PSG and FM" [ECO.08] | D (JIT disassembler, breakpoints, symbols, VRAM viewers, MCP server) · LR · S RA, GG link cable, 3-D glasses, gun, paddle | Win, macOS, Linux, BSD, ~20 libretro targets | 3.9.22 2026-10-11 · 190873a34c3f 2026-10-16 | fast-moving, friendly tracker; GPL-3, "instruction accurate (not cycle accurate)" [ECO.09] |
| MEKA | C++ · no LICENSE; meka.txt: "BSD-like for most parts", rest "Unlicensed. Gray area." [ECO.10] | "?"; own compat.txt 659/666 SMS, 516/527 GG, 195/195 SG-1000 Ok [ECO.11] | D (debugger.txt) · no LR · 3-D glasses, SK-1100, SF-7000, Graphic Board, Othello Multivision | Win; Linux/macOS "seeking maintainers" | meka-20250817 · 4c00b68ed08f 2026-10-13 | broadest device coverage; author: "codebase is old, ugly and messy", "doesn't has much use for the average player" |
| Kega Fusion | closed | Mid; "very high" compat by reputation [ECO.01] | no D · no LR | Win, 32-bit macOS/Linux | 3.64 2010-03-07; dead | still named in speedrun rulesets [ECO.12]; unmaintained, 32-bit |
| BizHawk SMSHawk | C# · repo MIT for BizHawk-team C#; LICENSE calls the whole repo "a minefield"; SMS.cs has no header [ECO.13] | Very high; "acceptable on TASVideos" | D (IDebuggable, CDL, Lua, RAM search) · no LR · S R M(BK2) RA? (not listed for SMS) | Win, Linux | 2.11.1 2026-05-01 · c9de9a8bff0f | the TAS platform; TODO: "HCounter manually set for light phaser", TMS mode 1 missing |
| BizHawk gpgx core | C · GPGX NC | as GPGX | SMS/GG/SG-1000 wired in 2.9.2 [ECO.14] | Win, Linux | 2.11.1 | second opinion inside BizHawk |
| Mednafen `sms` | C++ · GPL-2 | "?"; "based off of SMS Plus", "low-priority system" [ECO.15] | D (generic) · no Beetle SMS LR | Win, source | 1.32.1 2024-04-05 | – |
| MAME `sms` | C++ · GPL-2 whole; `sms.cpp`, `315_5124.cpp`, `sn76496.cpp` BSD-3 headers [ECO.16] | "?"; mappers "Unlicensed" level; hardware variants | D (full) · LR (mame) · SC-3000 cassette, 3-D glasses, keyboard | all | 0.289 2026-07-30 | openable VDP (BSD-3); heavy frontend |
| SMS Plus / smsplus-gx | C · README "GPLv2 or later"; no LICENSE file [ECO.17] | Mid (console row) | no D · LR · ColecoVision | libretro, RS97-class handhelds | no tags · 3844b46caa92 2026-09-04 | muOS SMS default, EmulatorJS default [ECO.18][ECO.19]; old lineage |
| BlastEm 1.0.0 | C · GPL-3 | "?" / TBD | D (GDB-like, scripting) · LR (Genesis only for SMS "standalone version for now") · Web build | Win, Linux, macOS, WASM | 1.0.0 2026-09-11 [ECO.20] | GG, SG-1000/SC-3000 new in 1.0.0; SMS secondary |
| Mesen2 → MesenCE | C++/C# · GPL-3 | "?" | D (full suite) · no LR | Win, Linux, macOS | Mesen2 archived at 2.1.1 (2025-07); MesenCE 2.2.1 2026-06-05 [ECO.21] | yes, "SMS/Game Gear" supported |
| Osmose | C++/Qt · GPL-3 | "?" | no D · no LR | Linux, RetroPie fork | 1.6 2020-08-02; dead [ECO.01] | "tedious to configure controllers" [ECO.22] |
| Dega | C · NC ("as long as you don't use it commercially") | not rated | no D · M(MMV) | Win, SDL | 1.16 ~2010; dead | first TAS SMS emulator; "enhanced PSG" FM-for-all mode [ECO.23] |
| MasterGear | C · closed (Windows free, Android paid) | High | D (undocumented) · no LR · NetPlay | Win, Linux, Android | 4.9 2021-03-31 [ECO.24] | – |
| PicoDrive (irixxxx) | C · NC (same clause as GPGX) | Mid; "designed to run on weak devices" [ECO.25] | no D · LR · S | ARM handhelds, PSP, libretro | v2.05 2025-05-31 · 2026-10-16 | fastest; RetroPie armv6 default; forum: "very poor core for Master System" [ECO.26] |
| MiSTer SMS_MiSTer | VHDL/SV · no LICENSE; T80 BSD-style, VM2413 NC clause, jt89 GPL-3 headers [ECO.27] | RTL; 28 open issues (EEPROM GG games, 4 Pak, Nemesis 2, Operation Wolf) | savestates, FM, System E, Evolution 132-game flash, GG link over USERIO | DE10-Nano | rbf in repo · 8443a0d19a88 2026-10-14 | timing reference candidate; mixed licences, facts only |
| Analogue Pocket openFPGA-SMS / -GG (Spiritualized1997) | binary only · no LICENSE | – | sleep/wake, memories | Pocket | 1.2.0 / 1.3.0 2022-08-27; dormant [ECO.28] | – |
| Delta · Provenance · OpenEmu (Apple) | Delta AGPL-3: Genesis only, no SMS/GG; Provenance 3.3.0 and OpenEmu 2.4.1: SMS, GG, SG-1000 via GPGX [ECO.29] | as GPGX | – | iOS, macOS | 2026-03-14 · 2023-12-30 | every Apple frontend is GPGX underneath |
| Sega official | AGES (M2, Switch): Phantasy Star, Alex Kidd; Genesis Classics and Mini 2: no SMS/GG; Sonic Origins Plus: 12 GG games, "zoomed-in" display criticised; Nintendo Switch Online: none [ECO.30] | – | – | – | – | no official GG reference exists |
| Browser | jsSMS (GPL-3, recompiler, last 2023); EmulatorJS SMS=smsplus, GG=gpgx; BlastEm web; jgenesis web; anisse/gears (Rust, MIT) [ECO.31] | – | – | WASM/JS | – | no permissive, accurate WASM SMS core |
| Permissive newcomers | Snepulator C++ MIT v26.06, wiki-recommended, SST tests [ECO.32]; CLK (MIT, SMS no GG) 2026-07-23; TotalSMS (C, MIT, 2025-07); Cega (C11, MPL-2.0 + BSD-3, cycle-stepped Z80/VDP/PSG, "100% compatibility", 0.6.3 2026-09-11) [ECO.33]; jgenesis (Rust, **GPL-3**, wiki wrongly says MIT; ZEXALL runner) [ECO.34]; Nuked-SMS (GPL-2, WIP 2024); Sandopolis (Zig, MIT) | unrated | – | – | – | Cega is the closest existing match to cogmaster's slot (section 6) |

### 1.2 What may be opened
ares `ares/ms/*` and its Z80 (ISC); MAME `sms.cpp`, `315_5124.cpp`, `sn76496.cpp` (BSD-3 headers), `hash/*.xml`
(CC0-1.0 header, checked at 1d7a454ed79e) [ECO.16][ECO.35]; SingleStepTests/z80 (MIT); T80 (BSD-style) inside MiSTer;
Snepulator, CLK, TotalSMS, gears (MIT). Never: GPGX, PicoDrive (NC), Gearsystem, smsplus-gx, BlastEm, jgenesis, MesenCE
(GPL), MEKA (gray), BizHawk SMS (no header; treated as unopened), Cega (MPL-2.0 is file-copyleft, not on the allowed list).

### 1.3 Accuracy reference today
No software emulator publishes hardware-verified timing; "100%" claims are self-assessed (GPGX, Cega) or per-game text
lists (MEKA). GPGX has open scanline/run-ahead issues (Micro Machines sprites, Cosmic Spacehead frame shift, Shadow of
the Beast collision) [ECO.36]; Gearsystem says it is instruction-level [ECO.09]; ares is cycle-oriented but slow and has
open mapper and run-ahead issues [ECO.37]. **Recommendation:** hardware arbitrates (the owner's consoles, a flash cart,
cogmaster's diagnostics); MiSTer RTL is the timing reference; released binaries of ares v148, GPGX (libretro) and
Emulicious are the differential oracles; TASVideos' 74 SMS + 25 GG + 6 SG-1000 movies are whole-game sync checks [ECO.38].
**Strongest alternative:** Emulicious alone (highest-rated, best tooling). **Would change if:** a capture contradicts MiSTer
where Emulicious agrees, or Cega publishes logic-analyser traces (then add it as an oracle).

## 2. Complaints and wishes → cogmaster response
Who: P player, D developer, H ROM hacker, T TASer/speedrunner, F frontend or porter. Horizons are the proposed ones of
section 3.1. **won't** = not in this repository, with the reason.

| # | Complaint or wish (who) | Evidence | cogmaster response |
|---|---|---|---|
| C.01 | GG picture: "stretched widescreen", 1.524 PAR disputed, windowed ratio wrong, Miyoo vertical scaling, 16:10 requests (P) | [ECO.39] | **v1.0** report true 160×144 GG and 256×192/224/240 SMS sizes with exact PAR (NTSC 8:7-class value derived from the pixel clock); scaling in the host; **won't** stretch options |
| C.02 | LCD ghosting: GPGX filter applies to every system (open since 2017, "design choice"), resets on state load; MAME GG blur on by default (P) | [ECO.39] | **v1.0** no filter in the hashed frame; **seed v4.0** optional GG LCD response model as a host-side helper, off by default |
| C.03 | GG-in-SMS-mode borders: 256×192 shown for SMS-engine GG carts, "has borders while Borders enabled", "option to disable border masking" (P) | [ECO.39] | **v1.0** GG profile emits the 160×144 window; a `gg_extended` option exposes the full field for SMS-mode carts, hashed separately |
| C.04 | FM: "always on" refused for cost; FM-for-all rejected; Nuked state-load garble open since 2022; state-load crash (2026); Gearsystem FM detection bugs; ares FM only with Japanese region (P) | [ECO.40] | **v2.0** YM2413 as a device present per machine profile (Mark III + unit, SMS-J), detectable through port $F2 as hardware; FM state inside the save state from day one; **v1.0** PSG only |
| C.05 | Region/BIOS: Korean and Japanese games stop at the export BIOS; GG state-of-region set wrong; copier headers; bootrom option broken in 1.7.4; bootrom + run-ahead freezes; "need to build a game database" (P) | [ECO.41] | **v1.0** fast boot needs no BIOS; optional BIOS per region checked by md5 under GPGX's names (`bios_E/U/J.sms`, `bios.gg`); region from the ROM header where present, hash list (CC0) for the rest, user override; **v2.0** Korean/Brazilian profiles |
| C.06 | Light Phaser: libretro mouse-as-gun open since 2015; Wii U trigger; touchscreen fires centre first; crosshair misaligned because "instruction accurate"; Laser Ghost needs pad in port 2 (P, T) | [ECO.42] | **v2.0** gun sensed from the exact beam position (H/V counters from the pixel clock), libretro lightgun + pointer, offset option; port assignment as a core option |
| C.07 | Paddle, Sports Pad, Graphic Board, SK-1100, GG link, Mark III link (F-16 J) requests (P, D) | [ECO.42][ECO.02] | **v2.0** paddle, Sports Pad; **seed v2.0** GG link (two instances in one process), SK-1100, Graphic Board; **seed v4.0** Mark III link |
| C.08 | 3-D glasses: anaglyph refused by GPGX ("6 years old and still not implemented"), Zaxxon shaking "not a bug", only MEKA/Dega/Gearsystem offer eye modes (P) | [ECO.43] | **v2.0** port $F7/$FFF8 shutter state exposed per frame; left/right/both/anaglyph composed by the host or the runner from the hashed frames |
| C.09 | Mappers: Codemasters, Korean 188-in-1, Janggun, 4 Pak loops back to menu, 93C46 EEPROM GG games (MiSTer: none boot), manual mapper override wanted (P) | [ECO.44][ECO.27] | **v1.0** none, Sega and Codemasters (97.0 % of the SMS+GG hash lists, CARTRIDGES 2); **v2.0** Korean family, 4 Pak, 93C46 and every other mapper in MAME's hash lists (SEED-001); override option; hash-list hints never behaviour hacks |
| C.10 | Sprite flicker: "no sprite limiter option?" refused then added; "flashing sprites are quite annoying" (P) | [ECO.45] | **v1.0** exact 8-per-line limit and overflow flag; **v2.0** `no_sprite_limit` option, outside the default hash, flagged in replay identity |
| C.11 | PAL: "forced at 60fps", Lucky Dime Caper and Dizzy too fast; no header so DB-listed PAL-only games; MEKA PAL list wrong for Brazil (P, T) | [ECO.46] | **v1.0** 50 Hz profile with the exact 313-line rational rate; PAL-only hash list; TASVideos' Brazil-NTSC rule documented (3.3) |
| C.12 | Run-ahead breaks collision flags (Shadow of the Beast, Fantasy Zone GG), pause NMI lost under run-ahead (ares), rewind corrupts (P, T) | [ECO.47] | **v1.0** all VDP status and NMI edge state in the instance; **v3.0** run-ahead and rewind tests from power-on |
| C.13 | Netplay: "savestates aren't fully deterministic" (GPGX libretro, open); Gearsystem "something difficult" (P) | [ECO.47] | **v1.0** cross-platform frame/audio hashes; **v3.0** fixed-size, fully deterministic states tested for netplay |
| C.14 | States: crash loading older states, "should just reject them"; Android vs PC incompatible; SRAM altered by states; FM state garbled (P, T) | [ECO.48] | **v3.0** versioned, endian-fixed state with a header; refusals return a status; battery RAM separate from states |
| C.15 | Cheats: PAR codes in a different format, "cheats are region specific", GPGX has no native cheats, Gearsystem wants names and toggles (P, H) | [ECO.49] | **v2.0** Game Genie and PAR decoders in the runner; cheats are recorded RAM/ROM pokes in replay identity |
| C.16 | RA only on GPGX, Gearsystem, RAMeka, Pizza Boy; MEKA "lots of technical debt"; users want 60 Hz where sets were made (P) | [ECO.50] | **v1.0** rcheevos memory map (5.2) and whole-file MD5; PAL games run at 50 Hz with the fact documented |
| C.17 | Borders: "blue bar on the left", left-column crop then California Games broke; 284 (13+15) vs centred; crop arbitrary lines; MEKA wants overscan shown and mid-screen mode changes (P) | [ECO.51] | **v1.0** emit the full 342×262 field or active area, each hashed; left-column mask colour as hardware (backdrop); host crops |
| C.18 | Line-interrupt games: Micro Machines misaligned sprites and Cosmic Spacehead frame shift (open 2025), Desert Speedtrap needed latched name-table and scroll, Back to the Future II attract glitches (fixed 2026), BTTF/Dizzy family (P, D) | [ECO.36][ECO.52] | **v1.0** register latching at the documented pixel positions, line counter reload rules, mid-line writes sampled per pixel; those titles become ledger rows |
| C.19 | Developers want hardware strictness and tooling: Emulicious' backward stepping, uninitialised-memory watch, profiler, VS Code; Gearsystem symbols and MCP (D, H) | [ECO.02][ECO.08] | **v1.0** side-effect-free peek and per-line stats API; **v5.0** symbols (WLA-DX, sjasm), bank-aware breaks, watch on uninitialised RAM, trace, profile |
| C.20 | Abandonware and closed source: Kega dead since 2010, Osmose dead, MasterGear closed, Emulicious closed, Pocket cores dormant (P, D) | [ECO.01][ECO.28] | **v1.0** permissive library, attested releases every phase |
| C.21 | Packaging: MEKA Defender/VirusTotal flags, no Mac/Linux; Gearsystem glibc, GLEW, macOS codesign failures, stale buildbot; "not the place for Retroarch support" (P, F) | [ECO.53] | **v1.0** signed macOS dylib, six-platform CI, issue forms asking frontend and version |
| C.22 | Speed on weak devices: PicoDrive R-Type "very slow" on GP2X; Gearsystem ~15 fps on old 3DS (unverified); GPGX "too slow for PSP" (P, F) | [ECO.54][ECO.55] | **v1.0** Ir/frame gate; **v6.0** device budgets (section 4) |
| G.01 | Input latency (P, T) | [ECO.56] | **v1.0** frame boundary at VBLANK start, pad latched there; **v3.0** run-ahead measured |
| G.02 | Stutter and audio pacing (crackling at 120/240 Hz) (P) | [ECO.53] | **v1.0** rational fps and sample rate reported, frame-time percentiles |
| G.03 | Battery and portability on handhelds; distros pick PicoDrive for speed (F) | [ECO.18][ECO.26] | **v6.0** measured budgets; integer-only, no allocation after load |
| G.04 | UI wishes: all-in-one, hotkeys, drag-and-drop (P) | [ECO.01] | **won't**: no GUI here; RetroArch or Playstead host |
| G.05 | Enhancements that change games (overclock "will break most games", widescreen impossible) (F) | [ECO.45][ECO.01] | **won't** by default; any enhancement explicit and in replay identity |
| G.06 | Unclear compatibility; hardware quirks reported as bugs (Golvellius hole "occurs on real hardware") (P) | [ECO.37] | **v1.0** generated `docs/compatibility.md` with a not-a-bug list |

Praise to keep: Emulicious "excellent tool for anyone writing homebrew"; Gearsystem "fantastic emulator"; GPGX "best core
for Master System emulation" [ECO.03][ECO.53]. **Recommendation (C.05/C.09):** header and hash list for identity only,
no per-game behaviour hacks. **Strongest alternative:** a GPGX-style internal database that also sets behaviour.
**Would change if:** a hardware-verified title needs state that no header, hash list or mapper model can express.

## 3. Personas and jobs to be done
### 3.1 Needs by horizon
Proposed horizons (derived; cogmaster has no PRODUCT-ROADMAP yet, pattern follows the sibling packs [ECO.57]): v1.0 SMS +
GG games with PSG sound in RetroArch · v2.0 whole library (mappers, FM, peripherals, regions) · v3.0 states and movies ·
v4.0 measured accuracy · v5.0 tools and AI · v6.0 everywhere (handhelds, WASM, Playstead).

| Persona | Job | Table stakes [horizon] | Differentiators [horizon] |
|---|---|---|---|
| Player, Mac + RetroArch | Play SMS and GG dumps as the console did; keep saves | Core loads on Apple Silicon [v1.0]; `.sms .gg .sg` [v1.0]; no BIOS needed [v1.0]; PAL [v1.0]; FM, gun, paddle, 3-D [v2.0]; battery saves [v2.0]; states, rewind, run-ahead, netplay [v3.0]; RA map [v1.0] | true sizes and PAR [v1.0]; ledger with not-a-bug notes [v1.0]; latency measured [v3.0] |
| Homebrew developer (SMS Power! competitions; WLA-DX, devkitSMS, z88dk) | Know it runs on hardware before the deadline; iterate in seconds | Accuracy at least Emulicious' on timing-sensitive tests [v1.0]; headless runner with frame/audio/RAM hashes for CI [v1.0]; deterministic runs [v1.0]; all mappers [v2.0] | symbol files (WLA-DX `.sym`, NoICE) [v5.0]; uninitialised-RAM watch, backward trace, per-line VDP stats, sprite-overflow and VRAM-timing lint [v5.0]; VS Code DAP [seed v5.0] |
| ROM hacker | Find code and data across banks; patch; prove the patch changed only what was meant | soft-patching in RetroArch (core loads from memory) [v1.0]; peek, `--probe` with MD5/SHA [v1.0] | bank-aware trace, write-to-ROM detection, tile/name-table viewer [v5.0]; hash diff of original against patched runs [v1.0] |
| Speedrunner | Practise and submit on a fair set-up | exact 59.9227510 / 49.7014601 fps [v1.0]; no frame skip [v1.0]; practice states [v3.0]; input display via runner [v5.0] | movie replay verifies a run [v3.0]; autosplit RAM hook [v5.0] |
| TASer | A movie that syncs everywhere | determinism, states, rerecord count, lag flag [v3.0] | BK2 and MMV import [seed v3.0]; Lua [v5.0]; sub-frame input not needed (3.3) |
| AI/ML researcher | Train agents on SMS/GG games reproducibly | Gymnasium env, pixels + 8 KiB RAM, deterministic reset [v5.0] | vectorised in-process instances, video-off mode, reward files [v5.0] |
| Frontend integrator (libretro, Playstead, custom) | Embed one small, stable, permissive core | `.info` with GPGX's BIOS names [v1.0]; runner exit codes [v1.0]; fixed serialize size [v3.0] | MIT C API, no globals, no FP [v1.0]; Playstead continuation [v3.0]; OpenEmu/Provenance plugin [seed v6.0] |
| Low-power porter | Hold 60 fps on an RG35XX/Miyoo/3DS/PSP | C17, integer-only, no threads [v1.0]; big-endian nightly [v1.0] | per-device Ir budgets in CI, RGB565 helper [v6.0] |
| Browser user | Play in a tab with no install | wasm32 hash lane [v1.0] | WASM build with Emscripten audio worklet [v6.0] |

**Recommendation:** players and developers first (v1.0–v2.0), movies (v3.0), measured accuracy (v4.0), tools and AI
(v5.0), ports (v6.0). **Strongest alternative:** pull symbols and the debugger API into v2.0 because SMS Power!
competition entrants are the most likely early adopters. **Would change if:** developers adopt the runner early.

### 3.2 Speedrunners
- speedrun.com platforms: Master System `83exwk6l` (284 games), Game Gear `w89r3w9l` (186), SG-1000 `83expv9l`; every
  ruleset fetched allows emulators; Alex Kidd lists "Kega Fusion 3.64, BizHawk, Genesis Plus GX, Other"; "If you're
  playing a UE ROM at 60Hz, submit as USA / NTSC"; no RetroArch ban found [ECO.12].
- TASVideos frame rates: NTSC 59.9227510135505, PAL 49.70146011994839 [ECO.38]. An emulator locked to 60.000 Hz runs
  0.129 % fast, 2.3 s per 30 min; one at 50.000 Hz 0.60 % fast, 10.8 s (derived).
- **Recommendation:** exact rational rates reported to hosts, nothing speed-changing on by default, emulated time printed
  by the runner [v1.0]. **Strongest alternative:** a 60 Hz "compat" rate. **Would change if:** never.

### 3.3 TASers
- TASVideos: 74 SMS, 25 GG, 6 SG-1000 publications; accepted emulators BizHawk (BK2) and Dega "(altered versions)" (MMV);
  Mednafen not listed [ECO.38]. Rules: BIOS unmodified, region matches the game, no 50/60 Hz swaps except "Sega Master
  System games with an official release in Brazil can use NTSC mode" [ECO.38].
- SMSHawk: lag frame = no controller-port read in the frame; Pause is NMI "not effected by lag"; Reset is a controller
  input; BIOS loaded only when the game needs it or the sync setting asks [ECO.13][ECO.14]. BK2 LogKey buttons: Reset,
  Pause (SMS) / P1 Start (GG), P1 Up Down Left Right B1 B2, P2 same, gun X Y [ECO.13]. MMV: "MMV\0", 2-byte packets,
  bit 0x40 Start(SMS) one frame, 0x80 Start(GG), flags PAL/Japan/GG, ROM MD5 [ECO.38].
- **Recommendation [v3.0]:** own text movie carrying BK2's information (MD5 and SHA-256, BIOS or fast boot, region,
  port devices, machine profile, behaviour revision), one line per frame; lag = no read of ports $DC/$DD (or GG $00) in
  the frame, a superset of SMSHawk's; BK2 and MMV importers so published movies become whole-game sync tests [seed
  v3.0]. Sub-frame input is not needed: pads are read by polling and Pause is a one-frame NMI edge. **Strongest
  alternative:** a BizHawk Waterbox port for acceptance. **Would change if:** TASVideos accepts a standalone runner.

### 3.4 AI agents
No SMS/GG RL environment exists (ALE is 2600; stable-retro has Genesis via GPGX but its SMS use is unverified) [ECO.58].
**Recommendation [v5.0]:** Python over the C API: Gymnasium `Env` + vector env, observations palette indices or RGB
(256×192 or 160×144) and `ram` = 8 KiB system (+ cart RAM), 2-button pad = 18 actions (9 directions × 4 button
states → 36 with Start on GG), sticky actions and frame-skip in the env layer, reward files per game; ≥ 5,000
frames/s per thread given ≤ 4 M Ir/frame (section 4). **Strongest alternative:** add the libretro core to stable-retro.

### 3.5 Frontends
- libretro: GPGX options are the de-facto schema (system_hw, region_detect, bios, overscan, left_border, gg_extra,
  lcd_filter, aspect_ratio, ym2413, ym2413_core, gun_input, gun_cursor, no_sprite_limit); BIOS md5 bios_E/U.sms
  840481177270d5642a14ca71ee72844c, bios_J.sms 24a519c53f67b00640d0048ef7089105, bios.gg 672e104c3be3a238301aceffc3b23fd6
  [ECO.07]. Gearsystem adds mapper, refresh rate, glasses, link cable [ECO.09]. Default cores: RetroPie GPGX (PicoDrive
  on armv6), Batocera GPGX first, Recalbox PicoDrive for SMS, muOS smsplus (SMS) and PicoDrive (GG), Onion PicoDrive
  first, EmulatorJS smsplus/GPGX [ECO.18][ECO.19][ECO.22]. Cheat files: 724 SMS, 818 GG [ECO.59].
  cogmaster: same option keys and BIOS names [v1.0]; gun, paddle, glasses as libretro devices and options [v2.0].
- Playstead: adapter pin plus continuation gate as the sibling packs describe [v3.0 runner, v6.0 adapter] [ECO.57].
- **Recommendation:** libretro first; OpenEmu/Provenance plugin as seeds. **Would change if:** a maintainer asks.

## 4. Low-power device budgets
**Work per NTSC frame** (derived from [ECO.60]): 228 × 262 = 59,736 Z80 cycles; 342 × 262 = 89,604 pixel clocks;
49,152 active pixels (GG shows 23,040 but renders all); 3,734 PSG clocks (÷16); ~830 YM2413 samples (÷72); 800 samples at
48 kHz. Full speed is 3,579,545 cycles/s, so a device's **budget** = MHz ÷ 3.58 host cycles per Z80 cycle. **Cost model**
(unverified until the v1 Cachegrind gate): Z80 20–40 Ir/cycle, pixel 4–8 Ir, PSG ≈ 0.1 M, FM 0.25–0.5 M → burst/lazy
design **1.8–3.8 M Ir/frame = 30–64 Ir per Z80 cycle**; stepping every pixel clock adds 0.5–0.9 M → **38–79**. At an
assumed in-order IPC of 0.5–0.8, that is 38–128 host cycles per Z80 cycle (burst) or 48–158 (per-tick). Anchors: S8DS
(ARM assembly) runs up to 4× speed on a 67 MHz DS; SMS Plus PSP needed FM off for "100% speed" at 222 MHz; PicoDrive
"2D games run full speed" on old 3DS; pico-smsplus full speed on an RP2350 at 252 MHz, FM needs 378 MHz; MCUME SMS/GG
"full speed with sound" on a 600 MHz Cortex-M7; WASM ≈ 45–55 % slower than native [ECO.54][ECO.55][ECO.61].

| Device | CPU · MHz | SMS/GG emulation today | Budget | Burst · per-tick |
|---|---|---|---|---|
| GBA | ARM7TDMI 16.8 | SMSAdvance (asm), no FM, fake collision | 5 | no · no |
| DS / DS Lite | ARM946E-S 67 | S8DS full speed, FM "missing good" | 19 | no (asm only) · no |
| DSi | ARM9 134 | S8DS, smooth GG upscaler 30+ fps | 37 | marginal · no |
| 3DS | ARM11 ×2 268 | PicoDrive full speed 2D; Gearsystem ~15 fps (unverified) | 75 | marginal · marginal |
| New 3DS | ARM11 ×4 804 | most cores full speed | 225 | yes · yes |
| PSP | Allegrex 222 / 333 | SMS Plus PSP full speed with FM off; GPGX "too slow" | 62 / 93 | marginal · marginal at 333 |
| Pi Zero / W | ARM11 1000 | lr-picodrive default (armv6) | 279 | yes · yes |
| Miyoo Mini (+) | Cortex-A7 ×2 1200 | Onion PicoDrive first | 335 | yes · yes |
| RG35XX H/Plus/SP, RG28XX, RG40XX | Cortex-A53 ×4 1500 | muOS smsplus / PicoDrive | 419 | yes · yes |
| TrimUI Smart Pro | Cortex-A53 ×4 2000 | – | 559 | yes · yes |
| RP2350 / Teensy 4.1 | M33 252–378 / M7 600 | pico-smsplus, MCUME full speed | 70–106 / 168 | marginal · no (RP2040 no) |
| WASM, 3 GHz desktop / 2 GHz phone | ÷1.75 overhead | jsSMS recompiler "full speed on mobile" (2013) | ~480 / ~320 | yes · yes |

Specs [ECO.62]. **Recommendation:** burst/lazy design with per-pixel register sampling only where a write lands mid-line;
v6.0 full-speed tiers are Pi Zero and every A7/A53 handheld, New 3DS, WASM; PSP at 333 MHz and old 3DS are "measure, then
decide"; DS, DSi, GBA and RP2040 stay with S8DS, SMSAdvance and pico-smsplus. **Strongest alternative:** a line-lump fast
profile for DS/PSP, rejected (two timing models, two hash sets). **Would change if:** v1 measures ≤ 1.5 M Ir/frame (PSP
and old 3DS become comfortable) or ≥ 6 M (Pi Zero and Miyoo slip).

## 5. Compatibility measurement in the ecosystem
### 5.1 Library sizes
| List | SMS / Mark III | Game Gear | SG-1000 | SC-3000 | Source |
|---|---|---|---|---|---|
| No-Intro (libretro mirror 2026.10.07; dumps incl. revisions) | 1,057 | 607 | 211 | – | [ECO.59] |
| MAME hash lists (1d7a454ed79e; 4 / 5 / 2 unsupported) | 653 | 823 (incl. SMS-mode carts) | 214 | 30 cart + 31 cassette | [ECO.35] |
| MEKA compat.txt tested | 666 | 527 | 195 | 35 | [ECO.11] |
| ares compatibility page | 410 | 468 | 179 | – | [ECO.05] |
| Wikipedia titles (18 JP-only, 4 NA-only, 158 PAL-only, 22 Brazil-only) | 314 | 365 | – | – | [ECO.63] |
| speedrun.com games | 284 | 186 | – | – | [ECO.12] |
| RetroAchievements sets (DevJam start 2026-05-01) | 149 sets, 4,989 achievements | 119 sets, 4,021 | – | – | [ECO.50] |
| TASVideos publications | 74 | 25 | 6 | – | [ECO.38] |
SMS Power! counts by region (Brazil Tec Toy, Korea Samsung, Japan Mark III) and homebrew competition entries: not
fetched (site down); open question.

### 5.2 RetroAchievements
Console IDs `RC_CONSOLE_MASTER_SYSTEM = 11`, `RC_CONSOLE_GAME_GEAR = 15`, `RC_CONSOLE_SG1000 = 33` (dfdc64ea035c)
[ECO.64]. Hash: whole-file MD5, no header skipping, extensions `.sms .gg .sg` [ECO.64]. Memory map SMS and GG: System RAM
$C000–$DFFF at RA address 0–$1FFF; "Cartridge RAM" 32 KiB virtual block at RA $2000–$9FFF ("real" $10000), exposed as
`RETRO_MEMORY_SAVE_RAM` whether battery-backed or not; SG-1000: 1 KiB at $C000, expansion B at $C400, A at $2000, Othello
and The Castle at $8000 [ECO.64]. Supported: GPGX, Gearsystem, RAMeka, Pizza Boy SC [ECO.50]. cogmaster: identical
descriptors and the 32 KiB cart-RAM view [v1.0].

### 5.3 How others report compatibility
GPGX: blanket "100%" plus a hardware-requirement list (BIOS-required California Games II; legacy VDP modes F-16, Penguin
Adventure, Wonsiin; extended modes "Master System II & Game Gear only" for Cosmic Spacehead, Micro Machines, Fantastic
Dizzy; Ys (J) on Mark III/SMS I only) [ECO.07]. MEKA: per-game text with device flags and tallies [ECO.11]. ares: a
status table (Completable/In-Game/Starts/Nothing) [ECO.05]. Gearsystem: ZEXALL and SMS VDP Test screenshots, no pass/fail
text [ECO.08]. MiSTer: GitHub issues [ECO.27]. **Recommendation:** a generated ledger per dump hash with status, machine
profile, evidence (hardware capture, oracle agreement, TAS sync) and a not-a-bug list, published as `docs/compatibility.md`
[v1.0]; GPGX's requirement list becomes profile rows, never hacks.

## 6. The slot cogmaster fills
No SMS/GG emulator combines a permissive licence, an embeddable C library, exact timing, cross-platform determinism,
first-class tooling and a libretro core. What people run is non-commercial (GPGX in every Apple frontend, RetroArch,
BizHawk; PicoDrive on handhelds) or GPL (Gearsystem, smsplus-gx, BlastEm, jgenesis, MesenCE); the best tooling is closed
(Emulicious); the openable code is slow by design (ares) or a frontend-bound driver (MAME).
- Permissive cores today: Snepulator, CLK, TotalSMS, gears (MIT) with no libretro core, no published timing evidence and
  no movies; Cega (MPL-2.0) is cycle-stepped and claims 100 % but is tied to the Jolly Good API [ECO.32][ECO.33].
- None publishes cross-platform frame/audio hashes, a measured ledger, a speed gate or an Ir budget.
- No RL environment; TAS only in BizHawk; RA only on GPGX, Gearsystem, RAMeka.
- Underserved: homebrew developers wanting CI-grade determinism with symbols; frontend authors who cannot ship NC code;
  handheld distros that fall back to PicoDrive for speed; browser users without a permissive accurate WASM core.
- **Strongest reason this is wrong:** Cega could add a libretro core and relicense; Snepulator is wiki-recommended and
  moving (push 2026-10-18); Gearsystem's MCP debugger wins developers first. **Would change if:** any permissive core
  passes a hardware-verified checklist with states and a libretro adapter.

## 7. Open questions
- SMS Power! game-list and competition counts, and its forum threads on Chuck Rock, Road Rash, Dynamite Headdy GG, Ecco,
  F-16 and The Jungle Book line-interrupt behaviour: site returned 503 all day; fetch when up (also SP file).
- Speed of ares v148, GPGX, Gearsystem and Emulicious on one named CPU (nobody publishes); measure beside Ir/frame in v1.
- Real IPC of emulator code on ARM11, Allegrex and Cortex-A7: the 3DS/PSP verdicts hinge on it.
- Is BizHawk's SMSHawk source MIT (repo statement) or unlicensed (no header)? Treated as unopened.
- RetroAchievements SG-1000 set count; current SMS/GG counts (site is bot-blocked; DevJam figures are a start date).
- Whether TASVideos would accept a standalone runner's movies; whether BizHawk wants a native C SMS core.
- GG LCD characteristics (ghosting, colour) from measurements, for the optional host-side model.
- Sonic Origins Plus GG emulator author and its known stereo/zoom faults, as a "what not to do" row.

## 8. Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| ECO.01 | T2 | emulation.gametechwiki.com/index.php/Master_System/Game_Gear_emulators (SMS and GG pages redirect here) | revid 151020 (2026-10-01), accessed 2026-10-18 | ratings, recommended set, dead projects |
| ECO.02 | T1 | emulicious.net; /License.txt; /news/ | accessed 2026-10-18 | licence, debugger features, 2026-03-19 release |
| ECO.03 | T4 | RetroRGB and Retro Dodo articles on Emulicious / Gearsystem | accessed 2026-10-18 | praise |
| ECO.04 | T1 | github.com/ares-emulator/ares LICENSE, releases | 9408cb43d494; v148 | ISC, dates |
| ECO.05 | T1 | ares-emu.net/compatibility | accessed 2026-10-18 | 400/410, 462/468, 178/179 |
| ECO.06 | T1 | github.com/ekeeke/Genesis-Plus-GX LICENSE.txt, README.md | 49c584764893 | NC clause, 100 % claim |
| ECO.07 | T1 | docs.libretro.com/library/genesis_plus_gx/; GPGX wiki/Compatibility.md | accessed 2026-10-18 | options, BIOS md5, requirement list |
| ECO.08 | T1 | github.com/drhelius/Gearsystem README, releases | 190873a34c3f; 3.9.22 | GPL-3, debugger, accuracy tests |
| ECO.09 | T1/T4 | docs.libretro.com/library/gearsystem/; Gearsystem issues 117, 116 | accessed 2026-10-18 | options, "instruction accurate" |
| ECO.10 | T1 | github.com/ocornut/meka README.md, meka/meka.txt, meka/sources.txt | 4c00b68ed08f | licence gray area, author's self-assessment |
| ECO.11 | T1 | github.com/ocornut/meka meka/compat.txt | 4c00b68ed08f | tallies 666/527/195/21/35 |
| ECO.12 | T1 | speedrun.com/api/v1 platforms, games?platform=…, Alex Kidd categories and variables | accessed 2026-10-18 | IDs, counts, rules |
| ECO.13 | T1 | github.com/TASEmulators/BizHawk LICENSE; src/…/Sega/SMS/SMS.cs, SMS.Input.cs, SMS.IEmulator.cs (headers and public attributes only) | c9de9a8bff0f | licence, lag rule, LogKey buttons |
| ECO.14 | T2 | tasvideos.org/HomePages/CoolHandMike/Bizhawk292ReleaseNotesDraft; tasvideos.org/Forum/Posts/452590 | accessed 2026-10-18 | gpgx SMS in BizHawk, Pause NMI |
| ECO.15 | T1 | mednafen.github.io/documentation/sms.html | 1.32.1 | SMS Plus lineage, low priority |
| ECO.16 | T1 | github.com/mamedev/mame COPYING; src/mame/sega/sms.cpp header | mame0289 | GPL whole, BSD-3 files |
| ECO.17 | T1 | github.com/libretro/smsplus-gx README.md, docs/license | 3844b46caa92 | GPL-2+, no LICENSE file |
| ECO.18 | T1 | muos.dev/systems/homeconsole/mastersystem, /handheld/gamegear, /homeconsole/sg1000; onionui.github.io/docs/emulators | accessed 2026-10-18 | handheld defaults |
| ECO.19 | T1 | emulatorjs.org/docs/systems/sega-master-system, sega-game-gear | accessed 2026-10-18 | browser defaults |
| ECO.20 | T1 | retrodev.com/blastem/changes.html; github.com/libretro/blastem | 1.0.0 2026-09-11 | BlastEm GG/SG support, web build |
| ECO.21 | T1 | github.com/SourMesen/Mesen2 README (archived); github.com/nesdev-org/MesenCE | 2.1.1; 2.2.1 | SMS/GG support, GPL-3 |
| ECO.22 | T1 | RetroPie-Docs Master-System.md, Game-Gear.md; RetroPie-Setup lr-genesis-plus-gx.sh, lr-picodrive.sh; wiki.batocera.org systems:mastersystem; Recalbox wiki (snippet) | accessed 2026-10-18 | distro defaults |
| ECO.23 | T1/T2 | github.com/Plombo/dega README; tasvideos.org/1754S | accessed 2026-10-18 | Dega licence, TAS history |
| ECO.24 | T1 | fms.komkon.org/MG/ | 4.9 | MasterGear |
| ECO.25 | T1 | github.com/irixxxx/picodrive COPYING, README; docs.libretro.com/library/picodrive/ | v2.05; 63fa85c41821 | NC, weak-device focus |
| ECO.26 | T4 | retropie.org.uk/forum/post/29403, /post/64934; forum.clockworkpi.com/t/1611 | accessed 2026-10-18 | PicoDrive SMS complaints |
| ECO.27 | T1 | github.com/MiSTer-devel/SMS_MiSTer README.md, rtl/T80, rtl/VM2413, rtl/jt89 headers, issues 194, 167, 164, 146, 145, 130 | 8443a0d19a88 | features, licences, open issues |
| ECO.28 | T1 | github.com/Spiritualized1997/openFPGA-SMS, openFPGA-GG | 1.2.0; 1.3.0 | Pocket cores |
| ECO.29 | T1 | github.com/rileytestut/Delta README; Provenance-Emu/Provenance README, LICENSES.md; OpenEmu/OpenEmu README | accessed 2026-10-18 | Apple frontends use GPGX |
| ECO.30 | T2 | Wikipedia Sega Ages, Sega Genesis Classics, Mega Drive Mini 2, Sonic Origins; pushsquare 2023-03 | accessed 2026-10-18 | official releases |
| ECO.31 | T1 | github.com/gmarty/jsSMS; anisse/gears; retrodev.com/blastem/web/1.0.0 | accessed 2026-10-18 | browser emulators |
| ECO.32 | T1 | github.com/JoppyFurr/Snepulator; TomHarte/CLK; ITotalJustice/TotalSMS (gh api licence, dates) | v26.06; 2026-07-23; 2025-07-10 | permissive newcomers |
| ECO.33 | T1 | gitlab.com/jgemu/cega README; gitlab API tags | 0.6.3 2026-09-11; e31a931b | Cega licence, cycle-stepped, 100 % claim |
| ECO.34 | T1 | github.com/jsgroth/jgenesis LICENSE, README, cpu/z80-test-runner | v0.14.1; b4f943bb5f6f | GPL-3, ZEXALL runner |
| ECO.35 | T1 | github.com/mamedev/mame hash/sms.xml, gamegear.xml, sg1000.xml, sc3000_cart.xml, sc3000_cass.xml (CC0-1.0 header) | 1d7a454ed79e | entry counts |
| ECO.36 | T4 | github.com/libretro/Genesis-Plus-GX issues 371, 372; ekeeke/Genesis-Plus-GX 596, 659 | accessed 2026-10-18 | line-interrupt and run-ahead issues |
| ECO.37 | T4 | github.com/ares-emulator/ares issues 259, 403, 546, 828, 2269, 2260 | accessed 2026-10-18 | ares open issues, not-a-bug |
| ECO.38 | T1/T2 | tasvideos.org PlatformFramerates, EmulatorResources, Movies-SMS/-GG/-SG1000, EmulatorResources/MMV, Bizhawk/BK2Format, MovieRules | accessed 2026-10-18 | rates, counts, formats, rules |
| ECO.39 | T4 | forums.libretro.com/t/44648; lr-GPGX issues 61, 138, 118; GPGX 191, 377, 402, 537; Gearsystem 105; picodrive 59; mametesters 8648; bitbucket eke #275 | accessed 2026-10-18 | C.01–C.03 |
| ECO.40 | T4 | GPGX issues 412, 660, 352, 409, 474, 410, 644; lr-GPGX 290, 403; Gearsystem 122, 125, 109; ares 830; consolemods SMSFM | accessed 2026-10-18 | C.04 |
| ECO.41 | T4 | ares issues 1257, 1258, 400, 640, 829, 1011; Gearsystem 134, 135; GPGX 272, 262, 558, 425; lr-GPGX 392, 305 | accessed 2026-10-18 | C.05 |
| ECO.42 | T4 | lr-GPGX issues 31, 136, 131, 396, 366, 340; GPGX 325, 88; Gearsystem 114, 116, 117, 112, 111, 130, 184; smsplus-gx 45; forums.libretro.com/t/16776 | accessed 2026-10-18 | C.06, C.07 |
| ECO.43 | T4 | GPGX 203; lr-GPGX 59; bitbucket eke #258; ares 828; misterfpga.org p=4728; meka.txt 3-D section | accessed 2026-10-18 | C.08 |
| ECO.44 | T4 | GPGX 440, 424, 500, 430, 433; Gearsystem 115, 133, 110, 119; ares 259, 403; picodrive 78; MEKA release 20250817 | accessed 2026-10-18 | C.09 |
| ECO.45 | T4 | GPGX 75, 76, 447; Gearsystem 153 | accessed 2026-10-18 | C.10, G.05 |
| ECO.46 | T4 | GPGX 524, 273, 316; forums.libretro.com/t/19150; Gearsystem 80; MEKA 155 | accessed 2026-10-18 | C.11 |
| ECO.47 | T4 | GPGX 596, 261; lr-GPGX 372, 330, 168, 393, 43; ares 2269, 2260; Gearsystem 45, 191; retropie.org.uk/forum/post/147744 | accessed 2026-10-18 | C.12, C.13 |
| ECO.48 | T4 | Gearsystem 113; lr-GPGX 165; GPGX 615, 616, 378; ares 841; smsplus-gx 5; picodrive 30, 166 | accessed 2026-10-18 | C.14 |
| ECO.49 | T4 | Gearsystem 35, 145; lr-GPGX 50; GPGX 331; retropie.org.uk/forum/post/55022 | accessed 2026-10-18 | C.15 |
| ECO.50 | T1 | docs.retroachievements.org/general/emulator-support-and-issues.html; /developer-docs/devjam/10-mastersystem-gamegear.html; /developer-docs/game-identification.html; MEKA issue 140 | accessed 2026-10-18 | supported emulators, set counts, MD5 rule |
| ECO.51 | T4 | GPGX 348, 483, 602; lr-GPGX 218, 333, 387, 282; Gearsystem 108, 88, 90, 138; MEKA 122, 148, 142; picodrive 108; smsplus-gx 42 | accessed 2026-10-18 | C.17 |
| ECO.52 | T4 | benryves.com/journal/2828698 (Desert Speedtrap latching); ares issue 55; Gearsystem 33 | accessed 2026-10-18 | C.18 |
| ECO.53 | T4 | MEKA issues 70, 63, 60, 32, 28, 54, 86; Gearsystem 85, 97, 179, 89, 120, 139, 108; GPGX 361; ares 1273 | accessed 2026-10-18 | C.21, G.02, praise |
| ECO.54 | T1/T4 | picodrive issues 232, 215; forums.libretro.com/t/4828; gametechwiki Emulators_on_3DS, Emulators_on_PSP (snippets) | accessed 2026-10-18 | C.22, 3DS/PSP speed |
| ECO.55 | T1 | github.com/FluBBaOfWard/S8DS README; gamebrew.org SMSAdvance_GBA, SMS_Plus_PSP; fhoedemakers/pico-smsplus; Jean-MarcHarvengt/MCUME; ducalex/retro-go | v1.1.8/1.1.9; accessed 2026-10-18 | low-power anchors |
| ECO.56 | T4 | nesturbator `.planning/preparation/NES-ECOSYSTEM.md` (relayed precedent) | 8ae2c6e32e7f | latency precedent |
| ECO.57 | T4 | muhtari `.planning/PRODUCT-ROADMAP.md` and `A7800-ECOSYSTEM-AND-USERS.md` (process precedent only) | updated 2026-10-15 | horizon pattern, Playstead contract |
| ECO.58 | T1 | ale.farama.org; github.com/Farama-Foundation/stable-retro docs/supported_emulators.md | accessed 2026-10-18 | no SMS RL env |
| ECO.59 | T1 | github.com/libretro/libretro-database metadat/no-intro/*.dat (version 2026.10.07), cht/ | 12c21cf79f1e | No-Intro and cheat counts |
| ECO.60 | T2 | smspower.org/Development/ClockRate, /forums/8161-SMSDisplayTiming, uploads/Development/msvdp-20021112.txt, /Development/SN76489 (fetched by the low-power agent before the outage) | accessed 2026-10-18 | 228 cycles/line, 342 px, 262/313 lines, PSG ÷16 |
| ECO.61 | T1 | usenix.net atc19-jangda.pdf (WASM overhead) | accessed 2026-10-18 | 45–55 % slower |
| ECO.62 | T1/T2 | 3dbrew.org Hardware; ppsspp.org Allegrex overview; Wikipedia DS, DSi, GBA, PSP, Raspberry Pi; cnx-software Miyoo Mini; muos.dev/devices; linux-sunxi TRIMUI Smart Pro | accessed 2026-10-18 | device specs |
| ECO.63 | T2 | en.wikipedia.org List_of_Master_System_games, List_of_Game_Gear_games | accessed 2026-10-18 | 314 / 365 titles, region exclusives |
| ECO.64 | T1 | github.com/RetroAchievements/rcheevos include/rc_consoles.h, src/runtime/rc_consoleinfo.c, src/hash/rc_hash.c | dfdc64ea035c | console IDs, memory regions, whole-file MD5 |
