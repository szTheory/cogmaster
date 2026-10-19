---
title: "SMS audio: Sega PSG, Game Gear stereo, YM2413 FM, synthesis and hashes"
summary: "Clock-exact facts and settled choices for cogmaster sound: the SN76489-variant PSG inside the VDP (16-bit LFSR, taps 0 and 3, period 0 = period 1, 2 dB table), the Game Gear stereo port $06, the YM2413 on Japanese machines (ports $F0-$F2, mute bits, instrument tables and their licences), integer band-limited synthesis with a transition-stream hash, per-frame sample rationals, and the test and oracle plan."
read_when: "Before designing, implementing or testing psg.c, fm.c, synth.c, the mixer, port $06/$F2 handling, audio hashes, the audio output contract or the libretro audio path."
updated: 2026-10-18
---

# SMS audio

## Where to look
| Question | Section |
|---|---|
| Clocks, native rates, samples per frame | 1 |
| PSG registers, latch/data protocol, tone counters, period 0 and 1, volume table | 2 |
| Noise: LFSR width, taps, periodic mode, reset on write, rates | 2.3 |
| PSG output polarity, DC, mixing, power-on | 2.4 |
| Game Gear port $06, headphone vs speaker, SMS mode on a Game Gear | 3 |
| YM2413: ports, detection, $F2 mute bits, instruments, rate, export consoles | 4 |
| Instrument table provenance and licences | 4.3 |
| FM/PSG balance | 4.4 |
| Synthesis, hashes, output contract, cost bound, libretro | 5 |
| A/V sync | 6 |
| Tests, test ROMs, VGM oracles, reference binaries | 7 |
| User complaints about other emulators | 8 |
| Open questions, risks, phase that settles each | 9 |

Licence status of the T3 sources opened for this file: MAME `sn76496.cpp` (BSD-3, Nicola Salmoria), MAME `sms.cpp`, `sms_m.cpp`, `315_5124.cpp` (BSD-3), ares `ares/ms/{psg,opll,cpu}` and `ares/component/audio/{sn76489,ym2413}` (ISC, "ares team, Near et al"), ymfm (BSD-3, Aaron Giles), emu2413 (MIT, Mitsutaka Okazaki), sverx/PSGlib (BSD-3), JoppyFurr test ROMs (MIT). All checked at the pins in the Sources table. jt89, VM2413, Genesis Plus GX, Gearsystem and SMS Plus were not opened; their facts come from issue trackers only.

## 1. Time base
| Quantity | NTSC | PAL | Source / derivation |
|---|---|---|---|
| Z80 clock = master/15 (MAME: crystal 10,738,635/3; GG 32,215,905/9) | 3,579,545.45 Hz (315 M/88) | 3,546,895 Hz | [AUD.09][AUD.06]; exact crystal values are owned by SMS-SYSTEM |
| PSG internal tick = Z80/16 | 223,721.6 Hz | 221,680.9 Hz | [AUD.01] (/16 divider); MAME clocks the PSG at VDP clock/3 = Z80 clock [AUD.10] |
| Tone frequency for period N | Z80/(32 N); N = $FE gives 440.4 Hz | | [AUD.01] |
| Noise shift rate, mode bits 00/01/10 | Z80/512, /1024, /2048 = 6,991 / 3,496 / 1,748 Hz | | [AUD.01][AUD.08] |
| YM2413 sample = Z80/72 | 49,715.9 Hz | 49,262.4 Hz | [AUD.05] (fsam = fM/72); MAME clocks the YM2413 at Z80 clock [AUD.09]; ares at colorburst/72 [AUD.11] |
| Frame = 262 or 313 lines x 228 Z80 cycles | 59,736 cycles, 59.9227 Hz | 71,364 cycles, 49.7014 Hz | (derived; line length and line count are owned by SMS-VDP) |
| 48 kHz, samples per Z80 cycle | **176/13,125** (gcd of 48,000 x 88 and 315 M) | **9,600/709,379** | (derived) |
| 48 kHz, samples per frame | 801.03 | 965.77 | (derived) |
| 44.1 / 96 kHz, samples per Z80 cycle | 1,617/131,250; 352/13,125 | 8,820/709,379; 19,200/709,379 | (derived) |
| PSG ticks per frame; YM samples per frame | 3,733.5; 829.67 | 4,460.25; 991.17 | (derived) |

The Game Gear's Z80 is nominally the same 3.579545 MHz [AUD.06]; MAME's 32,215,905/9 = 3,579,545 Hz differs from 315 M/88 by 0.1 ppm [AUD.09]. SMS-SYSTEM decides whether the GG profile shares the SMS rational; this file assumes it does.

## 2. The Sega PSG
The Mark III, SMS, SMS 2 and Game Gear integrate an SN76489 variant into the VDP; the SG-1000, SC-3000 and Othello Multivision use a discrete SN76489AN [AUD.01]. Facts below are for the integrated chip unless marked.

### 2.1 Registers and write protocol
| Item | Fact | Source |
|---|---|---|
| Port | Any write to $40-$7F reaches the PSG ($7F conventional, some games use $7E) | [AUD.07][AUD.01] |
| Latch byte (bit 7 = 1) | bits 6-5 channel, bit 4 type (1 volume, 0 tone/noise), bits 3-0 data | [AUD.01] |
| Data byte (bit 7 = 0) | bits 5-0 go to bits 9-4 of the latched tone register; for volume or noise the low bits are rewritten and the rest discarded | [AUD.01] |
| Latch persistence | The latched register is never cleared by a data byte; a data byte after a volume or noise latch is applied (ignoring it breaks Alex Kidd and Micro Machines) | [AUD.01] |
| Update timing | Tone registers take effect immediately on each byte, not after both halves; the GG manual says data is latched immediately with no wait | [AUD.01][AUD.06] |
| Registers | 3 x 10-bit tone periods, 4 x 4-bit attenuations, 1 x 3-bit noise control | [AUD.01] |
| Power-on (integrated) | tone and noise registers 0, all volumes $F (silent); discrete chips start random (SC-3000 reportedly boots with a tone) | [AUD.01]; MAME sega-style resets volumes to $F and the latch to register 3 [AUD.08] |
| READY | TI chips stall the bus 32 clocks per write; MAME models it for the Sega PSG too (4 x divider clocks) | [AUD.08] (untested on the integrated chip) |

### 2.2 Tone channels and the period 0 question
Each channel has a 10-bit down-counter at the /16 tick; at zero it reloads from the register and toggles its output, so the square wave period is 2N ticks [AUD.01]. **Period 0 and period 1 both give a constant +1 output on the integrated chip** (Maxim's document; used for PCM playback) [AUD.01]. MAME: "all the Sega-made PSG chips act as if the frequency was set to 0 if 0 is written", while every TI chip treats 0 as $400 [AUD.08]. ares reloads with 0 and toggles every tick for both 0 and 1 [AUD.12]: at the output rate that is the same DC level, not a 111.9 kHz tone, after band-limiting. A discrete SN76489 on Mega Drive-era test hardware gave the *lowest* pitch for 0, which is the TI $400 rule; the integrated 315-5313 behaves like 1 [AUD.22]. Highest audible period on an SMS 2 was $006 (18.6 kHz) [AUD.01].

**PSG-0.** **Recommendation:** on the integrated chip (all cogmaster profiles) period 0 behaves as period 1: a constant high level at the channel's volume. Noise mode 3 with tone 2 = 0 therefore shifts at the fastest rate (every tick pair). The SG-1000 profile (later seed) uses $400 and the 15-bit LFSR. **Strongest alternative:** ares' literal "reload 0, toggle every tick" (identical after band-limiting, but doubles transition-stream records for a hostile ROM). **Would change if:** JoppyFurr's test ROM (section 7) on a Mark III shows a tone at period 0.

### 2.3 Volume
Each step attenuates 2 dB (x 10^(-0.1) = 0.7943); 16 levels, $F is off; the TI datasheet range is 0-28 dB [AUD.01][AUD.32][AUD.28]. Maxim's 16-bit table, index = attenuation: **32767, 26028, 20675, 16422, 13045, 10362, 8231, 6568, 5193, 4125, 3277, 2603, 2067, 1642, 1304, 0** [AUD.01] (as printed on the page, re-read 2026-10-18; entry 7 is a transcription slip: 32767 × 10^(−0.7) = 6538, and 6568 breaks the 2 dB law by 0.04 dB, so the checked-in table uses 6538 and the unit test derives every entry from the law). MAME uses the same law scaled to 8191 per channel [AUD.08]; ares uses 2^(-level/3) [AUD.11]. Maxim measured that the **SMS 2 clips the top three levels** (a board-level mistake) while the SMS 1 and Mega Drive follow the law [AUD.01] (one measurement; unverified here).

**PSG-VOL.** **Recommendation:** check in the 16-entry integer table above as the per-channel level in Q15; the channel's level is table[att] when its output bit is 1, else 0. An `sms2_clip` profile flag (default off) caps entries 0-2 at entry 3 for the SMS 2 profile once a second measurement confirms it. **Strongest alternative:** generate the table at init with pow() (floating point in the core, forbidden by AGENTS.md rule 4). **Would change if:** a hardware capture of the 16 steps on an SMS 1 differs from the 2 dB law by more than 0.5 dB.

### 2.4 Noise channel
| Item | Integrated Sega PSG | Discrete SN76489AN (SG-1000, SC-3000H, BBC) | Source |
|---|---|---|---|
| LFSR width | 16 bits | 15 bits | [AUD.01][AUD.16] |
| White-noise taps (bit 2 = 1) | bits 0 and 3 ($0009), feedback into bit 15 | bits 0 and 1 ($0003), into bit 14 | [AUD.01]; MAME SEGAPSG taps $01 and $08, mask $8000 [AUD.08]; ares `(b0 ^ b3) << 15 \| lfsr >> 1` [AUD.12] |
| "Periodic" noise (bit 2 = 0) | only bit 0 fed back: a 16-tick loop, 1/16 duty | 15-tick loop | [AUD.01] |
| Reset | any write to the noise register (latch or data byte) sets the LFSR to $8000 (only the top bit) | $4000 | [AUD.01][AUD.08][AUD.12] |
| Output | the bit shifted out (bit 0) | same | [AUD.01] |
| Shift clock | bits 1-0: 00 = counter 16, 01 = 32, 10 = 64, 11 = tone 2's period; the LFSR shifts once per two counter expiries | same | [AUD.01][AUD.12] |
| XOR vs XNOR | XOR (MAME assumes XOR for Sega, verified for TI) | XOR | [AUD.08] |

Charles MacDonald sampled SMS 1, GG and Mega Drive noise and found the same pattern as the SMS 2 [AUD.01]. Sega's 16-stage periodic loop plays 6.25 % lower than TI's 15-stage loop, which is why SG-1000 titles sound wrong through an SMS core (Gearsystem fixed exactly this in 2024) [AUD.24].

**PSG-LFSR.** **Recommendation:** 16-bit register, feedback = bit0 XOR (bit3 AND white), shifted right, new bit into bit 15, reset to $8000 on every noise-register byte, output = old bit 0, shift on every second counter expiry; the tone 2 period is read live (a later tone 2 write changes the rate at once). The SG-1000 profile carries width 15 and taps $0003 as profile constants. **Strongest alternative:** MAME's generic (mask, tap1, tap2, negate) parameter set. **Would change if:** a JoppyFurr test ROM capture on hardware shows the LFSR survives a data byte after the noise latch (then reset only on the latch byte).

### 2.5 Output polarity, DC and mixing
The chip sums the four attenuated channels into one analog output [AUD.01][AUD.32]. Maxim describes tone outputs as 0/+1 levels that drift toward -0.5/+0.5 through the output stage's AC coupling, with the DC offset restored by each volume change, which is what makes 4-bit PCM playback audible [AUD.01]. MAME negates the Sega output ("assumed to be inverted") [AUD.08]. Polarity is inaudible for a single chip; it matters only for phase against FM on Japanese machines, where the two chips are mixed as separate analog signals (unverified ratio, section 4.4).

**PSG-MIX.** **Recommendation:** level_c = table[att_c] x out_c; mono sum = level_0 + level_1 + level_2 + level_3 (max 4 x 32,767, scaled to the output contract in section 5); one first-order DC blocker (about 20 Hz, Q30 coefficients checked in) after the mix, before the PCM hash. No inversion. **Strongest alternative:** model the leaky coupling with Maxim's curvature. **Would change if:** a hardware recording of 4-bit PCM playback (Maxim's method) differs audibly from the straight-sum model.

## 3. Game Gear differences
| Item | Fact | Source |
|---|---|---|
| Port $06 (write only) | bits 7-4 = noise, tone 3, tone 2, tone 1 to **left**; bits 3-0 the same to **right**; 1 = routed. Sega's manual marks all bits as 1 after power-on except NOSR, which is unmarked; MAME resets the mask to $FF | [AUD.06][AUD.01][AUD.08] |
| Headphones vs speaker | the register affects only the headphone output; the mono speaker always carries every channel, so a channel muted on both sides is still heard on the speaker; Sega tells developers to silence with attenuation, not with $06 | [AUD.06][AUD.01] |
| Headphone insertion | cuts the speaker and gives stereo | [AUD.06] |
| Otherwise | the PSG is the same chip as the SMS 2's (same LFSR, same volume law) | [AUD.01] |
| SMS mode on a GG | MacDonald's "MS mode" port map lists $00-$3F without the GG registers; MAME maps $06 only in the GG-mode I/O view and selects the SMS view when the cartridge declares SMS mode; ares decodes $06 on any LCD machine regardless of mode | [AUD.07][AUD.09][AUD.11] |
| FM on a GG | none; SMS games on a GG play PSG only | [AUD.07] |

**GG-STEREO.** **Recommendation:** the GG profile emits two channels; left = sum of channels whose left bit is set, right likewise; the mask starts at $FF and the mask value is itself a transition-stream source (section 5). In SMS mode on a GG the port is not decoded: the write falls through to the SMS even-address memory-control mirror exactly as an SMS would treat it, the mask stays $FF, and the output is dual mono. A `gg_speaker` host hint (default off) lets a frontend ask for the unmasked mono sum instead. **Strongest alternative:** ares' decode-always. **Would change if:** an SMS-mode test on a real GG with headphones shows panning from a $06 write (then decode always, and move the memory-control hazard to SMS-SYSTEM).

## 4. YM2413 (OPLL) on the Mark III FM Sound Unit and the Japanese SMS
### 4.1 Ports and detection
| Port | Function | Source |
|---|---|---|
| $F0 write | YM2413 address latch (A0 = 0) | [AUD.07][AUD.09] |
| $F1 write | YM2413 data (A0 = 1); the chip needs 12 master clocks after an address write and 84 after a data write, otherwise the value is uncertain | [AUD.07][AUD.05] |
| $F2 write | bits 1-0 = audio control; bits 7-2 ignored | [AUD.02] |
| $F2 read | bits 1-0 = last written value; bits 4-2 = 0; bits 7-5 = bits 11, 7, 3 of a 12-bit counter clocked by C-Sync in the 315-5297 (Charles MacDonald) | [AUD.02][AUD.09][AUD.11] |
| $F2 on export SMS | reads return %10 in the low bits; writes do nothing | [AUD.02] (unverified here) |
| $F2 on a Mark III without the unit | reads return port 0's input; writes do nothing | [AUD.02] |
| I/O chip | $C0-$FF normally reads the I/O chip; detection first sets bit 2 of port $3E (I/O disable) so $F2 reaches the FM logic | [AUD.07][AUD.03] |

Detection (Phantasy Star's routine, reproduced on SMS Power): disable the I/O chip, write bit 0 of a counter to $F2 seven times (1,0,1,0,1,0,1), read back masked to bits 2-0, count matches; 7 matches = FM present; write the result to $F2 (so FM is enabled and, on some hardware, the PSG muted); restore $3E from the RAM copy at $C000 [AUD.03]. Because the compare includes bits 2-1, a unit that returned non-zero there would fail detection.

### 4.2 Audio control bits ($F2 bits 1-0)
| Value | Japanese SMS (both chips mixable) | Mark III + FM Sound Unit (bit 1 forced to 0) | Source |
|---|---|---|---|
| 0 | PSG on, FM off (power-on default) | PSG on, FM off | [AUD.02][AUD.09][AUD.11] |
| 1 | PSG off, FM on | PSG off, FM on | same |
| 2 | both off | behaves as 0 | same |
| 3 | both on | behaves as 1 | same |

SMS Power, MAME and ares agree on this table; SMS Power's bit labels ("bit 1 PSG mute, bit 0 FM mute") do not map onto the value list and are ignored here [AUD.02]. The Mark III unit therefore makes the chips mutually exclusive, which is the "mute PSG when FM enabled" behaviour [AUD.02]. Homebrew FM add-ons may lack the PSG enable [AUD.02]. The Japanese BIOS's Space Harrier theme uses both chips; no released game does [AUD.31].

**FM-CTRL.** **Recommendation:** profile flag `fm_unit` in {none, mark3_unit, smsj}. `none`: $F0-$F2 writes are ignored and $F2 reads follow the export rule (%10 low bits with the I/O chip disabled; the I/O chip's value otherwise). `mark3_unit`: bit 1 masked to 0 before applying. `smsj`: full table. Muting is applied at the mixer, not inside the chips (registers keep running, as MAME's gain switch does [AUD.09]). **Strongest alternative:** ares' two-state model (FM present or absent) [AUD.11]. **Would change if:** a Mark III capture shows $F2 = 2 silencing both chips.

### 4.3 Chip facts and instrument tables
- 9 channels; each selects one of 16 instruments by a 4-bit number; instrument 0 is the user patch in registers $00-$07; rhythm mode (register $0E bit 5) replaces channels 7-9 with bass drum, snare, tom, top cymbal, hi-hat; 9-bit DAC on two pins (MO melody, RO rhythm) summed externally; Yamaha recommends F-numbers $16/$17/$18 = $20/$50/$C0 and $26/$27/$28 = $05/$05/$01 for the rhythm slots [AUD.05][AUD.04].
- Built-in instruments 1-15: Violin, Guitar, Piano, Flute, Clarinet, Oboe, Trumpet, Organ, Horn, Synthesizer, Harpsichord, Vibraphone, Synthesizer Bass, Acoustic Bass, Electric Guitar [AUD.05].
- **No YM2413 die has been imaged.** andete's reverse engineering (2015-2018, github.com/andete/ym2413) worked from measurements and from FHB013 die shots, a derivative with a different instrument set [AUD.04]. Every public instrument table is therefore measured or ear-tuned, not read from silicon; only the VRC7 rhythm patches come from a decap (Nuke.YKT, siliconpr0n) [AUD.15].

| Table | Licence | Provenance as stated in the file | Note |
|---|---|---|---|
| ymfm `ym2413::s_default_instruments` (15 melodic + 3 rhythm rows of 8 bytes) | BSD-3 | "taken from plgDavid's wiki Copyright-free OPLL(x) ROM patches; April 2015 David Viens, tweaked May 2015 Hubert Lamontagne" | row 0 = 71 61 1E 17 EF 7F 00 17 [AUD.14] |
| emu2413 v1.6.4 `default_inst[0]` | MIT | Okazaki, citing andete's notes, ymf262.c, Nuke.YKT's VRC7 presets, Chabin's YMF281B presets; rhythm rows "from VRC7" | row 1 = 71 61 1e 17 d0 78 00 17 [AUD.15] |
| ares `YM2413::melodicTonesOPLL` / `rhythmTonesOPLL` | ISC | none stated (author Talarubi) | row 0 equals emu2413's; row 4 differs from both [AUD.13] |

The two lineages differ in envelope bytes 4-5 of most rows; the difference is audible on sustained notes (untested here). MAME's YM2413 is ymfm, so RetroArch MAME and ymfm-based cores share the first table.

**FM-CORE.** **Recommendation:** a C port of ymfm's OPLL (`ymfm_opl.cpp` OPLL classes plus the shared `fm_engine`, integer-only at runtime; the same path muhtari takes for OPM [AUD.21]) with the BSD-3 notice, and ymfm's instrument table as the default; emu2413's table checked in beside it as `opll_patches_emu2413` behind a profile constant for A/B listening. **Strongest alternative:** emu2413 as-is (already C, MIT, maintained by its author against andete's findings, but its table generation calls sin/pow/log at init, which rule 4 forbids unless the tables are baked offline and checked in). **Would change if:** the ymfm port exceeds 2,500 lines or fails the JoppyFurr YM2413 ROM audibly where emu2413 passes; then bake emu2413's tables and port it instead.

### 4.4 FM/PSG balance
No schematic of the Japanese SMS mixer or the FM unit was found. MAME routes both chips at gain 1.0; its open bug 5849 (2015, acknowledged, unfixed) reports FM far too quiet against the PSG and converges on PSG 0.25-0.35 relative to FM 1.0 by listening, with the BIOS Space Harrier theme as the only both-chip reference [AUD.23]. Gearsystem 2025 issues report FM music audible where it should not be (Monster World 2) and garbled FM after state load [AUD.24]; Genesis Plus GX had the same state-load fault [AUD.26].

**FM-MIX.** **Recommendation:** output = psg_sum x psg_gain + fm x fm_gain, both Q12 profile constants; defaults psg_gain = 0.35 x 4096 = 1434 and fm_gain = 4096 with the YM's 9-bit DAC scaled to the same full-scale as one PSG channel at attenuation 0. The scale is chosen so the largest possible DC sum maps to at most 32,767 (no clipping by construction). **Strongest alternative:** MAME's 1.0/1.0. **Would change if:** a hardware line-out recording of the Japanese BIOS theme (owner's hardware or a published capture) gives a ratio outside 0.25-0.5; then retune and bump the behaviour revision (the transition hash is unaffected).

## 5. Synthesis, hashes, output contract
- **Model:** every source produces level changes stamped in Z80 cycles: PSG channels at the write cycle (volume, combinational) or at the /16 tick (counter toggles), the GG mask at the write cycle, the YM2413 one sample per 72 cycles. Changes are merged per source per cycle; each merged change adds delta x a kernel row into an integer ring buffer; a running sum gives the output. The kernel is the sibling's: 16 taps x 32 phases, Kaiser beta 8, Q15, each row summing to exactly 32,768; measured worst non-harmonic component -87 dB below 16 kHz [AUD.20][AUD.21].
- **Retiming:** phase accumulator +176 per Z80 cycle modulo 13,125 (NTSC 48 kHz); kernel row = acc x 32 / 13,125, remainder interpolates between rows (acc x 32 < 2^19, fits 32-bit). PAL: +9,600 modulo 709,379 (acc x 32 < 2^25). One Z80 cycle is 1/74.57 of a sample, so the 32 phases resolve positions finer than a PSG tick.
- **Rates and bound (derived):** PSG tone at period 1 toggles every tick: 223.7 k changes/s per channel, 895 k for four; volume writes are bounded by the Z80's fastest OUT (11 cycles, 325 k/s) and merge with ticks; YM 49.7 k/s. Worst case about 1.27 M merged changes/s x 16 taps = 20 M multiply-adds/s on a hostile ROM, against about 0.2 M/s for typical music. No "box bins" fallback is needed; keep it as a seed if the wasm32 or handheld bench shows more than 5 % of frame time.
- **Why band-limit:** the PSG's 223.7 kHz tick aliases into the audio band at any plain decimation; the only audible-correct cheap alternative is averaging, which dulls the 18.6 kHz region games do use [AUD.01].

**SYN.** **Recommendation:** reuse nesturbator's `synth.c` kernel (MIT, same author) with per-cycle merging, one DC blocker, stereo as two rings sharing the phase accumulator. **Strongest alternative:** ares-style per-tick float resampler (floating point, forbidden). **Would change if:** the measured spectral test (section 7) shows a component above -80 dB in band.

**HASH.** **Recommendation:** primary audio hash = SHA-256 over little-endian records {u64 cycle, u8 source, u16 level}, one per merged change; sources 0-3 PSG channels (level after attenuation, before the stereo mask), 4 GG mask (level = mask byte), 5 YM2413 (9-bit DAC sum), 6 audio-control state. It is independent of kernel, gains and output rate, computed only in the runner through the hook tap (zero cost when off). Secondary PCM hash per (rate, channels, profile) over the int16 stream. Before freezing PCM digests run the clipping check (0 clipped samples, peak at or below -1 dBFS) over FM titles, PCM-playback titles and GG stereo titles; a synthetic full-scale test once hid 3.04 % clipping in a sibling [AUD.21]. **Strongest alternative:** PCM-only. **Would change if:** hashing costs more than 2 % in the runner.

**OUT.** **Recommendation:** the library emits int16 at a host-chosen rate from {44,100, 48,000, 96,000}, `channels` = 1 for SMS/Mark III/SG profiles and 2 (interleaved L, R) for the GG profile; the host reads `cogmaster_audio_format()` once. The libretro core always delivers interleaved stereo through `retro_audio_sample_batch_t`, duplicating mono, once per `retro_run` with exactly the frame's samples (801 or 802 at NTSC 48 kHz; 965 or 966 PAL), never padded; `retro_get_system_av_info` reports 48,000 Hz and fps 3,579,545.45/59,736 = 59.9227 (PAL 49.7014) so RetroArch's dynamic rate control absorbs the 0.13 % gap to 60 Hz. **Strongest alternative:** always stereo at the library level (simpler, twice the bandwidth on handheld ports). **Would change if:** a second host needs a mono GG speaker mix; then `gg_speaker` (section 3) covers it.

## 6. A/V sync
A frame is one VBlank to the next: 59,736 or 71,364 Z80 cycles, fixed per video standard (the 192/224/240-line modes change the active area, not the line count). The sample count per frame is cycles x 176/13,125 with the remainder carried in the phase accumulator, so the cadence is 801, 801, 801, 802, ... with no drift and no wall clock (rule 4). Run-ahead and state load must save the accumulator, the ring buffer and the DC blocker with the state, otherwise the first frame after a load clicks (the fault behind Gearsystem #109 and Genesis Plus GX #290 is the FM state, not the ring, but the test is the same: load, run, compare hashes) [AUD.24][AUD.26].

## 7. Tests and oracles
| Behaviour | Unit test that proves it (from the datasheet-level facts above) |
|---|---|
| Write protocol | latch then data updates bits 9-4; data after volume latch rewrites volume (Alex Kidd case); latch survives data bytes |
| Tone | period N toggles every N ticks for N in {1, 2, $FE, $3FF}; N = 0 equals N = 1; $FE at 48 kHz measures 440.4 Hz in the PCM FFT |
| Volume | the 16 Q15 entries; monotonic; entry 15 = 0; write takes effect at the write cycle, not the next tick |
| Noise | white sequence from $8000 with taps 0 and 3 matches a 65,535-step reference generated in the test; periodic mode period 16; reset on latch and on data byte; rates 16/32/64/tone 2; tone 2 = 0 under mode 3 |
| GG stereo | mask $F0 gives silence right; $00 silence both (speaker hint restores); SMS-mode write does not change the mask |
| $F2 | detection routine returns 1 on smsj and mark3_unit, 0 on none; value table of section 4.2 per profile; Mark III masks bit 1 |
| YM2413 | register writes with the 12/84-clock gaps; rhythm key-on; instrument 1 note sustained 1 s matches a frozen PCM digest; ymfm vs emu2413 table switch changes only the digest, never the transition count |
| Synth | spectral test: period 1 (111.9 kHz) and period 6 (18.6 kHz) tones below -80 dB in band at 48 kHz; cadence 801/802 over 1,000 frames sums to exactly 801,034 (NTSC) |
| Mix | no overflow at 4 PSG channels at 0 dB plus YM full scale; DC blocker settles within 50 ms |

**Test ROMs and their licences:**
| ROM | Author, date | Licence | Use |
|---|---|---|---|
| SN76489 TestRom (SMS NTSC/PAL, GG, SG-1000 cartridge and SC-3000 tape) | JoppyFurr, 2024 (SMS Power 2024 competition) | MIT [AUD.17] | keyboard-driven tone/noise/volume with a "const" mode; built in CI from source, PCM digests pinned, and the oracle for the §9 hardware questions |
| YM2413 TestRom | JoppyFurr, 2024 | MIT [AUD.18] | FM register exerciser; same use |
| PSGlib and its demos | sverx, 2014- | BSD-3 [AUD.19] | VGM-derived music playback, latch/data edge cases, PCM digests |
| SMS Power spectrogram test (Deflemask project) | kaportza, 2018 | none stated (forum attachment) | local-only listening reference [AUD.27] |
| Heliophobe's Sound Test | unknown, about 2000 | unknown; copy not located | none |

**VGM as oracle.** The VGM 1.71 spec is an open text (Maxim, Valley Bell; no licence statement, freely redistributed with players) [AUD.16]: 44,100 Hz time base; header $0C SN76489 clock, $10 YM2413 clock, $28 feedback ($0009 SMS/GG/MD, $0003 SC-3000H), $2A width (16 or 15), $2B flags (bit 0 period 0 = $400, bit 1 negate, bit 2 stereo off, bit 3 /8 off, bit 4 XNOR); commands $4F dd (GG port $06), $50 dd (PSG), $51 aa dd (YM2413), $61 nn nn / $62 (735) / $63 (882) waits [AUD.16]. The runner gains a `--vgm` input that replays commands at the stated clock through the same chips and emits the transition hash, so a VGM is a register-level fixture independent of the Z80 and VDP. Music from vgmrips carries no licence for the recordings; use such files locally only and never commit them; commit only VGMs rendered from the MIT/BSD test ROMs above.

**Reference binaries as locators:** MAME (`smsj`, `sms`, `gamegear`) and ares run as released binaries only; their PCM is a locator for a wrong octave, a missing channel or a wrong LFSR, never a digest to match, because their resamplers and gains differ from ours.

## 8. User-facing complaints and the response
| Complaint (where) | Cause as reported | cogmaster response |
|---|---|---|
| SG-1000 noise sounds wrong through an SMS core (Gearsystem #100, 2024) | 16-bit $0009 LFSR used for a 15-bit $0003 chip | profile constants per chip (section 2.4) |
| Noise channel a low rumble, not hiss (gendev thread) | tone 2 = 0 under mode 3 treated as the slowest rate | PSG-0 |
| Percussion missing in Alex Kidd (Genesis Plus GX #283) | data byte after a non-tone latch ignored | write-protocol test |
| GG "SEGA" chant missing (Gearsystem #32) | PCM-by-volume playback not reproduced | volume at write cycle, DC model, chant title in the ledger |
| GG stereo missing or wrong (Genesis Plus GX #225, Gearsystem #123; Galaga '91 intro) | no $06 support, then per-channel masks wrong | GG-STEREO; Galaga '91 and Dragon Crystal in the ledger |
| FM too quiet or too loud, wrong mix (mametesters 5849) | gains guessed | FM-MIX with a named oracle recording |
| FM audible where PSG should play, or silent (Gearsystem #122, #125; Genesis Plus GX #160) | $F2 state model wrong | FM-CTRL table per profile |
| Garbled FM after state load (Gearsystem #109, Genesis Plus GX #290) | FM state not fully serialised | state round-trip hash test over FM titles |
| Clicks, pops, crackle (Gearsystem #139, libretro forums) | host vsync or frame pacing, padded frames | exact per-frame cadence (section 6), never pad; host latency is the frontend's |
| Latency | large host buffers | one batch per frame, no internal buffering beyond the kernel's 16 taps (0.33 ms) |

## 9. Open questions and risks
| Open question | Settled by | How |
|---|---|---|
| Period 0 on a Mark III and SMS 1 (tone or DC) | psg phase | JoppyFurr ROM capture on owner's hardware if available; else keep PSG-0 |
| LFSR reset on the data byte after a noise latch | psg phase | same capture; ares and MAME both reset |
| SMS 2 top-three-level clipping | psg phase (profile flag off until then) | second measurement |
| GG port $06 in SMS mode, and NOSR's power-on value | gamegear phase | SMS-mode test on hardware; default $FF |
| Export SMS $F2 read value (%10) | fm phase | hardware read with the I/O chip disabled |
| FM/PSG ratio | fm phase | line-out capture of the Japanese BIOS theme |
| Which instrument table is closer to a real YM2413 | fm phase | A/B against a hardware YM2413 recording of the JoppyFurr ROM |
| Mark III $F2 = 2 | fm phase | hardware |
| PSG READY stall on the integrated chip | psg phase | timing test (SMS-SYSTEM owns bus timing) |

Risks: the volume law and the FM ratio churn PCM digests (the transition hash does not move); the ymfm port is the largest single piece of audio code; emu2413's floating-point table generation makes it a seed only after baking; no SMS Power forum thread could be fetched today (503), so AUD.27 is cited from search snippets.

## Sources
| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| AUD.01 | T2 | SMS Power! Development/SN76489 (Maxim; data from John Kortink, Charles MacDonald, Daniel Bienvenu) | accessed 2026-10-18 | clocks, protocol, period 0/1, volume table, LFSR widths and taps, reset, DC behaviour, SMS 2 clipping, GG port $06 |
| AUD.02 | T2 | SMS Power! Development/AudioControlPort | accessed 2026-10-18 | $F2 bits, value table, Mark III vs Japanese SMS vs export, counter bits |
| AUD.03 | T2 | SMS Power! Development/FMChipDetection (Phantasy Star routine) | accessed 2026-10-18 | detection protocol |
| AUD.04 | T2 | SMS Power! Development/YM2413 (andete's notes index) | accessed 2026-10-18 | rhythm slots, no YM2413 die, FHB013 |
| AUD.05 | T1 | Yamaha YM2413 Application Manual, HTML by Maxim (smspower.org/maxim/Documents/YM2413ApplicationManual) | accessed 2026-10-18 | fsam = fM/72, instruments, register map, 12/84 clock waits, 9-bit DAC |
| AUD.06 | T1 | Sega Game Gear official documentation, SMS Power! Development/GGOfficialDocs | accessed 2026-10-18 | port $06 bits and defaults, headphone/speaker, 3.579545 MHz, immediate latch |
| AUD.07 | T2 | Charles MacDonald, SMS/GG hardware notes 2002-11-12 (smspower.org uploads smstech-20021112.txt) | accessed 2026-10-18 | port maps, $40-$7F PSG, $F0-$F2, GG mode vs MS mode |
| AUD.08 | T3 | MAME `src/devices/sound/sn76496.cpp` (BSD-3) | 1d7a454ed79e | variant table, period 0 rule, taps, negate, stereo mask, volume law, READY |
| AUD.09 | T3 | MAME `src/mame/sega/sms.cpp`, `sms_m.cpp`, `sms.h` (BSD-3) | 1d7a454ed79e | clocks, $F0-$F2 handlers, mute table, GG I/O views |
| AUD.10 | T3 | MAME `src/devices/video/315_5124.cpp` (BSD-3) | 1d7a454ed79e | PSG inside the VDP at VDP clock/3 |
| AUD.11 | T3 | ares `ares/ms/cpu/memory.cpp`, `ares/ms/psg/psg.cpp`, `ares/ms/opll/opll.cpp` (ISC) | 9408cb43d494 | port decode, $F2 model, colorburst/16 and /72, volume formula |
| AUD.12 | T3 | ares `ares/component/audio/sn76489/sn76489.cpp` (ISC) | 9408cb43d494 | LFSR expression, reset, rate table, period 0 handling |
| AUD.13 | T3 | ares `ares/component/audio/ym2413/{ym2413.hpp,constants.cpp}` (ISC, Talarubi) | 9408cb43d494 | instrument table values |
| AUD.14 | T3 | github.com/aaronsgiles/ymfm `src/ymfm_opl.cpp`, `ymfm_opl.h`, LICENSE (BSD-3) | 81aec25ccbb9 | OPLL classes, instrument table and its provenance comment |
| AUD.15 | T3 | github.com/digital-sound-antiques/emu2413 `emu2413.c` v1.6.4 (MIT) | a1d5120903b7 | table, provenance, float table generation |
| AUD.16 | T2 | VGM Specification 1.71 beta (vgmrips/vgmplay-legacy `VGMPlay/vgmspec171.txt`) | 8262c68570d6 | header fields, commands, time base |
| AUD.17 | T1 | github.com/JoppyFurr/SN76489-TestRom (MIT) | 7d2068dfc171 | test ROM scope and builds |
| AUD.18 | T1 | github.com/JoppyFurr/YM2413-TestRom (MIT) | 571018fe8bfb | FM test ROM |
| AUD.19 | T1 | github.com/sverx/PSGlib LICENSE (BSD-3, 2014) | 002d74462bff | PSGlib licence |
| AUD.20 | T1 | github.com/szTheory/nesturbator `src/synth.c` (MIT, owner's sibling) | 9f9386c661f6 | kernel, interpolation |
| AUD.21 | T1 | github.com/szTheory/muhtari `.planning/preparation/A7800-AUDIO.md` section 8 (method precedent only) | 952fce970117 | transition hash, cadence, clipping lesson, ymfm port plan |
| AUD.22 | T4 | gendev.spritesmind.net forum post 15270 (sigMate, Nemesis, Eke) | accessed 2026-10-18 | discrete vs integrated period 0 |
| AUD.23 | T4 | mametesters.org bug 05849 | accessed 2026-10-18 | FM/PSG balance, 0.25-0.35 estimate |
| AUD.24 | T4 | github.com/drhelius/Gearsystem issues 32, 100, 109, 122, 123, 125, 139 | accessed 2026-10-18 | complaints |
| AUD.25 | T4 | bitbucket.org/eke/genesis-plus-gx issues 160, 224, 225, 283, 304 | accessed 2026-10-18 | complaints |
| AUD.26 | T4 | github.com/libretro/Genesis-Plus-GX issue 290 | accessed 2026-10-18 | FM state-load fault |
| AUD.27 | T4 | SMS Power! forums 17215 (spectrogram test), 2338 (PSG emulation) | search snippets only; HTTP 503 on 2026-10-18 | spectrogram ROM existence |
| AUD.28 | T1 | TI SN76489 / SN76489A datasheet | (unverified: not located today; facts via AUD.01 and AUD.32) | 2 dB steps, 4 MHz max, /16 |
| AUD.29 | T4 | github.com/plgDavid/misc wiki "Copyright-free OPLL(x) ROM patches" | (unverified: not fetched; cited through AUD.14) | table origin |
| AUD.30 | T2 | SMS Power! Development/Documents index | accessed 2026-10-18 | document locations and dates |
| AUD.31 | T2 | segaretro.org FM Sound Unit | accessed 2026-10-18 | BIOS uses both chips, no game does |
| AUD.32 | T4 | tinytapeout.com SN76489 project page (rejunity) | accessed 2026-10-18 | 16 steps, 28 dB maximum, summed output |
