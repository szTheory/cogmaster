---
title: "Hosts and integration: libretro, RetroAchievements, RetroArch on Mac, runner + Playstead, automation, WASM"
summary: "What every host needs from cogmaster (libretro adapter for SMS and Game Gear, rcheevos memory and hash rules, RetroArch on Apple Silicon, cogmaster-run and Playstead's protocol, scripting, WASM) and which milestone each piece lands in."
read_when: "Building the libretro adapter or .info, the runner CLI, saves, the RetroArch e2e job, a Playstead pin, scripting/Gym/TAS surfaces, or a web build."
updated: 2026-10-18
---

# Hosts and integration

Markers: (tested) run here on 2026-10-18, (derived), (untested), (unverified). "Lnnn" = line of libretro.h at RetroArch v1.22.2 [HI.01]. Milestone labels follow muhtari's pack (v1.0 core + runner + PSG and YM2413 sound (DEC.08), v2.0 saves + library breadth, v3.0 states + movies + continuation, v4.0 measured accuracy, v5.0 tools/AI, v6.0 ports); GSD-HANDOFF may renumber. Another console appears only as labelled precedent.

## Where to look
| Question | Section |
|---|---|
| Exports, geometry, fps, aspect, options, firmware, input, saves, serialize, `.info`, buildbot | 1 |
| rcheevos console IDs, SMS/GG memory regions, RA hash, hardcore, disallowed options | 2 |
| RetroArch install, pin + digest, low-latency settings, bounded hosted e2e | 3 |
| `cogmaster-run` exit table, options, `--probe`, first fault, Playstead pin + continuation | 4 |
| Inspection/hooks, Python vs Lua, Gym env, movies (BK2, MMV), lag, debugger | 5 |
| Web build and browser precedents | 6 |
| Feature → host or core | 7 |

## 1. libretro contract
**Exports.** All 25 entry points (L7511–7840) in nesturbator's ~350-line adapter shape, kept by muhtari [HI.22]: one instance `retro_init`→`retro_deinit`, statics reset in deinit, no game → test card (`SET_SUPPORT_NO_GAME` 18, L1051), XRGB8888 via `SET_PIXEL_FORMAT` 10 (L865) plus RGB565 when `USE_RGB565` is defined, stereo batch audio (GG is stereo; SMS mono duplicated), ROM copied at load (`need_fullpath` false). `retro_get_region` 0/1 from the loaded region. `retro_reset` = the console Reset button on SMS (a game-readable input, so "Restart" keeps RAM) and a power cycle on GG (no Reset button). **Rec:** as stated. **Alt:** power cycle everywhere (Gearsystem's Restart "resets the machine and preserves battery memory" [HI.06]). **Would change if:** users expect a cold boot from Restart.

| AV field | SMS NTSC | SMS PAL | GG | Derivation |
|---|---|---|---|---|
| base w × h | 256 × 192 (224/240 via `SET_GEOMETRY` 37 on mode change) | 256 × 192/224/240 | 160 × 144; 256 × 192 when the "full field" option is on | VDP active sizes per MAME `m_y_pixels` 192/224/240 [HI.19]; Gearsystem/GPGX ship the same bases [HI.06] |
| max w × h | 256 × 240 | 256 × 240 | 256 × 240 | one buffer for every mode; border/overscan modes are a host filter (§7) (derived) |
| fps | 1640625/27379 = 59.922751 | 13300856/267615 = 49.701459 | = NTSC | (945 MHz/88)/(2·342·262); (53,203,424/5)/(2·342·313); 342 dots/line, 262/313 lines [HI.19][HI.20] (tested) [HI.24] |
| sample_rate | 48000 | 48000 | 48000 | 801.03 / 965.77 samples per frame → 1,024-sample buffer; Gearsystem and GPGX use 44100 [HI.06] (derived) |
| aspect_ratio | 32/21 ≈ 1.5238 (192 rows), 64/49 (224), 128/105 (240) | 1.848 (192), 1.584 (224), 1.479 (240) | 10/9 ≈ 1.111 (square LCD pixels) | PAR = half the 4:3 square-pixel rate ÷ dot clock: (12.2727/2)/5.3693 = 8/7; (14.75/2)/5.3203 = 1.386; GPGX exposes these as "NTSC PAR"/"PAL PAR", Gearsystem defaults to square pixels (4:3 at 256×192, 10:9 at 160×144) [HI.06][HI.24] (derived) |

**Rec:** dot-clock-correct PAR for SMS (8/7 NTSC), square pixels for GG, both computed in the adapter (its only floating point). **Strongest alternative:** Gearsystem's square pixels everywhere (4:3 for 256×192). **Would change if:** the owner prefers 4:3 fill; RetroArch can override per user anyway.

**Core options** (`SET_CORE_OPTIONS_V2` 67, L2339; v1 fallback via `GET_CORE_OPTIONS_VERSION` 52). Keys follow the three existing cores so users recognise them [HI.06]:

| Key | Values (default first) | In state hash | Lands |
|---|---|---|---|
| `cogmaster_machine` | auto (header region code / extension), sms, sms2, gg, gg_sms_mode (GPGX `system_hw`, Gearsystem `system`) | yes | v1.0 |
| `cogmaster_region` | auto, ntsc-u, ntsc-j, pal (GPGX `region_detect`, smsplus `region`; rcheevos forbids `pal` on both, §2) | yes | v1.0 |
| `cogmaster_boot` | fast (HLE hand-off), bios (file missing → fast + OSD; Gearsystem exposes `bios_sms`/`bios_gg` toggles) | yes | v1.0 |
| `cogmaster_fm` | auto (header/known list), off, on (GPGX `ym2413`, Gearsystem `ym2413`) | yes | v1.0 phase 4 (DEC.08; SND-02) |
| `cogmaster_gg_screen` | lcd (160×144), full (256×192 field; GPGX `gg_extra`) | no (adapter crop) | v1.0 |
| `cogmaster_border` | off, top_bottom, full_284, full_320 (Gearsystem `overscan`; GPGX `overscan`) | no (host filter, §7) | v2.0 |
| `cogmaster_left_column` | auto (crop when the VDP masks it), never, always (Gearsystem `hide_left_bar`, GPGX `left_border`) | no | v2.0 |
| `cogmaster_3d_glasses` | both (off), left, right, anaglyph (Gearsystem `glasses` has the first three) | no (adapter) | v2.0 |
| `cogmaster_crosshair` | off, on (+ shape/colour as Gearsystem) | no | v2.0 |
| `cogmaster_sprite_limit` | on (hardware 8 per line), off (GPGX/Gearsystem `no_sprite_limit`) | yes: breaks frame hashes; stays default-on | v2.0 |
| `cogmaster_psg_volume`, `cogmaster_fm_volume` | 100, 0–200 (Gearsystem) | no (mix) | v4.0 |

Port devices are `SET_CONTROLLER_INFO` (35, L1506) subclasses, not options: SMS pad (default), GG pad (only in GG mode), Light Phaser (port 1 and 2; GPGX lists both), Paddle (port 1), Sports Pad (ports 1–2), MD pad (B/C read as 1/2, verify in the cartridge file), none. **Rec:** these seven. **Alt:** Gearsystem's five (no MD pad). **Would change if:** rcheevos reviewers reject the list; else add `{"cogmaster_region","pal"}` to rcheevos' disallowed table as GPGX and smsplus have [HI.04].

**Firmware** (from `GET_SYSTEM_DIRECTORY` 9, L850): libretro's `System.dat` lists `bios.sms`, `bios_E.sms`, `bios_U.sms` (all 8,192 B, crc 0072ED54, md5 840481177270d5642a14ca71ee72844c), `bios_J.sms` (8,192 B, crc 48D44A13, md5 24a519c53f67b00640d0048ef7089105) and `bios.gg` (1,024 B, crc 0EBEA9D4, md5 672e104c3be3a238301aceffc3b23fd6) [HI.03]. GPGX's `.info` declares `bios_E.sms`, `bios_U.sms`, `bios_J.sms`, `bios.gg`; Gearsystem and smsplus declare `bios.sms` and `bios.gg`; all `opt = "true"` (tested) [HI.02][HI.06]. **Rec:** five `firmwareN` rows, all optional; look up `bios_<U|E|J>.sms` by region, fall back to `bios.sms`, then `bios.gg`; accept any 8 KB / 1 KB image, log its SHA-256 and whether it matches a known md5. **Alt:** GPGX's four names only. **Would change if:** a game depends on a BIOS revision (region lock-out behaviour belongs to the system file).

**Input** (`SET_INPUT_DESCRIPTORS` 11, L882). RetroPad layout = GPGX "MS Joypad 2 Button" = Gearsystem "SMS / GG Pad" = smsplus, so remaps carry over (tested from the docs' icon tables) [HI.06]:

| RetroPad | SMS / GG | Notes |
|---|---|---|
| D-pad | directions | |
| B, A | button 1, button 2 | all three cores; `RETRO_DEVICE_ID_JOYPAD_MASK` 256 (L380) for one poll |
| Start | Pause (SMS, NMI) / Start (GG) | all three cores |
| Select (port 1) | Reset (SMS) | Gearsystem only; GPGX/smsplus leave Select unused |
| Light Phaser | `RETRO_DEVICE_LIGHTGUN` 4 (L238): SCREEN_X/Y 13/14 → column/row, TRIGGER 2, IS_OFFSCREEN 15, RELOAD 16 (L411–417); GPGX names A/B/C/Start aux ids; a pointer (touchscreen) variant as both cores offer | L238–420 |
| Paddle | `RETRO_DEVICE_ANALOG` 5 (L260): left stick X → paddle (GPGX); Gearsystem also maps RetroMouse X | L389–393 |
| Sports Pad | left stick X/Y → trackball, B/A → 1/2 | both cores |

**Rec:** GPGX/Gearsystem layout with Gearsystem's Select = Reset. **Alt:** Reset on L3. **Would change if:** players report accidental resets.

**Saves.** `RETRO_MEMORY_SAVE_RAM` (0, L509) = the cartridge's non-volatile span, fixed from load to unload; RetroArch writes `.srm` on unload and at `autosave_interval` [HI.05]; the adapter does no file I/O. Layout, raw: Sega-mapper battery RAM up to 32,768 B (two 16 KB pages); 93C46 EEPROM 128 B (BizHawk's SMS core keeps `EEPROM.93c46.cs`; Gearsystem has an "EEPROM 93C46" mapper and writes "battery RAM or EEPROM" to `.srm`) [HI.06][HI.12]; non-battery mapper RAM (Codemasters 8 KB) stays out of the span. rcheevos notes that libretro cores expose "extended RAM" as `SAVE_RAM` regardless [HI.04] (see §2). **Rec:** one raw `.srm` per game, EEPROM as 128 B little-endian words (v2.0). **Alt:** always 32 KB padded with $FF so size never changes. **Would change if:** a frontend chokes on a 128 B file or Playstead's medium table wants fixed sizes.

**Serialize.** Size constant between load and unload (L7699), 0 without content; no `SET_SERIALIZATION_QUIRKS` (44, L1696); byte-wise little-endian; `GET_SAVESTATE_CONTEXT` (72, L2430) ignored. Run-ahead and rewind need only speed and determinism; netplay compares content and states [HI.22] → byte-identical states on six platforms + big-endian qemu. VRAM 16 KB + RAM 8 KB + cart RAM dominate size (derived). **Rec:** LE always. **Alt:** BE in the rollback context. **Would change if:** a frontend really exchanges context-3 states across machines.

**`.info`.** `display_name "Sega - MS/GG (cogmaster)"`, `corename "cogmaster"`, `systemname "Sega 8-bit (MS/GG)"`, `systemid "master_system"` (Gearsystem and smsplus; GPGX uses `mega_drive`), `database "Sega - Game Gear|Sega - Master System - Mark III"` (+ `Sega - SG-1000` with the compatibility seed), `manufacturer "Sega"`, `supported_extensions "sms|gg|bin|rom"` (+ `sg` later), `license "MIT"`, `needs_fullpath`/`hw_render`/`disk_control`/`load_subsystem` false, `cheats "true"` (RetroArch's engine), `display_version` from `version.txt` [HI.02]. Each claim maps to its proving tests at configure time (nesturbator pattern [HI.22]):

| Claim → true | Lands | Guard test |
|---|---|---|
| `supports_no_game` | v1.0 | in-process `libretro.host`, NULL game → test-card hash |
| `input_descriptors`, `core_options` + `core_options_version "2.0"` | v1.0 | descriptor ids valid; key list equals the one sent to rcheevos review |
| `memory_descriptors` (false until §2 Q1 is settled; Gearsystem says false, GPGX true) | v2.0 | vendored rcheevos resolver over our map |
| `libretro_saves` | v2.0 | in-process `.srm` round trip + hosted case 2 (§3) |
| `savestate`; `savestate_features` basic → serialized → deterministic | v3.0 | size constant; save+load every frame for 600 frames = straight run; six-platform state hashes; "deterministic" is what Gearsystem and GPGX declare [HI.02] |
| `is_experimental "false"` | v2.0 | compatibility-ledger pass count ≥ threshold |

RetroArch disables rewind/run-ahead/netplay above the declared level and reads the level only when the `.info` sits beside the core [HI.22].

**Buildbot (v6.0).** libretro ci-templates (pin `96f603ee450e`, from muhtari's pack, not re-fetched) carry CMake variants for every modern target incl. console statics: `-DLIBRETRO_STATIC=ON -DLIBRETRO_SUFFIX=_<platform>`, target `${CORENAME}_libretro`, artefact `${CORENAME}_libretro_<platform>.a` (`.bc` for emscripten) linked into RetroArch, so every global symbol needs the `cogmaster_`/`retro_` prefix [HI.09]. The Linux image is Ubuntu 16.04 + GCC 9 + CMake 3.31.7 [HI.10] → C17 and a CMake 3.25 floor fit. Gearsystem's libretro core already ships on Vita, PS3, 3DS, Wii/WiiU, Switch and Emscripten [HI.06], so the recipe is proven for this console class. The `.info` PR goes to libretro-core-info [HI.02]. **Rec:** CMake-only. **Alt:** add `Makefile.libretro`. **Would change if:** PS3 or old MSVC builds are wanted.

## 2. RetroAchievements
rcheevos `rc_consoles.h`: `RC_CONSOLE_MASTER_SYSTEM = 11`, `RC_CONSOLE_GAME_GEAR = 15`, `RC_CONSOLE_SG1000 = 33`; extension map `sms`→11, `gg`→15, `sg`→33 (`rc_hash.c` L1248–1280) [HI.04] (tested at develop `dfdc64ea035c`, 2026-10-17; latest tag v12.5.0; RetroArch 1.22.2 bundles v12.1.0 per muhtari [HI.22]). SMS and GG share one region table (`rc_consoleinfo.c` L528–538, L604–614):

| rcheevos address | Real address | Type | cogmaster exposure |
|---|---|---|---|
| $0000–$1FFF System RAM | $C000 (8 KB; mirrored at $E000 on hardware) | SYSTEM_RAM | `RETRO_MEMORY_SYSTEM_RAM` (2, L517) = the 8 KB work RAM |
| $2000–$9FFF "Cartridge RAM" | $10000, a virtual block "mapping all cartridge memory in one contiguous block" (32 KB) | SAVE_RAM | `RETRO_MEMORY_SAVE_RAM` (0) = battery span; rcheevos says cores expose non-battery "extended RAM" here too |

SG-1000 (L871–881) differs: 1 KB RAM at $C000 plus expansion RAM at $C400, $2000, $8000. Without descriptors rcheevos reads `SYSTEM_RAM` then `SAVE_RAM` as flat buffers (`rc_libretro.c` L497–511); with a memory map it resolves the real addresses through the descriptors (L575, L753–770) [HI.04]. Mirrors would share one `ptr` with different `offset` (L3700s) [HI.01]; call `SET_SUPPORT_ACHIEVEMENTS` (42, L1660) true in `retro_load_game`.
- **Hash** = MD5 of the whole file, capped at 64 MB (`MAX_BUFFER_SIZE`, `rc_hash_internal.h` L26); SMS/GG/SG fall in the generic `rc_hash_buffer` / `rc_hash_whole_file` cases (`rc_hash.c` L533–556, L822–842) [HI.04] (tested) → no header stripping; a 512-byte copier header changes the hash (Q2). `--probe` prints it as `md5`.
- **Disallowed options** (hardcore): GPGX `region_detect = pal` and `lock_on` cheats, smsplus `region = pal` (`rc_libretro.c` L92–96, L145–148) [HI.04] → register `cogmaster_region = pal` the same way and keep cheat devices out of the core.
- **Hardcore** is enforced by RetroArch (no state loading, rewind, slow motion, frame advance; memory editors, scripting and input playback banned) [HI.11]. RA lists Genesis Plus GX and Gearsystem (libretro) and RAMeka/Pizza Boy SC (standalone) for both Master System and Game Gear; unlisted cores may be excluded from hardcore [HI.11] → ask RA in v6.0 once the buildbot ships cogmaster. Sets were authored against those cores' flat `SYSTEM_RAM`/`SAVE_RAM`, so the two-block exposure above is what sets expect (derived).
- **Licence:** rcheevos is MIT (LICENSE at pin) [HI.04] → vendor `rc_libretro.c` + `rc_consoleinfo.c` as the CTest oracle: every rcheevos address in both tables resolves through our exports to the byte the core's own peek returns.

**Rec:** flat `SYSTEM_RAM` 8 KB + `SAVE_RAM` battery span, `memory_descriptors "false"` at first (Gearsystem's position), descriptors added when a cheat or set needs bus addresses. **Alt:** descriptors from v1.0 as GPGX. **Would change if:** Q1 shows RetroArch cheats need offset = CPU address, or an RA set reads a mirror or a register.

## 3. RetroArch on Apple Silicon
- **Install:** `brew install --cask retroarch-metal` (universal 1.22.2 DMG from buildbot stable; the cask's sha256 is `no_check`, so Homebrew verifies nothing) or the DMG; the `retroarch` cask is the x86_64 DMG; the App Store build is sandboxed [HI.07][HI.22]. 1.22.2 (2025-11-20) is still the newest release (tested) and the owner's machine has it installed (`kMDItemVersion` 1.22.2) [HI.08]. **Pin:** `https://buildbot.libretro.com/stable/1.22.2/apple/osx/universal/RetroArch_Metal.dmg`, 232,801,022 B, SHA-256 `81b79121ba26d539064ae13b4d0419a120c3d165afbe656cf5f5412b15fdb434`, equal to muhtari's and nesturbator's pins (tested: `curl -fsSL -o RetroArch_Metal.dmg <url> && shasum -a 256 RetroArch_Metal.dmg`) [HI.08][HI.22].
- **Core + `.info`:** `~/Library/Application Support/RetroArch/{cores,info}`; delete `info/core_info.cache` after an `.info` change; curl downloads carry no quarantine; the linker's ad-hoc signature suffices [HI.22].

| Low-latency setting (cfg key) | Value | Why (untested with cogmaster) |
|---|---|---|
| video driver | Vulkan via MoltenVK (default in the Metal build since 1.15.0) or Metal | swapchain depth is a Vulkan/D3D knob [HI.05] |
| `video_max_swapchain_images` | 2 | default 3 (retroarch.cfg L191) → one frame less queued [HI.05] |
| `video_frame_delay` / auto | auto (default 0, L203) | a cogmaster frame ≈ 1 ms if ~1,000 fps (derived); manual delay blocked in RA hardcore since 1.10.3 |
| `video_swap_interval` | auto (L187) | ProMotion 120 Hz: each 59.92 Hz frame shown twice |
| `vrr_runloop_enable` | on only with an Adaptive-Sync display | paces exactly 59.9228 / 49.7015 Hz |
| `audio_latency`, `audio_rate_control_delta` | 32–48 ms (default 64, L324); 0.005 (L331) | 59.9228 vs 60 Hz = 0.13 %, inside DRC's ±0.5 % (derived) |
| `run_ahead_frames` / `preemptive_frames_enable` | 1, preemptive preferred | needs `savestate_features "deterministic"` (v3.0) |

**Rec:** preemptive frames 1 + auto frame delay + swapchain 2. **Alt:** run-ahead 1 in second-instance mode. **Would change if:** RetroArch's latency statistics show stalls at cogmaster's frame cost.

What RetroArch's CLI can do unattended (man page + `retroarch.c` option table at v1.22.2) [HI.05]:

| Flag | Use here | Note |
|---|---|---|
| `-L PATH` | load the built core | overrides the config's core |
| `-c PATH` / `--appendconfig A,B` | isolated config; Playstead can append `savefile_directory` | later files win |
| `--max-frames=N`, `--max-frames-ss`, `--max-frames-ss-path=FILE` | run N frames, screenshot, exit | the only exit-after-N path; no config key does this |
| `--verbose` | log to stdout/stderr, captured by the driver | needed to diagnose a refused load |
| `--subsystem NAME` | not used; Gearsystem's `gg_link_2p` shows the shape | a two-ROM GG link is a later seed |
| `-S`/`-U` save and state dirs | not relied on: unverified in the man page at this pin | use config keys instead |

**Bounded hosted e2e** (RetroArch aborts in the agent sandbox → hosted only [HI.22]). Same approach as muhtari, verified in its tree (tested): one macos-15 arm64 job fetches the pinned DMG through `scripts/fetch-pin.sh` (SHA-256 checked before unpacking), `hdiutil attach … -nobrowse -readonly`, `ditto` the one `.app`, checks `CFBundleShortVersionString` = 1.22.2; a CMake `-P` driver creates a fresh run directory with every directory the config names, writes an isolated config from `test.cfg.in` (`config_save_on_exit=false`, `video_gpu_screenshot=false`, `audio_driver=null`, `pause_nonactive=false`, `menu_driver=rgui`, `bundle_assets_extract_enable=false`, all paths inside the run dir), sets `HOME`/`CFFIXED_USER_HOME` to the run dir, and runs `RetroArch --verbose --config test.cfg -L CORE --max-frames=N --max-frames-ss --max-frames-ss-path=shot.png [ROM]` with a 50 s timeout; the long-option table in `retroarch.c` L7004–7006 defines the three `max-frames` flags, `--appendconfig` and `-L` are in the man page [HI.05][HI.22]. muhtari's job is 241 lines (`run_retroarch.cmake` 54, `test.cfg.in` 46, `bmp.c` 87, `compare.c` 54) plus ~70 workflow lines; `sips` converts PNG → BMP and a 54-line C compare prints the first differing pixel; a negative control flips one byte and expects exit 1 [HI.22].
Case 1 (v1.0): no content → test card; later an own diagnostic ROM; screenshot = runner `--png N` after the same palette, byte-exact at 256×192 (GG: 160×144). Case 2 (v2.0): an own ROM increments a counter in battery RAM at boot and draws it; run 1 (no `.srm`) shows 1, run 2 shows 2; compare both shots and the `.srm` bytes with the runner. Nothing else: no UDP commands, no RetroArch rewind/run-ahead/netplay (proved in-process). Budget ≤ 300 test lines enforced by a line-count test; POSIX sh only, run under `dash` in CI; no stateful shell harness (nesturbator's grew to 7,706 lines [HI.22]). **Rec:** two cases. **Alt:** in-process only. **Would change if:** a RetroArch update breaks loading unnoticed (add a version check, not a case).

## 4. Runner `cogmaster-run` and Playstead
Exit statuses: one X-macro table, printed by `--help`, README equality test [HI.22]: **0** done (also after SIGTERM/SIGINT: finish the frame, flush saves, exit 0) · **1** failure (I/O, corrupt or hostile image, failure after fixture access) · **2** usage · **3** hash mismatch (`--hash`/`--expect`) · **4** save size mismatch · **5** state rejected · **77** capability refusal (unsupported mapper or device, BIOS required but absent, SG-1000 before the seed lands).

| Option | Meaning | Lands |
|---|---|---|
| `[--rom] FILE`, `--machine auto\|sms\|gg`, `--region auto\|ntsc-u\|ntsc-j\|pal`, `--frames N`, `--hash N`, `--png N:FILE`, `--bench N` | none → test card; SHA-256 of native pixels; PNG via host palette; fps + p50/p90/p99/p99.9/max µs | v1.0 |
| `--probe` | header fields below, no emulation; exit 0/77/1 | v1.0 |
| `--boot fast\|bios`, `--bios FILE` | BIOS bytes are an input; SHA-256 printed | v2.0 (LIB-03; phase 1 CONTEXT D-08) |
| `--port1/--port2 DEV` | auto · pad · none (v1.0); lightgun · paddle · sportspad · mdpad (v2.0) | v1.0 / v2.0 |
| `--expect FILE`, `--capabilities` | compare printed hashes; JSON, writes nothing | `--expect` v1.0 phase 3; `--capabilities` v3.0 (phase 1 CONTEXT D-08) |
| `--hash-audio` | PCM SHA-256 per channel | v1.0 |
| `--trace first-fault` | stop at the first unmapped or illegal access and print it (format below) | v1.0 |
| `--save-dir DIR`, `--save-interval F`, SIGUSR1 | `DIR/<rom basename>.sav`, temp → fsync → rename | v2.0 |
| `--input FILE` (movie) | replays per-frame input; prints the lag count | v1.0 |
| `--record FILE`, `--state-at-frame N:FILE`, `--load-state FILE`, `--hash-state N` | movie recording and states | v3.0 |
| `--trace full`, `--symbols FILE.sym`, `--vdp-dump N`, `--lua` | §5 | v5.0 |

No window, config file, environment, stdin or network; invalid combinations exit 2 [HI.22]. **First-fault line:** `fault frame=N line=L dot=D pc=$XXXX op=$XX addr=$XXXX width=8|16 dev=<rom|ram|vdp|psg|io|mapper|unmapped> kind=<read|write|exec>`; one line, then exit 0 (the fault is data, not failure) unless `--expect` disagrees. **`--probe` output** (one `key=value` per line): `size`, `copier_header` (size mod 16,384 = 512), `header_at` ($7FF0; also $1FF0/$3FF0), `header_magic`, `checksum`, `checksum_ok`, `product_code`, `version`, `region_code` (3 SMS Japan, 4 SMS Export, 5 GG Japan, 6 GG Export, 7 GG International), `rom_size_code`, `machine_guess`, `region_guess`, `mapper_guess` (none ≤ 48 KB; Codemasters when its $7FE0 header checks; Korean/MSX by known-hash list; else Sega), `sha1`, `sha256`, `crc32` (RetroArch scanner), `md5` (= the RetroAchievements hash, §2), and the same four over the file with a copier header removed as `*_headerless`. Header-field semantics come from the cartridge file; the probe never emulates.

| Playstead pin field [HI.21] | `cogmaster-run` value | Lands |
|---|---|---|
| system | `sms` and `gg`: both absent from the frozen server list (`gba gb gbc nes snes md psx unknown`, `system_id.ex`); `AdapterCatalog` derives accepted extensions as `.<system>`, so the ids must equal the file extensions; adding them is a Playstead-side owner task (D-14 registry change) | v2.0 |
| emulator, version, download_url, sha256 | macos-arm64 runner zip from the attested release; digest from SHA256SUMS | v1.0 |
| launch.executable_relative_path | `bin/cogmaster-run`; Playstead's installer mounts a DMG and copies one `.app` (`AdapterInstaller.swift` L306–340) → a zip/CLI path is a Playstead change | Playstead |
| argument_template | `["--save-dir","{saveDir}","--rom","{romPath}"]` (`AdapterPin.swift` substitutes only `{romPath}` and `{saveDir}`) | v2.0 |
| config_injection | `cli_flags`: save_directory `--save-dir {path}`; bios_path `--boot bios --bios {path}` (the host appends the rendered `bios_path` template, `AdapterHost.swift`); controller_mapping n/a (headless) | v1.0 / v2.0 |
| save_contract | glob `{saveDir}/{romBaseName}.sav`; media `sram_8k` 8,192, `sram_16k` 16,384, `sram_32k` 32,768, `eeprom_93c46` 128; `size_is_medium_identity` true; binding `rom_sha256` = headerless SHA-256 | v2.0 |
| flush triggers; on_demand_flush_supported | within `--save-interval` frames of a change, SIGUSR1, SIGTERM/SIGINT, exit; true | v2.0 |
| worst_case_loss_seconds | interval ÷ 59.92 (60 frames ≈ 1.0 s) on SIGKILL/crash; 0 on SIGTERM (flush inside Playstead's 2 s grace before SIGKILL, `AdapterProcessRegistry.terminateAll`) | v2.0 |
| exit_detection | clean 0/exit (also after SIGTERM); crash 6 or 11 uncaughtSignal; killed 9 | v2.0 |

**Rec:** SIGTERM → flush → 0. **Alt:** a dedicated "stopped by signal" status. **Would change if:** Playstead must tell host-stop from completion. Playstead today plays only through a GUI adapter (mGBA `.app`, `Contents/MacOS/mGBA`); the runner is headless, so play goes through the pinned RetroArch DMG + a core zip: RetroArch's DMG fits the one-`.app` installer, but the core and `.info` must land beside it and the launch needs `-L <core> --appendconfig <cfg with savefile_directory={saveDir}> {romPath}`, a private config Playstead already knows how to write (`MGBAInputBridge.writePrivateConfig`) → a two-artifact pin and a config writer are the Playstead-side change [HI.21]. RetroArch on macOS installs no SIGTERM handler (`platform_unix.c` L2735–2737 does; `platform_darwin.m` has none, `retroarch.c` L1013 leaves `install_signal_handler` NULL) → SIGTERM is status 15 "killed" with no `.srm` flush; `autosave_interval` bounds the loss (derived from the grep, untested) [HI.05].

Continuation protocol (v3.0) [HI.21]: an owner-local adapter wraps the runner with exactly one action per call (`qualify`, `initial-run`, `continue`), a fresh private dir, no network (a symbol-allowlist test proves the runner has no socket code, v1.0). A pass needs qualified capabilities, an initially empty save dir, a populated one after the initial run, safe exit, a distinct fresh child, a completed Continue and at least two agreeing visible-state oracle runs; exits 0 pass / 77 refusal before fixture access / 1 failure after match the runner table.

| Action | Runner use | Facts asserted | Lands |
|---|---|---|---|
| qualify | `--capabilities` (dir stays empty) | initial_run, safe_exit, fresh_process, continue, repeatable_visible_oracle, oracle_agreement | v3.0 |
| initial-run | `--rom F --input A --save-dir persistent-save --frames N` | save_transition (empty → populated), safe_exit (0, no temp files) | v3.0 (saves v2.0) |
| continue | per oracle run: copy the save dir, fresh process, `--input B --frames M --hash M` | fresh_process, continue_complete, oracle_agreement (≥ 2 equal hashes) | v3.0 |

## 5. Scripting and automation (YAGNI order)
| # | Surface | Design | Lands |
|---|---|---|---|
| 1 | Inspection | `cogmaster_peek` (bus view without port side effects, mapper slot changes or lag credit), VDP register/VRAM/CRAM getters, Z80 registers, line and frame counters; property test: peeking all 64 KB + VRAM leaves the state hash unchanged | v1.0–v3.0 |
| 2 | Hooks | instruction, bus-access, I/O-port, line and frame observers; the loop compiled twice, the traced copy chosen at run entry; `cogmaster_run(budget, &stop)` for breakpoints; Cachegrind proves hooks-off Ir unchanged | v5.0 |
| 3 | Python | ctypes over the C API (stdlib; releases the GIL per call → threaded envs); wheels carry the release library | v5.0 |
| 4 | Gymnasium env | below | v5.0 |
| 5 | Lua in the runner | `--lua FILE` on the hook API; Lua is MIT; 5.4 matches BizHawk and MAME though their APIs differ [HI.16] | on demand |

**Rec:** Python first. **Alt:** Lua first (no packaging, runs on consoles). **Would change if:** homebrew devs or TASers ask for in-emulator scripts.

**Gym env** (ALE/Gymnasium conventions [HI.14]): `obs_type` native (192×256 uint8 palette indices; GG 144×160 with 12-bit colour needs rgb) · rgb · ram (8 KB + cart RAM); actions: 9 directions × {none, 1, 2, both} = 36, minimal per game; Pause/Start stays outside the action set (`reset()` presses it); `frameskip` 4 and `repeat_action_probability` 0.25 live in the wrapper, drawn from its seeded `np_random`, never in the core; `reset(seed)` seeds the wrapper RNG and the core's RAM-fill profile; vector env = N instances on threads. Throughput hypothesis ≥ 2,000 frames/s/core with conversion off (a Z80 at 3.58 MHz is about a NES's work; nesturbator measured 855–1,100 [HI.22]; ALE-in-EnvPool ≈ 3,500–6,300 [HI.15]); measure before claiming.

**Movies.** **Rec:** own versioned text format (v3.0): header (headerless ROM SHA-256, machine, region, boot + BIOS SHA-256, port devices, FM on/off, RAM fill + seed, behaviour revision, start = power-on | state hash | save hash) and one line per frame with absolute button states per port plus gun/paddle axes. Imports in v5.0, both formats researched (tested from their specs) [HI.12][HI.13]:
- **BK2** (BizHawk, the only TASVideos-accepted SMS/GG format): a zip of `Header.txt`, `Input Log.txt`, `SyncSettings.json`, `Comments`, `Subtitles`, optional `CoreState`; input lines start with `|`, one character per boolean (`.` = off), analog as 5-digit numbers with commas; the log key is informational only. SMS buttons are `P1/P2 Up Down Left Right B1 B2` + console `Pause` (GG: `P1 Start`), mnemonics from the base table `U D L R 1 2` and `p`/`S` (`Bk2MnemonicLookup.cs` L16–64); `Platform = SMS` even for GG, with `IsGGMode` in the header; devices per port are Standard, Paddle, SportsPad, Phaser with a `Trigger` button and axes (`SMSControllerDeck.cs` L128–136) [HI.12].
- **MMV** (Dega; TASVideos lists it as "not accepted" today): 244-byte binary header (`MMV\0`, version, frame count, rerecords, "from reset" flag, state and input offsets, packet size, 64-byte author, flags bit 1 PAL / bit 2 Japan / bit 3 Game Gear, 128-byte ROM name, 16-byte ROM MD5), then 2 bytes per frame (port 1, port 2): bit 0 up, 1 down, 2 left, 3 right, 4 button 1, 5 button 2, 6 SMS Pause (set for one frame), 7 GG Start; movies starting from a state are refused by BizHawk's importer too [HI.12][HI.13].
**Alt:** BK2 as the native format. **Would change if:** TASVideos accepts cogmaster. Format interop ≠ sync: both importers re-time inputs against cogmaster's own frame boundaries (derived).

**Lag frame:** a frame in which the CPU never reads the controller ports ($DC/$DD and their mirrors on even/odd ports $C0–$FF) nor, on GG, port $00 (Start); peeks excluded. BizHawk clears its lag flag in exactly those three reads (`SMS.Input.cs` L22–66, L176–186) [HI.12]; Light Phaser and Paddle also read $DC/$DD, so they count (derived). **Sub-frame input. Rec:** per-frame input applied at VBLANK start; `cogmaster_input` stays size-tagged so a tick-stamped queue can follow. **Would change if:** a TAS or a game shown to sample mid-frame (Light Phaser latches H-counter mid-line, which is already modelled inside the frame) needs it.

**Debugger** (the SMS homebrew bar is Emulicious [HI.17]): debugger with breakpoints, watchpoints and conditional expressions, reverse stepping, profiler, tracer, coverage analyser, memory editor, palette/tile/tilemap/sprite viewers, sound viewer, RAM search/watch, memory tracer, source view and VS Code remote debugging; symbols from WLA-DX `.sym` files whose sections are `[labels]` (`bank:offset name`), `[definitions]`, `[breakpoints]`, `[symbols]`, `[source files v2]`, `[rom checksum]`, `[addr-to-line mapping v2]`, `[sections]`, `[ramsections]` (WLA-DX 10.8a) [HI.18]. Split: **core API** = peek/poke, register and VDP state getters, hooks, bank state, line stats, coverage bitmap (zero cost off); **runner** = trace with tick + beam row/dot, `--symbols` label resolution, VDP dump at frame N, lag counter; **host** (not in this repo) = viewers, source mapping, a DAP server. Precedent: MAME's debugger has break/watch/register points and `trace` [HI.17 via muhtari]; BizHawk's SMS core exposes System Bus, Main RAM, Video RAM, ROM, Save RAM and Cart (Volatile) RAM domains and a lag flag (`SMS.IMemoryDomains.cs`) [HI.12]; Gearsystem's repo now advertises a built-in debugger with an embedded MCP server (description only, not opened) [HI.23].

## 6. WASM (the hash lane may come in phase 1)
**Rec:** Emscripten build of the core + a ~150-line JS harness (canvas 256×192 or 160×144, AudioWorklet at 48 kHz, keyboard + Gamepad API); content = test card, own MIT diagnostic ROMs; plus a WASI build of `cogmaster-run` under Node or wasmtime as hash lane seven. The core already satisfies the needs: no threads (no SharedArrayBuffer → no COOP/COEP headers), no files, allocator hook, no mutable globals; check 32-bit `size_t` in state-size math [HI.22]. **Alt:** RetroArch's web player via `emscripten-static-cmake` (statically linked into GPL-3 RetroArch, output `${CORENAME}_libretro.{wasm,js}`) [HI.09]. **Would change if:** the owner wants shaders and menus on the web.
Browser precedents for comparison (metadata only, none opened: all GPL-3) [HI.23]: jsSMS (SMS + GG, dynamic recompiler, last push 2023-07, hosted page live), Miracle (SMS, last push 2026-04, hosted page live), EmulatorJS (RetroArch web frontend listing Sega Master System and Game Gear systems), Gearsystem's libretro core on the Emscripten target [HI.06].

## 7. Feature → host or core
| Feature | Owner | Core provides |
|---|---|---|
| Palettes, GG 12-bit → RGB conversion | adapter (outside hashes) | native 6-bit SMS / 12-bit GG colour per pixel |
| LCD ghosting filter (GPGX `lcd_filter`) | host helper in C, opt-in | 160×144 native frames |
| GG border art, bezels, CRT shaders, scanlines, screenshots, recording | RetroArch overlays/shaders | nothing |
| Aspect correction | adapter reports PAR; RetroArch applies | geometry per mode |
| Border/overscan modes (240/288 lines, 284/320 columns) | adapter composes from backdrop colour | 256×240 max buffer + backdrop colour |
| 3-D glasses: left/right select, anaglyph, alternate | adapter (frame parity from the $FFF8 glasses latch) | the glasses latch state per frame |
| Light-gun crosshair | adapter draws (shape/colour options as Gearsystem) | gun position in |
| Cheats (Game Genie, Pro Action Replay) | RetroArch cheat engine | memory exports (§2); `retro_cheat_set` no-op |
| Patches (IPS/BPS/UPS) | RetroArch softpatching (`need_fullpath` false) | load copies bytes |
| Rewind, run-ahead, netplay | RetroArch | fixed-size deterministic LE state |
| Input mapping, remaps, turbo | RetroArch; adapter maps RetroPad → device | pin model per port |
| FM/PSG mix balance (Gearsystem `psg_volume`/`fm_volume`) | adapter mixes two streams | PSG and FM as separate sample streams |
| Sprite-limit removal | core option, default on (hardware); off breaks frame hashes and is excluded from conformance | per-line 8-sprite logic with a bypass |
| Battery files | host writes `.srm` / runner writes `.sav` | battery span + save generation |
| Achievements | RetroArch (rcheevos) | `SYSTEM_RAM` + `SAVE_RAM` exports |
| Region detect, fast boot, FM detect, mapper detect | core | header + hash list |
| Lag counter, line stats, trace, coverage | core (zero cost off) | inspection + hooks |

## Open questions
1. RetroArch's cheat engine: does it read `SYSTEM_RAM` offsets or descriptors for SMS/GG `.cht` files? 10-minute local check with one libretro-database file (decides `memory_descriptors`).
2. RA sets and 512-byte copier headers: are headered dumps hashed as-is (whole-file MD5 says yes) and do any sets exist for them? One scan with the owner's RA web API key.
3. RetroArch DMG on SIGTERM: the grep says no handler on macOS (status 15, no flush); confirm by one run before Playstead relies on `autosave_interval`.
4. Which driver does the Metal build pick by default on Apple Silicon, and does Metal honour the swapchain setting?
5. MD pad on an SMS port: which buttons map to 1/2 and does TH matter (cartridge/peripheral file to settle before the subclass ships)?
6. Playstead: two-artifact pins (RetroArch + core), a config writer for `--appendconfig`, zip/CLI install, and the `sms`/`gg` registry rows (owner tasks).

## Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| HI.01 | T1 | github.com/libretro/RetroArch `libretro-common/include/libretro.h` (7,846 lines) | `69a4f0ea1e8a` (v1.22.2); SHA-256 `bd3398d29c3763d18617087020ff56b5450a48a63123403a4b674f8e79947acb`; file last changed at `677e6df54c45` (2026-10-17) on master | entry points, env ids, devices, memory ids, descriptors |
| HI.02 | T1 | github.com/libretro/libretro-core-info `gearsystem_libretro.info`, `smsplus_libretro.info`, `genesis_plus_gx_libretro.info` | `5a74858ab2f7` | info keys, systemid, databases, firmware rows, savestate_features |
| HI.03 | T1 | github.com/libretro/libretro-database `dat/System.dat` L383–390 | `12c21cf79f1e` | BIOS names, sizes, crc/md5 |
| HI.04 | T1 | github.com/RetroAchievements/rcheevos `include/rc_consoles.h`, `src/runtime/rc_consoleinfo.c` L528–538, L604–614, L871–881, `src/hash/rc_hash.c` L402–407, L533–556, L822–842, L1248–1280, `src/hash/rc_hash_internal.h` L26, `src/libretro/rc_libretro.c` L92–96, L145–148, L497–511, L575, L753–770, `LICENSE` (MIT) | `dfdc64ea035c` (develop, 2026-10-17); latest tag v12.5.0 = `1433173220a7` | console ids, regions, hash rule, disallowed options, resolver |
| HI.05 | T1 | RetroArch `retroarch.cfg` L187–331, L895, L986, `docs/retroarch.6` L65–67, L88–91, L110–113, L169–170, `retroarch.c` L1013, L7004–7006, `frontend/drivers/platform_unix.c` L2735–2737, `frontend/drivers/platform_darwin.m` (no signal handler); `CHANGES.md` via muhtari | `69a4f0ea1e8a` | settings, CLI flags, signal handling |
| HI.06 | T1 | docs.libretro.com `library/genesis_plus_gx/`, `library/gearsystem/`, `library/smsplus/`, `development/cores/developing-cores/` | accessed 2026-10-18 | options, device types, RetroPad tables, geometry, BIOS, platforms, Emscripten |
| HI.07 | T2 | formulae.brew.sh/api/cask/retroarch-metal.json | accessed 2026-10-18 | cask 1.22.2, `no_check` |
| HI.08 | T1 | Local measurement: DMG download + `shasum -a 256`; `mdls` of the installed app; GitHub releases listing | 2026-10-18 | DMG digest and size, newest release (tested) |
| HI.09 | T1 | git.libretro.com/libretro-infrastructure/ci-templates | `96f603ee450e` (from muhtari's pack, not re-fetched) | buildbot contract |
| HI.10 | T1 | git.libretro.com/libretro-infrastructure/libretro-build-amd64-ubuntu `Dockerfile.xenial-gcc9` | `869fce5ac223` (from muhtari's pack) | GCC 9, CMake 3.31.7 |
| HI.11 | T1 | docs.retroachievements.org `general/emulator-support-and-issues.html`, `general/hardcore-compliance-requirements.html` | accessed 2026-10-18 | listed SMS/GG emulators, hardcore rules |
| HI.12 | T1/T3 | github.com/TASEmulators/BizHawk `src/BizHawk.Emulation.Cores/Consoles/Sega/SMS/{SMS.ISaveRam,SMS.IInputPollable,SMS.Input (L22–66, L176–186),SMS.IMemoryDomains,SMSControllers,SMSControllerDeck (L12–40, L128–136),EEPROM.93c46}.cs`, `src/BizHawk.Client.Common/movie/import/MmvImport.cs`, `src/BizHawk.Emulation.Common/Base Implementations/Bk2MnemonicLookup.cs` L16–64; `LICENSE` (C# parts MIT) | `c9de9a8bff0f` | devices, SaveRam, lag, domains, BK2/MMV import |
| HI.13 | T1 | tasvideos.org `EmulatorResources`, `EmulatorResources/MMV`, `Bizhawk/BK2Format` | accessed 2026-10-18 | accepted formats, MMV layout, BK2 layout |
| HI.14 | T1 | ale.farama.org, gymnasium.farama.org `api/env/` | accessed 2026-10-15 (muhtari) | obs/action/frameskip/sticky/seed API |
| HI.15 | T1 | github.com/sail-sg/envpool README; arxiv.org/abs/2206.10558 | accessed 2026-10-15 (muhtari) | Atari frames/s |
| HI.16 | T1 | lua.org/license.html; MAME `3rdparty/lua`; BizHawk `lua54.dll` | via muhtari | Lua MIT, versions |
| HI.17 | T2 | emulicious.net (home page feature list); docs.mamedev.org debugger via muhtari | accessed 2026-10-18 | debugger bar, tools |
| HI.18 | T1 | wla-dx.readthedocs.io `en/latest/symbols.html` (10.8a) | accessed 2026-10-18 | `.sym` sections and formats |
| HI.19 | T3 | github.com/mamedev/mame `src/devices/video/315_5124.h` L36–52, L167, `src/mame/sega/sms.cpp` L256–264, L531, L600–603, L679–695 (both `license:BSD-3-Clause`) | `1d7a454ed79e` | 342 dots, 262/313 lines, 192/224/240, XTAL 10,738,635 and 53,203,424/5, GG 32,215,905/3 |
| HI.20 | T2 | smspower.org `uploads/Development/msvdp-20021112.txt` (Charles MacDonald) L757–766, L878, L946 | accessed 2026-10-18 | 262/313 lines, 342 pixels per line |
| HI.21 | T1 | playstead `playstead-mac/Playstead/Adapter/{AdapterPin.json,AdapterCatalog.swift,AdapterHost.swift,AdapterInstaller.swift L306–340,AdapterPin.swift L5–21}`, `playstead-server/lib/playstead/formats/system_id.ex`, `playstead-mac/docs/CONTINUATION-PROTOCOL.md` L51–75, `playstead-mac/scripts/ci/continuation_protocol.py` | local checkout, accessed 2026-10-18 | pin fields, installer, grace period, frozen ids, protocol |
| HI.22 | T1 | muhtari `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.github/workflows/ci.yml` L290–360, `tests/retroarch/{run_retroarch.cmake,test.cfg.in,bmp.c,compare.c}` (241 lines), `scripts/fetch-pin.sh`; nesturbator figures as cited there | local checkout `9499ad881a52`, accessed 2026-10-18 | adapter shape, e2e method, speeds, e2e size history |
| HI.23 | T1 | GitHub repository metadata (`gh api`) for gmarty/jsSMS, mattgodbolt/Miracle, drhelius/Gearsystem, EmulatorJS/EmulatorJS; HTTP 200 on gmarty.github.io/jsSMS and xania.org/miracle; emulatorjs.org/docs/systems | accessed 2026-10-18 | licences, activity, hosted builds, systems list (no source opened) |
| HI.24 | T1 | Arithmetic run here (Python `fractions`): fps rationals, PAR, samples per frame | 2026-10-18 | fps, aspect, buffer sizing (tested) |
