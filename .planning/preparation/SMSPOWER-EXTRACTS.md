---
title: "SMS Power! and Z80-community extracts for cogmaster (SMS + Game Gear)"
summary: "Paraphrased facts harvested from the SMS Power! wiki and forums, Charles MacDonald's documents, the SingleStepTests/z80 repository, the Z80 undocumented-behaviour community (Rak, hoglet67, redcode), toolchain repositories and die-shot archives, each with handle, date and evidence type; plus a test-ROM inventory with licences, fixture candidates and the WLA-DX pin."
read_when: "Before citing any community (T2/T4) claim about Z80, VDP, PSG, mapper or BIOS behaviour; before choosing a test ROM, fixture or assembler for CI."
updated: 2026-10-18
---

# SMS Power! extracts (Sega Master System and Game Gear)

Harvested 2026-10-18. Method: `gh api` for every GitHub repository (licence field, LICENSE file, tree, commits), live fetches where www.smspower.org answered, Wayback 2024 captures where it returned 503 (most wiki pages did, see [SP.043]), and search-engine snippets only where noted as (unverified). No ROM, BIOS or forum attachment was downloaded except the WLA-DX source tarball and the SingleStepTests generator. Handles appear as shown on the page; real names are used only where the author publishes under them (Charles MacDonald, Sean Young, Patrik Rak, Frank Cringle). Tier: wiki pages and MacDonald's documents are T2, forum posts T4, repositories T1 for what they themselves contain, FPGA and emulator code T3. Prose is paraphrase; quotes are limited to register bit strings and byte sequences.

**Evidence types:** HW = measurement on hardware (logic analyser, scope, sampled audio); test ROM = program run on hardware; die = decap photograph; doc = official Sega or Zilog/TI text transcribed; EMU = inference from an emulator's behaviour or source; recollection = assertion without a stated method; (derived) = this harvester's inference.

## 0. Quick reference: the facts an SMS/GG core most needs

| Topic | Fact | Evidence | ID |
|---|---|---|---|
| Clocks | mclk 10.738580 MHz measured; line 684 mclk = 228 Z80 clocks; NTSC frame 262 lines = 179 208 mclk; PAL 313 lines | HW | SP.044, SP.057 |
| Interrupt position | frame /INT falls 607 mclk into line 192, line /INT 608 mclk into its line, both 3–4 mclk before the /HSYNC edge; V counter steps at the same moment; Pause /NMI only at the start of line 261 (312 PAL) | HW | SP.045–SP.047 |
| Line counter | decrements on lines 0–192, reloads from reg $0A elsewhere; "line 0" IRQ is generated at the end of the last line | HW + EMU | SP.054–SP.056 |
| Status read | clears frame, overflow and collision flags and the control latch; a read coinciding with the flag set loses the IRQ; low 5 bits are unconnected garbage on SMS2 | test ROM | SP.058, SP.059, SP.061 |
| VRAM pacing | SMS2 tolerated one data write every 26 Z80 cycles in active display; Sega says 29; too-fast writes misdirect bytes; no limit with display off | test ROM | SP.063–SP.066 |
| INT bus byte | $FF on SMS2, GG and Genesis (RST 38h in IM 0, vector $FF in IM 2); random on SMS1 | HW | SP.035 |
| Z80 part | NEC D780C-1 or Zilog Z0840004PSC, both NMOS; SCF/CCF X/Y and OUT (C),0 therefore follow the NMOS rules; the one-chip GG is unmeasured | recollection + (derived) | SP.038–SP.040 |
| SCF/CCF | X/Y = ((Q ^ F) \| A) bits 5/3; Q = 0 after a non-flag instruction, after a DD/FD prefix and after an interrupt; NEC NMOS always copies A | HW | SP.012–SP.016 |
| PSG noise | 16-bit LFSR, taps 0 and 3 into bit 15, output bit 0, reset to $8000 on any noise write; period 0/1 = constant +1 | HW sampled | SP.077–SP.081 |
| Mapper reset | 315-5235 resets to 0,0,1,2; older chips undefined; writes mirror into RAM at $DFFC–$DFFF | recollection + HW | SP.093, SP.094 |
| BIOS hand-off | $3E = $AB (SMS) or $A8 (GG) and stored at $C000; VDP registers left as the BIOS used them; some games need SP pre-set | HW + EMU | SP.106–SP.109 |

## Where to look

| Question | Section |
|---|---|
| SingleStepTests JSON fields, p and q, CMOS switch, file count | 1.1 |
| SCF/CCF flags, Q, Zilog vs NEC vs ST, which Z80 Sega used | 1.2, 1.4 |
| MEMPTR, DAA, R, HALT, EI delay, NMI, interrupt costs | 1.3 |
| INT bus value, Pause/NMI wiring on SMS and GG | 1.4, 5 |
| H and V counters, interrupt positions, line-counter reload | 2.1 |
| Status flags, sprite overflow and collision, zoom bug, VRAM pacing | 2.2 |
| Game Gear window, CRAM latch, Genesis-mode differences | 2.3 |
| PSG LFSR, volume, period 0, stereo, init pops | 3 |
| Sega, Codemasters, Korean and multicart mappers, power-on paging | 4 |
| BIOS behaviour per region, residual state, ports $3E/$3F | 5 |
| Test ROM inventory and licences, Z80 vector sets | 6, 6.1 |
| Homebrew fixture candidates | 6.2 |
| Assembler choice, WLA-DX pin and digest, devkitSMS crt0 exception | 7 |
| Scope captures, die shots, FPGA cores, who to cite | 8 |

## 1. Z80 core facts (consumed by SYS)

### 1.1 SingleStepTests/z80 JSON (read here at `ebe1875d48f3`, MIT)

| ID | Fact | Source | Handle | Date | Evidence |
|---|---|---|---|---|---|
| SP.001 | Each file is one opcode sequence with 1000 cases: `name`, `initial`, `final`, `cycles`, and `ports` for I/O opcodes. Register keys: pc sp a b c d e f h l i r ei wz ix iy af_ bc_ de_ hl_ im p q iff1 iff2 ram | README | raddad772 (2024-05-10 initial), README edits howprice 2025-11-28 | 2026-04-10 commit | repo README (tested: read here) |
| SP.002 | `wz` is MEMPTR. `q` is the Q latch: after an instruction that affects flags it holds the new F byte; after one that cannot (EX AF,AF', POP AF, NOP-like internals, interrupt acceptance) it is 0. The generator seeds `q` as 0 or a copy of `f` (`z80_test_generator.js` ~line 588) | README; generator; issue #11 (samplx 2025-11-01, redcode 2025-11-03 and 2026-03-18) | redcode | 2025-11 | repo source (tested) |
| SP.003 | `p` is a one-bit latch set when `LD A,I` or `LD A,R` was the last instruction, used for the NMOS bug where an interrupt accepted right after them clears P/V; it is set only when the generator is in NMOS mode (`P = !CMOS`, line 1702) and zeroed at the start of every test (line 2395). Since interrupt cases were removed (issue #5, ProdOrDev 2025-08-15) it never changes a shipped final state | generator comments lines 58–59 | raddad772, redcode | 2026-04-10 | repo source (tested) |
| SP.004 | `ei` records that EI was the last instruction (one-instruction acceptance delay); README says it can be ignored by most consumers | generator comment | raddad772 | | repo source |
| SP.005 | `cycles` entries are `[address, data or null, "rwmi"]` sampled between T-states; null means the bus floats (default substitutes the last address). Default generator options: CMOS off; simplified strobes (MREQ/RD and MREQ/WR pulse for one T-state, not two); I/R on the address bus during refresh; nulls during wait states off | README | | | repo README (tested) |
| SP.006 | SCF/CCF model (lines 1298–1309, 2144–2153): if Q is non-zero and the prefix is not DD/FD, X and Y are first cleared, then X/Y = bits 3/5 of (F OR A); so a flag-setting predecessor yields A's bits and any other predecessor yields F OR A. Q is set after both | generator source | raddad772, redcode | 2026-04-10 | repo source (tested) |
| SP.007 | `OUT (C),0` writes $00 in the NMOS model and $FF when CMOS is selected (line 1844); the shipped v1 set is NMOS | generator source | | | repo source (tested) |
| SP.008 | 1604 files in `v1/` (256 base, 256 CB, 80 ED, 508 DD including DDCB, 508 FD), 1 604 000 cases; no separate ED set | git tree count (tested) | | 2026-04-10 | repo |
| SP.009 | Known limits: cycle order for `LD (IX+d),n`, `EX (SP),HL/IX/IY`, RLD/RRD was wrong until mid-2025 fixes; the validation workflow checks final state, not `cycles`; redcode said some pin inaccuracies remained | issue #3 (itsmevjnk 2024-06-26; redcode 2024-06-26, 2025-08-31) | redcode | 2025-08-31 | emulator inference vs floooh netlist timing |
| SP.010 | A DD or FD prefix zeroes Q (discovered by Tony Brewer); the whole set was regenerated for it | commit message, 2026-04-10 | redcode, TonyB | 2026-04-10 | HW relayed by commit |
| SP.011 | The tests derive from a translation of ares' Z80 core with later fixes; redcode/Z80 (LGPL-3) is the CI reference | README | | | repo README |

### 1.2 SCF/CCF, X/Y flags and chip variants

| ID | Fact | Source | Handle | Date | Evidence |
|---|---|---|---|---|---|
| SP.012 | Original rule: after SCF/CCF, F bits 5 and 3 = ((Q XOR F) OR A) bits 5/3, where Q holds the F assembled by the previous instruction and is 0 after any instruction that did not modify flags (EX AF,AF' and POP AF count as non-modifying); an interrupt acceptance also clears Q; Q does not depend on I or R | worldofspectrum.org/forums/discussion/41704 | Patrik Rak; "Winston" reported a Mostek part differing | 2012-11 (edited 2014-04) | test ROM on a Zilog 48K Spectrum |
| SP.013 | Instability: NEC parts were first thought non-deterministic; later found that any Z80 can give random bits 5/3 after SCF/CCF depending on the board; Zilog parts almost always stable in real machines | raxoft/z80test readme.txt | raxoft | v1.1 2022-01-16 | test ROM result |
| SP.014 | Variant table: Zilog NMOS and second sources: Q true → YF=A.5, XF=A.3; Q false → YF\|=A.5, XF\|=A.3. NEC NMOS: always YF=A.5, XF=A.3. ST CMOS: XF=A.3 always, YF follows the Zilog rule. POP AF does not count as flag-modifying. DD/FD before SCF/CCF hides Q because the prefix is treated as the previous (non-flag) instruction | github.com/hoglet67/Z80Decoder/wiki/Undocumented-Flags | hoglet67; addition credited TonyB 2026-07-09 | page edit 2026-07-09 | logic-analyser decoder on Acorn Z80 second processor |
| SP.015 | Per-chip stability plots (256×256 F×A, 16 cases, 2.0/3.5/6.0 MHz): Zilog NMOS 8039, Zilog CMOS, SGS NMOS stable with the Q-dependent OR; Zilog NMOS 8811 stable "always copy A"; Zilog NMOS 8644 unstable (YF = (YF AND random) OR A.5 when Q false); NEC NMOS unstable (YF = (YF OR random) AND A.5, tending to copy-A at low clock); ST CMOS unstable on XF only; NEC CMOS unanalysed | …/wiki/Unstable-CCF-SCF-Behaviour | hoglet67 | 2024-05-26 | HW |
| SP.016 | Counter-view: the (Q^F)\|A rule held on every Zilog NMOS and CMOS part tested by ICEknight, Ricardo Martínez Cantero and Marta Sevillano Mancilla; the ST CMOS result is doubted because later ST parts behaved like Zilog, so it may be instability | github.com/redcode/Z80_XCF_Flavor README (GPL-3; docs only) | redcode | pushed 2024-11-20 | test ROM results relayed |
| SP.017 | MAME's model: Zilog NMOS; LD A,I/R P/V glitch fixed on CMOS; OUT (C),0 → 0 NMOS, $FF CMOS; SCF/CCF X/Y = (F\|A)&$28 on SGS/Sharp/Zilog NMOS vs F&A&$28 on NEC NMOS, with a note that newer findings make it depend on the previous instruction | mame `src/devices/cpu/z80/z80.cpp` header | Juergen Buchmueller et al. | master 2026-10-18 | emulator inference |
| SP.018 | Interrupted LDIR/LDDR leave YF/XF from PC bits 13/11 of the ED prefix address; this fixed all decoder mispredictions. Interrupted INxR/OTxR also adjust PF and HF from CF, the data byte and B | stardot "Z80 Protocol Decoder" p.3 (viewtopic.php?p=211042, Wayback) and hoglet wiki | hoglet, TonyB, Coeus | 2018-08-11 → 13 | HW |
| SP.019 | TonyB holds that "The Undocumented Z80 Documented" is out of date and that most of its HALT description is wrong (refers to the Special Reset page at primrosebank.net) | same stardot thread | TonyB | 2018-08 | recollection |
| SP.020 | ZEXALL-SMS v0.20 added a SCF/CCF pre-test reporting per-flag set counts over $1FFF runs; expected all 0 or $1FFF; the pattern varies by CPU type and four Sega patterns are known | zexall-sms source and SMS Power page | Maxim | 2025-05-16 | test ROM on hardware |

### 1.3 MEMPTR, DAA, R, HALT, interrupts (generic Z80)

| ID | Fact | Source | Handle | Date | Evidence |
|---|---|---|---|---|---|
| SP.021 | BIT n,(IX+d) takes YF/XF from bits 5/3 of the high byte of IX+d; BIT n,(HL) takes them from an internal 16-bit address register (the MEMPTR high byte) left by the previous 16-bit operation | The Undocumented Z80 Documented v0.6 §4.1 (sam.speccy.cz mirror, GNU FDL 1.1) | Sean Young | 2003-11-20 (v0.6 is the newest copy fetched; a later v0.91 is believed to exist, unverified) | test ROM on MSX |
| SP.022 | MEMPTR after LDIR/LDDR is unchanged when BC=1, else PC+1 of the instruction; some clones differ in MEMPTR and can be told apart by it. Provenance: theory by boo_boo and Vladimir Kladov, chips tested by Wlodek, CHRV, icebear, molodcov_alex, goodboy, zx.pk.ru 2006 | MEMPTR doc (gist drhelius/8497817) | as listed | 2006 | test ROM |
| SP.023 | Acceptance cost: NMI 11 T (5+3+3); IM 1 13 (7+3+3); IM 2 19 (7+3+3+3+3); IM 0 adds 2 wait T per INTA M1; a second NMI is not accepted during the NMI response; the "IM 2 vector bit 0 must be 0" rule does not exist | redcode/Z80 wiki Interrupts; Sean Young v0.6 ch.5 | redcode; Sean Young | 2026-02-26; 2003 | emulator doc + test ROM |
| SP.024 | No interrupt is accepted directly after EI or DI; the next instruction always runs; a run of EIs (and a run of DD/FD prefixes) blocks acceptance for the run; never mid-instruction; block instructions can be interrupted between iterations | Sean Young v0.6 §5.5 | Sean Young | 2003 | test ROM |
| SP.025 | RETI/RETN do not accept a maskable interrupt immediately after them when IFF1 != IFF2 beforehand (only after an NMI) | redcode/Z80 wiki (credit Andre Weissflog) | redcode | 2026-02 | emulator doc |
| SP.026 | IFF table: reset, DI and INT acceptance clear both; EI sets both; NMI acceptance clears IFF1 only and does not first copy IFF1 to IFF2 (nested-NMI test); all ED RET variants copy IFF2 to IFF1; LD A,I / LD A,R copy IFF2 to P/V | Sean Young v0.6 §5.3 | Sean Young | 2003 | test ROM |
| SP.027 | NMOS parts: an interrupt accepted right after LD A,I or LD A,R leaves P/V = 0 even if IFF2 was 1; fixed on CMOS | MAME header; redcode/Z80 README | | | emulator inference |
| SP.028 | HALT re-executes itself (PC not advanced) until INT or NMI is accepted, then PC advances before the handler; the HALT pin is asserted. R keeps incrementing per M1 during HALT (derived from SP.030; Young does not say so explicitly) | Sean Young v0.6 §5.4 | Sean Young | 2003 | test ROM + derived |
| SP.029 | Z80 without a responding device reads $FF on the bus | Sean Young v0.6 §5.2 | | 2003 | recollection |
| SP.030 | R: low 7 bits +1 per M1; bit 7 only via LD R,A; unprefixed +1; CB/ED/DD/FD and DDCB/FDCB +2; stray DD/FD +1; LD A,R sees R after its own increment; LDIR +2 per iteration; INT and NMI acceptance +1; R=0 at reset | Sean Young v0.6 §6.1 | Sean Young | 2003 | test ROM |
| SP.031 | NMI arriving immediately after EI: netlist simulation (Visual Z80, ChipSim, Pavel's simulator) shows a node conflict leaving IFF2 = 0; the author judges it a simulator-fidelity artefact rather than silicon | hoglet67/Z80Decoder/wiki/NMI-during-EI-Anomaly | hoglet67 | notes 2018-10-20, page 2022-02-25 | transistor-level simulation |
| SP.032 | DAA: correction 00/06/60/66 picked from CF, high nibble, HF, low nibble; added when NF=0, subtracted when NF=1; CF' and HF' from the published tables; S/Y/X copy result bits 7/5/3; Z from result; N unchanged. Tables by Stefano Donati (Ramsoft), added v0.5, corrected v0.6 | Sean Young v0.6 §4.7 | Sean Young, Stefano Donati | 2003-11 | test ROM |
| SP.033 | `OUT (C),0`: MSX1 with NMOS wrote 0 (screen black), CMOS MSX1 and Turbo R in Z80 mode wrote 255 (white), R800 treats it as NOP; openMSX implements this | msx.org forum "Turbo-R and out(c),0" | gdx, Metalion, Grauw | 2022-05-07/08 | test ROM on hardware |

### 1.4 What SMS, Game Gear and Mega Drive hardware show

| ID | Fact | Source | Handle | Date | Evidence |
|---|---|---|---|---|---|
| SP.035 | INT acknowledge data bus: SMS2, Game Gear and Genesis always read $FF, so IM 0 executes RST 38h and IM 2 vectors through I*256+$FF; on SMS1 the byte is random, so IM 0 is unusable and IM 2 needs a 257-byte table of one value. IM 1 goes to $0038 | smstech-20021112.txt | Charles MacDonald | 2002-11-12 | HW/test ROM |
| SP.036 | Pause drives the Z80 /NMI pin: press jumps to $0066, release does nothing; GG has Pause via Start, Genesis without the converter has none. /NMI is negative-edge triggered (holding it low does not repeat); /INT is level-sensitive and re-triggers after each instruction while low | smstech; SMS Power `Development/InterruptMechanism`, `Development/PauseButton`, `JoypadPauseButton` (315-5216 NMI-IN line pulled low by an open-collector OR) | Charles MacDonald; wiki (no credits) | 2002; accessed 2026-10-18 | HW + wiki |
| SP.037 | Pull-ups make the IM 0 byte $FF (RST 38h); with pull-downs it would be $00 (NOP); IM 2 vector bit 0 need not be 0 | `Development/InterruptMechanism` | wiki | accessed 2026-10-18 | wiki |
| SP.038 | CPU parts: boards photographed carry an NEC D780C-1 (NTSC SMS1, Mark III) or a Zilog Z0840004PSC (SMS II, later boards); Korean Mark III boards a Goldstar Z8400A PS. Both NMOS. The copetti article says Sega second-sourced the Z80 across revisions | copetti.org Master System (2020-10-12, modified 2026-08-08); consolemods 171-5379 and 171-5304 (snippets only, Anubis-blocked); spritesmind teardown mirror (snippet) | Rodrigo Copetti; nepereny | 2020–2026 | recollection/photo; (unverified beyond snippets for consolemods) |
| SP.039 | Game Gear: VA0 uses discrete Z80A (plus 315-5377 and 315-5378); VA1 and later merge CPU, VDP and I/O into one custom LSI (service manual snippet; elotrolado names 315-5535). Which part number is the CPU+VDP is (unverified) | GG VA1 service manual (consolemods PDF snippet); Wikipedia Game Gear (2026-10-01) | | | recollection |
| SP.040 | No SMS Power source states whether real SMS/GG write 0 or $FF for OUT (C),0, nor any SMS thread on MEMPTR, NEC vs Zilog or SCF/CCF (site search 2026-10-18). Inference: NMOS NEC/Zilog parts in SMS1/SMS2 write 0; one-chip GG and Mega Drive 2 (CMOS parts per TmEE) may write $FF. Spritesmind: MD2 always uses CMOS parts; Sik: OUT (C),0's CMOS behaviour is a no-op in MD mode and impractical to test; Mask of Destiny: MD1 VA3+ return $FF from Z80 open bus, earlier boards return the last value read | gendev.spritesmind.net viewtopic.php?p=36700 | TmEE co.(TM), Sik, Mask of Destiny | 2020-08-15 → 22 | recollection/HW; inference marked (derived) |
| SP.041 | SMS Power forum consensus on Z80 references: no perfect source; The Undocumented Z80 Documented for flags; the Mostek manual for per-M-cycle bus timing; the SMS ZEXALL port masks undocumented flags by default because no SMS game is known to depend on them; a correct emulator should reproduce real-system CRCs, not the program's embedded ones, because test-order state leaks | forums/16466 (DarkMoe, efry, Maxim, ValleyBell, Charles MacDonald, 2016-12-20/21); forums/11361 (benryves, Martin, Maxim, 2008-09-07/08) (both via Wayback) | as listed | 2008, 2016 | recollection |
| SP.042 | Homebrew uses IXH/IXL instructions; no commercial-game dependence on undocumented opcodes was established in the threads found (threads 2269, 13638 returned 503 and were not read) | SMS Power search | | 2026-10-18 | (unverified) |

## 2. VDP timing facts (consumed by VDP)

Access note: www.smspower.org answered HTTP 503 for most wiki pages on 2026-10-18; `Development/VDPRegisters` (partial) and `msvdp-20021112.txt` were fetched live, everything else from 2024 Wayback captures. Archived wiki pages show no last-edit date, written here as "wiki, 2024 capture". Page names that do not exist: `HCounter`, `VCounter`, `Interrupts`, `StatusRegister`, `VideoModes`, `SegaMapper`, `CodemastersMapper`, `KoreanMappers`; the real pages are `ScanlineCounter`, `ControlPort`, `Sprites`, `CollisionDetection`, `Mappers`, `Port3E`, `BIOSes`, `GGVDP`, `GGLCD`, `Palette`, `SMS1SMS2VDPs`, `GGOfficialDocs`, `SN76489` [SP.043].

### 2.1 Clocks, counters and interrupt positions

| ID | Fact | Source | Handle | Date | Evidence |
|---|---|---|---|---|---|
| SP.044 | Master clock 10.738580 MHz measured; line = 684 mclk (228 Z80 clocks); frame = 179 208 mclk = 262 lines, about 59.92 Hz; active 512 mclk, border 172; Z80 = mclk/3 with 1-high/2-low duty; pixel = mclk/2; /HSYNC starts 558 mclk into the line, 53 wide | forums/8161 "SMS Display Timing", first post | Charles MacDonald | 2006-02-19 | logic analyser, NTSC SMS1 |
| SP.045 | Frame /INT falls 607 mclk into line 192 (4 mclk before the /HSYNC edge that starts the next line); line /INT falls 608 mclk into its line | same | Charles MacDonald | 2006-02-19 | logic analyser |
| SP.046 | The earliest I/O read that sees a new V counter value is 12 mclk (4 Z80 clocks) before the /HSYNC rising edge, so the V counter steps at essentially the moment the line interrupt asserts | same, 2006-02-27 post | Charles MacDonald | 2006-02-27 | logic analyser + polling loop |
| SP.047 | /NMI (Pause) is asserted by the VDP only at the start of line 261 NTSC (the /HSYNC edge 611 mclk into line 260); it stays asserted while held and releases at the same point of a later frame; the button bounces more than 50 times, explaining the debounce. PAL: start of line 312 | same, "Fun with the pause button"; FluBBa post | Charles MacDonald, FluBBa | 2006-02-19/20 | logic analyser |
| SP.048 | Lowest H counter read in the handlers with the CPU halted: $06 frame IRQ, $07 line IRQ, $06 NMI; back-calculated VBlank IRQ near HC $F6; reconciled with the official GG doc's $F4 and the GG window starting at HC $20 | same, 2008-03-01 and 2008-05-15 posts | FluBBa, Christophe | 2008 | test ROM |
| SP.049 | Official GG VDP manual: H counter runs $00–$93, jumps to $E9, counts to $FF; one count = 2 dots (342 dots = 171 counts); LCD window = H $20–$6F, V $18–$A7; line interrupt, V increment and horizontal-scroll latch all happen at H $F4, so a scroll written in a line IRQ takes effect one line later | `Development/GGOfficialDocs` | Sega (transcribed) | wiki, 2024 capture | official doc |
| SP.050 | Older H counter table without the jump (9-bit counter, upper 8 bits returned): $00–$7F active, $80–$87 right border, $87–$8B right blank, $8B–$ED hsync, $ED–$EE left blank, $EE–$F5 burst, $F5–$F9 left blank, $F9–$FF left border; measured on an NTSC **Genesis** with the warning that a real SMS may differ; conflicts with SP.049 | msvdp-20021112.txt §11 | Charles MacDonald | 2002-11-12 | HW (Genesis) |
| SP.051 | The H counter is latched when a controller TH pin changes; port $7F returns the frozen value until the next change; the Light Phaser page says the latch happens on TH going to 0 and the return to 1 does not re-latch | msvdp §11; `Development/LightPhaser` | Charles MacDonald; wiki | 2002; 2024 capture | HW/recollection |
| SP.052 | V counter tables: NTSC 192 $00–$DA,$D5–$FF; NTSC 224 $00–$EA,$E5–$FF; NTSC 240 $00–$FF,$00–$06 (no valid picture); PAL 192 $00–$F2,$BA–$FF; PAL 224 $00–$FF,$00–$02,$CA–$FF; PAL 240 $00–$FF,$00–$0A,$D2–$FF. NTSC 192 budget 192+24+3+3+13+27; NTSC 224: 224/8/3/3/13/11. Checked on an NTSC SMS2 with a PAL/NTSC switch | `Development/ScanlineCounter`; msvdp §11 | Charles MacDonald | 2002-11-12 | HW |
| SP.053 | Frame flag set on V line $C1 (192), $E1 (224), $F1 (240); stays until a control-port read; reg 1 bit 5 gates the IRQ line only | msvdp §12 | Charles MacDonald | 2002 | HW/EMU |
| SP.054 | Line counter: decrements on lines 0–191 and 192, reloads from reg $0A on 193–261; underflow $00→$FF reloads and sets a pending flag cleared only by a status read; a write to $0A does not alter the live counter until the next reload; reg 0 bit 4 gates the line | msvdp §12 | Charles MacDonald | 2002 | HW/EMU |
| SP.055 | Sega's statement of the same rule: the counter counts only on effective-area lines plus the line immediately before them ("line 511"); elsewhere it keeps reloading | GGOfficialDocs reg #10 | Sega | 2024 capture | official doc |
| SP.056 | Because the line IRQ fires 3 mclk before the end of a line, the interrupt games see as "line 0" is generated at the end of line 261 (312 PAL); emulating decrement-at-end-of-line from line 261 fixed off-by-one scroll bugs with no regressions; FluBBa: line IRQs at the start of lines 0–192, VBlank at the start of 193 | forums/11313 | guest (PoorAussie era), FluBBa | 2008-08-18 | emulator inference vs SP.045 |
| SP.057 | PAL has 313 lines (last line 312) | forums/8161 | FluBBa | 2006-02-20 | test ROM |

### 2.2 Status flags, sprites, VRAM access

| ID | Fact | Source | Handle | Date | Evidence |
|---|---|---|---|---|---|
| SP.058 | Status: bit 7 frame, bit 6 overflow, bit 5 collision; bits 4–0 undriven garbage on SMS/SMS2 ("fifth sprite" only in TMS modes); a status read clears all three flags and the control-port byte latch; data-port reads and writes also clear the latch | msvdp §4; `Development/ControlPort` | Charles MacDonald; wiki | 2002; 2024 capture | HW/EMU |
| SP.059 | On SMS2 the low 5 status bits were zero for any sprite count and random bus garbage in a sprite-less dump, so they are unconnected; overflow sets whenever more than 8 sprites share a line even with transparent patterns; overflow cannot be observed while bit 7 is already set (delayed read to line $80 after overflowing on line 10); a line IRQ alone does not set bit 6; sprites in the left 8 columns still collide when column 0 is blanked by reg 0 bit 5 | forums/1568 "Sprite overflow test program" | Charles MacDonald (tests run by Maxim and Zoop on SMS2, Charles on Genesis) | 2001-08-31 | test ROM |
| SP.060 | Overflow should be set when the SAT is scanned in H-blank for the next line; collision when overlapping opaque pixels are drawn during active render; setting both at HSYNC gave a flickering line under Fantastic Dizzy's status bar | forums/11320 | PoorAussie, Maxim | 2008-08-21 | emulator inference |
| SP.061 | A status read executing exactly when the VDP sets a flag clears it and drops /INT before the Z80 samples it, so the interrupt is lost; bit 7 exists to tell frame from line IRQs, not to signal VBlank | forums/15105 | FluBBa, Calindro, sverx | 2014-11-24 | emulator inference/recollection |
| SP.062 | Collision bit is cleared on read; line-accurate setting suffices for known software; users: Cheese Cat'astrophe, Ecco (SMS/GG), Fantastic Dizzy (SMS/GG), Fantasy Zone Gear, Impossible Mission, Taz-Mania (SMS/GG), Master of Darkness | `Development/CollisionDetection` | wiki | 2024 capture | recollection |
| SP.063 | Sega's limits are 16 T-states between VRAM accesses in VBlank and 29 in active video; an SMS2 accepted one write every 26 CPU cycles without corruption; keep 28–29 for safety; with display off, OUTI pace is fine | forums/10523 | Maxim | 2007-10-30 | test ROM, SMS2 |
| SP.064 | Game write cadences: OUT (C),r 12 cycles, OUTI 16 (Shadow of the Beast level start), 21.33-cycle loops, Phantasy Star about 25.66 untimed. Inferred model: VDP delay about 13 mclk (not the TMS9918's 21), one CPU slot per 64 mclk in active display, address increments at the delay, a single 8-bit buffer written at the next slot, and a second write before the slot overwrites the buffer so the first address receives the second byte | forums/11689 "VRAM Timing" | PoorAussie | 2009-03-16/19 | emulator inference from about 20 games |
| SP.065 | Untimed OTIR loading gives garbled tiles with wrong colours on hardware; Emukon and RetroCopy reproduce it; no delay needed when the display is disabled (backdrop shows) | forums/14106 | MrD, Bock, Maxim | 2013-02-19 | test/recollection |
| SP.066 | Only a limited number of CPU accesses per line are serviced; writes during a busy VDP are lost or misdirected; seen on SMS2, not on Genesis or Game Gear (stated as theory) | msvdp | Charles MacDonald | 2002 | HW observation |
| SP.067 | SMS1 zoom bug: only the first four of the eight sprites on a line are zoomed horizontally, all eight vertically (theory: the TMS9918's four zoom flags were never extended); confirmed by Earthworm Jim (Tectoy) status bar breaking on SMS1 only. `Development/Sprites` says the **last** 4 found per line lack horizontal zoom; `Development/VDPRegisters` says "the first N minus 4 are double width": the three wordings disagree, treat as a test target. Genesis ignores reg 1 bit 0; GG behaves as SMS2 | forums/1619 (Charles MacDonald, test by Bock on SMS1, 2001-10-22); forums/1643 (Bock 2001-11-20); wiki pages; msvdp §13; forums/11374 (FluBBa 2008-10-13) | as listed | 2001–2008 | test ROM + wiki |
| SP.068 | Y=$D0 ends SAT processing only in 192-line mode; sprites draw at Y+1; GG mode does not honour the terminator (msvdp). PoorAussie argues the +1 is the VDP evaluating line N+1 during line N from line 261 onward | `Development/Sprites`; msvdp; forums/11374 | wiki; PoorAussie | 2024 capture; 2008-09-12 | recollection/EMU |
| SP.069 | 315-5124 ANDs legacy register bits into VRAM addresses (reg 2 bit 0 masks a name-table row bit, reg 5 bit 0 masks the SAT, reg 6 bits 1–0 mask sprite patterns, reg 3/4 mask tile bitplanes); 315-5246 ignores them; only SMS2 and GG do 224/240; a few units were swapped at the factory | `Development/SMS1SMS2VDPs`, `VDPRegisters` | wiki (Charles MacDonald credited) | 2024 capture / live | recollection/test |
| SP.070 | Mode bits: M4 = reg 0 bit 2, M2 = reg 0 bit 1, M1 = reg 1 bit 4 (224), M3 = reg 1 bit 3 (240); 224/240 need M4 plus the height bit; no full truth table on the page (msvdp §6: only four TMS combinations are documented, the rest are variations); in 224/240 the name table is reg 2 bits 3–2 × $1000 + $700 | `VDPRegisters` (live); msvdp §6 | wiki | 2026-10-18 | doc |
| SP.071 | Mode-change timing: no SMS Power measurement found of when a mid-frame mode or height change takes effect (unverified gap) | search 2026-10-18 | | | |

### 2.3 Game Gear and Genesis-mode (precedent only)

| ID | Fact | Source | Handle | Date | Evidence |
|---|---|---|---|---|---|
| SP.072 | GG mode shows the central 160×144 of the 256×192 field (offset 48, 24; lines 24–167). In SMS mode the LCD (160×146) drops 8 columns each side and maps 3 source pixels to 2 with sub-pixel mapping; 192 lines are padded to 218 then scaled to 146 (letterbox); 224-line drops 3 top and 3 bottom; 240-line freezes the display until power-cycle. Off-screen sprites still count toward 8 per line | `Development/GGVDP`, `GGLCD`, `GGOfficialDocs` | wiki; Sega | 2024 capture | doc/recollection |
| SP.073 | GG CRAM: 32 × 12-bit (----BBBBGGGGRRRR), 64 bytes, address wraps past $3F; an even-address write only loads a latch, a later odd-address write commits latch + new byte as one word, and the same latched low byte can be committed to several entries; in SMS mode on a GG writes take effect immediately. Mark III/SMS1 render the first blue level slightly brighter than SMS2 | msvdp §5; `Development/Palette` | Charles MacDonald; wiki | 2002; 2024 capture | HW/recollection |
| SP.074 | Genesis Mode 4: reg 1 bit 2 selects Mode 5; TMS modes give black; the first command byte is latched so the address LSB updates only on the second; data writes do not fill the read buffer; CRAM is chosen by code bit 1 alone; no 224/240; no zoom. Breakages: F-15 Fighting Falcon, Bart vs. Space Mutants, Ax Battler (GG), Cosmic Spacehead, Micro Machines, Ys (JP). Overflow bit 6 works. H counter latch behaviour on Genesis not addressed (unverified) | msvdp §13; forums/12656; forums/1568 | Charles MacDonald | 2002; 2010-10-25 | HW/recollection |

## 3. PSG facts (consumed by AUD)

Primary source `Development/SN76489` (credits: Maxim; initial results John Kortink; noise samples by Charles MacDonald on SMS1, GG, Genesis, SC-3000H; Daniel Bienvenu ColecoVision; finaldave parity routine); history table ends 2005-11-12; evidence is sampled audio output with brute-forced taps unless noted [SP.076].

| ID | Fact | Handle | Date | Evidence |
|---|---|---|---|---|
| SP.077 | SMS1/SMS2/GG/Genesis white noise: 16-bit register, taps bits 0 and 3 ($0009) XORed into bit 15, output bit 0; discrete SN76489 (SG-1000, SC-3000H, BBC, ColecoVision): 15-bit, taps 0 and 1 ($0003) into bit 14; Tandy 1000: taps 0 and 4. Charles MacDonald confirmed SMS1 (2003-04-21) and GG/Genesis (2002-08-22) match SMS2. The Sega 16-bit sequence is about 53 000 long, not maximal | Maxim, Charles MacDonald | 2002–2005 | HW sampled |
| SP.078 | Periodic noise feeds back bit 0 only, so the register circulates: a 1/16 (1/15 discrete) duty pulse, not true periodic noise. Compact step: `sr = (sr>>1) | (((white ? sr^(sr>>3) : sr) & 1) << 15)` | Maxim | 2005-06-02 (forums/6838) | HW sampled |
| SP.079 | Any write to the noise register resets the shift register to $8000 | Maxim | 2005 | HW |
| SP.080 | Noise clock: low bits 00/01/10 → divider 16/32/64; 11 → tone 2 output clocks the register on 0→1; periodic-via-tone-2 spans about 6.8 Hz to 6991 Hz at 3 579 545 Hz | Maxim | 2005 | HW |
| SP.081 | Tone period 0 or 1 gives a constant +1 output (DC at the channel volume), used for PCM; highest audible on an SMS2 test was period 6 (about 18 643 Hz); f = clk/(32n) | Maxim | 2005 | test |
| SP.082 | Volume: 2 dB per step, ratio 0.79432823; 32767, 26028, 20675, 16422, 13045, 10362, 8231, 6568, 5193, 4125, 3277, 2603, 2067, 1642, 1304, 0 (page values; entry 7 should be 6538 by the stated ratio, the page's 6568 is a slip, see SMS-AUDIO 2.3) | Maxim | 2005 | measurement + maths |
| SP.083 | Maxim's SMS plus TV-card capture showed the top three levels clipping; the page says SMS1 and Mega Drive show proper 2 dB spacing while the SMS2 clips through a hardware design mistake; the linked discussion thread could not be located (unverified detail) | Maxim | 2005 | HW capture |
| SP.084 | Tone channels output 0/1; AC coupling makes it −0.5/+0.5, so noise should be mixed at double the amplitude older cores used | Maxim (after Daniel Bienvenu) | 2005-05-31 (forums/6838) | HW sampled |
| SP.085 | GG stereo port $06: bits 0–3 enable channels 0–3 right, bits 4–7 left, 1 = enabled, headphone output only; smstech.txt words the same bits as "1 = disabled", so polarity conflicts between the two documents: test target. Users: GG Aleste (FluBBa 2005-07-24), Galaga (Bock, unverified) | Maxim; Charles MacDonald; FluBBa | 2002–2005 | HW/recollection |
| SP.086 | Latch byte %1cctdddd always sets the latched channel and type; data bytes update the latched register's high 6 bits (tone) or low bits and never clear the latch; tones update per byte. Alex Kidd message tone and Micro Machines noise depend on this. Sega-integrated PSGs power up tone/noise 0 and volume $F; discrete chips start random | Maxim | 2005 | HW/recollection |
| SP.087 | Tone counter per TI datasheet: 10-bit counter decrements at clk/16, toggles the flip-flop and reloads at zero; no measured write-to-effect latency exists (Maxim: nobody has scoped it) | Blargg, Maxim | 2008-09-14 (forums/11371) | doc |
| SP.088 | About 37 Sega titles (After Burner, Thunder Blade, Rampage, Kenseiden, Ys, Psycho Fox, …) send `80 00 90 00 C0 00 E4 9F BF DF FF` ($90 where $A0 was meant), briefly setting channel 0 to full volume: a "pop" regression target. Rampage/Kenseiden pops vanish if period-0 output is muted, but Ecco's intro speech needs period-0 output | PoorAussie | 2008-09-13/15 (forums/11384) | ROM analysis |
| SP.089 | Clock: input divided by 16; NTSC 3 579 545 Hz, PAL 3 546 893 Hz | Maxim | 2005 | doc |

## 4. Mapper facts (consumed by CART)

Primary source `Development/Mappers` (wiki, 2024 capture, no author credited) unless noted [SP.090].

| ID | Fact | Source | Handle | Date | Evidence |
|---|---|---|---|---|---|
| SP.091 | $FFFC: bit 7 ROM-write enable (dev RAM), bits 6–5 unused, bit 4 cart RAM over $C000–$FFFF (no known game; needs work RAM disabled via $3E), bit 3 cart RAM in slot 2 (overrides $FFFF), bit 2 RAM bank, bits 1–0 bank shift; $FFFD/E/F select slots 0/1/2; $0000–$03FF is never remapped | Mappers | wiki | 2024 capture | doc |
| SP.092 | Bank shift 00/01/10/11 → +$00/+$18/+$10/+$08, applied when $FFFD–F are next written; one 512 KB mapper chip implements it; no known software uses it. Street Fighter II writes those bits but needs 6-bit bank numbers (768 KB ROM), not shifting | Mappers; forums/11482 | Maxim, FluBBa, Bock | 2008-11-06 | recollection |
| SP.093 | Power-on: undefined for 831001/832011 and 315-5208; the 315-5235 resets to FFFC=00, FFFD=00, FFFE=01, FFFF=02. The export BIOS maps slots 0 and 1 and probably leaves slot 2 at the last page after checksumming; Dega leaves all slots at 0 without harm; a few games need slot 2 = page 2 (unnamed); the official GG doc says the registers are not initialised and shows an init of 0,0,1,2 | forums/12191; GGOfficialDocs | Charles MacDonald, Maxim, RetroRalph | 2010-02-27 | recollection/doc |
| SP.094 | Writes to $FFFC–$FFFF also land in RAM so the last value is readable at $DFFC–$DFFF; the registers are write-only and writing the RAM copy elsewhere does not page; $FFF8–$FFFB are the 3-D glasses | smstech | Charles MacDonald | 2002 | HW |
| SP.095 | Mapper widths 3/5/6 bits (128 KB/512 KB/1 MB largest known); pinouts for 315-5208, 315-5235, 315-5365, 315-5912; slot-2-only mappers present the ROM's second 16 KB in slot 1 | Mappers, MapperPinouts; forums/15013 | Maxim | 2014-09-20 | recollection |
| SP.096 | Codemasters: registers at $0000/$4000/$8000 mirrored across each 16 KB slot, first 1 KB not protected, reset 0,1,0, no RAM mirror; Go! Dizzy Go! (Excellent Dizzy Collection) needs slot-0 paging; Ernie Els Golf maps 8 KB RAM at $A000–$BFFF when a value with bit 7 set is written to $4000; single-slot boards are a PAL plus quad NAND with 4 bank bits (256 KB); Codemasters titles use 224-line mode and suit PAL SMS2; no line-interrupt quirk quantified (unverified) | Mappers; forums/2404 (Bock 2004-04-18); forums/14545 (TmEE 2014-01-12, PCB); smstech; msvdp | as listed | 2002–2014 | recollection/PCB |
| SP.097 | Korean $A000 mapper: slots 0/1 fixed, slot 2 via $A000 (Run! Dodgeball King). MSX 8 KB mapper: $0000→$8000, $0001→$A000, $0002→$4000, $0003→$6000, first 16 KB fixed; Nemesis variant fixes $0000–$1FFF to the last 8 KB bank; the 188-in-1 writes banks to $2000 (unresolved) | Mappers; forums/12395 | Paul Baker | 2010-06-27 | debugger trace |
| SP.098 | Janggun-ui Adeul: 8 KB banks via $4000/$6000/$8000/$A000, 16 KB via $FFFE/$FFFF; bits 0–5 bank, bit 6 bit-reverses bytes on read (sprite-flip assist); writes reach RAM; GG prototypes also mirror to Codemasters addresses | forums/13589 | Maxim, Eke | 2012-02-23 | emulator inference |
| SP.099 | 4 PAK All Action: $3FFE maps $0000–$3FFF, $7FFF maps $4000–$7FFF, $BFFF maps bank ((xx & $30) + zz) to $8000–$BFFF; 1 MB; pages 1–15 hold only `xor a / ld ($3FFE),a`; untouched mapper reads page 1 in both low slots; fails the export BIOS | forums/13548 | Bock | 2012-01-20 | SMS Reader test |

## 5. BIOS and boot facts (consumed by SYS and CART)

Primary source `Development/BIOSes` (disassemblies credited Bock and segmtfault; dumps Hiroshi, Mike Gordon, Mike Cukan, dhau, Eunos), 2024 capture [SP.105].

| ID | Fact | Source | Handle | Date | Evidence |
|---|---|---|---|---|---|
| SP.106 | US/EU v1.3: probes card, cartridge, expansion via $3E (slot codes $CB, $AB, $6B, code running from RAM at $C700); looks for "TMR SEGA" at $7FF0/$3FF0/$1FF0, region nibble 4, checksum by size nibble assuming a Sega mapper; about 1.67 s wait; stores the final $3E value at $C000 (cleared if nothing found) | BIOSes; forums/1292 | Eric R. Quinn | 2001-03-14 | disassembly |
| SP.107 | Hand-off $3E: snail-maze SMS1 and Alex Kidd SMS2 write $AB for a cartridge, Majesco GG writes $A8; bits D7 expansion, D6 cart, D5 card, D4 work RAM, D3 BIOS, D2 I/O, 1 = disabled; D7/D5 do nothing on SMS2; SMS1 gives cart precedence when BIOS and cart are both enabled, SMS2 ANDs the buses; D2 set makes $C0–$FF read $FF on SMS2/GG/Genesis but the last instruction byte on SMS1; GG with D3 clear maps a 1 KB BIOS at $0000–$03FF. No source gives a hardware power-on value for $3E or $3F (unverified) | smstech §4; `Development/Port3E` | Charles MacDonald | 2002 | HW |
| SP.108 | VDP registers are not reset when the BIOS launches a game; Maxim's best-guess residual values: reg0 %00000-00, reg1 %-1000-00, reg2 %----111-, reg5 %-111111-, reg6 bit 2 set, reg7 0, reg8/9 0, reg10 $FF, several marked "probably"; Shanghai needs mostly cleared RAM; export BIOSes are recalled to clear the 8 KB (uncertain) | forums/2008 | Maxim, Zoop, asynchronous | 2002-11-18; 2001-03-17 | recollection |
| SP.109 | Aces of Aces and Shadow Dancer never set SP and need it pre-set (works with $FFFD, not $FFFF or 0); Aces of Aces writes $08 then $00 to $3E ($A8 with BIOS); Fantasy Zone (JP) and Choplifter boot only on the M404 prototype BIOS; the Power Base Converter holds a tiny PAL "BIOS" that sets SP and $C000; Codemasters mappers stay at reset 0,1,0 because the BIOS never touches them; GG Rise of the Robots blank-screens unless something IRQ-related is pre-initialised (vague) | forums/10557; BIOSes; forums/1562 | Fy Li, Maxim, Dave | 2007-11-08; 2001-08-29 | emulator inference |
| SP.110 | When the header is at $7FF0 the mapper must power up with slot 1 = bank 1 because the BIOS does not initialise paging; benryves resets $FFFC–$FFFF to 0,0,1,2 | BIOSes; forums/10557 | benryves | 2007-11-08 | recollection |
| SP.111 | Port $3F: D7/D6 port B TH/TR level, D5/D4 port A, D3–D0 directions (1 = input); region check writes TH as output and reads $DD bits 7/6: export echoes, Japanese reads 0 (tested on a switchable Genesis). $DD bit 4 = Reset (always 1 on Genesis/GG), bit 5 = 1 on SMS/GG and 0 on Genesis | smstech | Charles MacDonald | 2002 | HW |
| SP.112 | Genesis does not initialise Z80 RAM before the game; GG Start is port $00 bit 7 (active low) in GG mode and acts as Pause in SMS mode; port $00 also exposes NJAP (bit 6, 0 = Japan) and NNTS (bit 5, 0 = NTSC) | smstech; `Development/StartButton`; GGOfficialDocs | Charles MacDonald; Sega | 2002 | HW/doc |
| SP.113 | Japanese BIOS v2.1 (MPR-11124): no header check, looks for variation in bit pattern $3C over the first 256 bytes of each slot, plays Space Harrier on PSG+YM2413 when nothing is found; v2.0 accepts any ROM whose first 513 bytes differ; v2.4/3.4/4.4 (MPR-11459A/11458/11460, 1988) embed Hang On + Safari Hunt / Hang On / Missile Defense 3-D in 128 KB and carry the "$2 range" checksum bug; Alex Kidd BIOS (MPR-12808, 1990) static logo, no sound; Sonic BIOS (MPR-15155) 256 KB, SMS2-only; Korean Gam*Boy BIOS checks the first 16 bytes of each slot for a non-$FF byte; M404 prototype checks "COPYRIGHT SEGA" at $7FE0 and `jp $00c8` at $0038 | BIOSes | Bock, segmtfault | 2024 capture | disassembly |
| SP.114 | GG BIOS (Majesco): writes and reads port $02 to tell GG mode (echo) from SMS mode for the palette; checks "TMR SEGA" at $7FF0/$3FF0/$1FF0 and halts with the screen off if absent; VA0 has no BIOS, VA1 (1992) adds it jumper-disabled early, standard by 1993; many non-Majesco units carry the identical ROM; a loader stub with the header that bank-switches works; GG games in SMS mode (OutRun Europa) fail an SMS BIOS checksum | BIOSes; forums/10235, 10210, 4612 | Bock, Maxim, Mike G | 2002–2007 | disassembly/recollection |
| SP.115 | Official GG doc: the CPU may leave reset while the VDP is still resetting, so spin until the V counter reads $B0 before touching the VDP; keep the stack out of $DFF0–$DFFF; port $02 defaults all PC lines to input with NMI-on-PC6 disabled (bit 7 = 1) | GGOfficialDocs | Sega | 2024 capture | official doc |
| SP.116 | Games never call the BIOS; only its residual state matters | forums/11154 | Maxim | 2008-05-26 | recollection |

## 6. Test ROM inventory (consumed by CF; licences by AP)

Checked 2026-10-18 with `gh api repos/<o>/<r>` (licence field, LICENSE file) and the SMS Power Homebrew page for each. "No licence statement" means the SMS Power page shows only "source included / no" and no licence text. GPL ROMs may be **run** as fixtures; their source is never opened or copied (AGENTS.md rule 1).

| ID | ROM | Author (handle) | Year | Tests | Hosted | Licence seen | Source | Pin |
|---|---|---|---|---|---|---|---|---|
| SP.120 | ZEXALL-SMS v0.21 (`zexall.sms`, `zexdoc.sms`) | Maxim; port lineage Cringle, J.G.Harston, Brett K; contributions Eric R. Quinn, FluBBa, asynchronous | 2003–2025 | every Z80 instruction group, CRC per group; v0.20 added a SCF/CCF pre-test that reports per-flag set counts over $1FFF runs | github.com/maxim-zhao/zexall-sms; smspower.org/Homebrew/ZEXALL-SMS | GPL-2.0 (LICENSE; README says ZEXALL is GNU-covered) | WLA-DX, included | `8bfcd2ca594f` |
| SP.121 | Frank Cringle ZEXALL/ZEXDOC original | Frank Cringle | 1994 | same | floooh/chips-test `tests/roms/zex*.src` (MIT harness, GPL-2 ROM source) | GPL-2 | yes | chips-test `9e88298ce563` (chips) |
| SP.122 | SMS VDP Test v1.31 | FluBBa (Fredrik Ahlström) | 2009 (repo pushed 2026-06-17) | VDP data path (codes 0–3, read buffer, address wrap, latch reset), V/H counter values, HC latch via TH, port mirrors, frame/line IRQ V/H positions, VINT flag HCount, sprite collision and overflow flags with HC position | github.com/FluBBaOfWard/SmsVdpTest; smspower.org/Homebrew/SMSVDPTest-SMS | **none** (no LICENSE, no page statement) | WLA-DX, included | `490c00b38c83` |
| SP.123 | SMS Test Suite v0.38 | sverx | 2021–2026 | PLUGE, colour bars, grid, linearity, drop-shadow sprite, striped sprite, PSG channels and volume clipping, FM tune, pads, Paddle, Pause/Reset, VDP 315-5124 vs 315-5246 detection, 50/60 Hz, GG and Twin-ASIC detection, BIOS checksum | github.com/sverx/SMSTestSuite; smspower.org/Homebrew/TestSuite-SMS | **none** | devkitSMS C, in repo (page says "Source no") | `985bc04fef6f` |
| SP.124 | GG Test Suite | sverx | 2022–2026 | GG build of SP.123; START toggles stereo (headphones) | github.com/sverx/GGTestSuite | **none** | devkitSMS C | `1a29804d1fc8` |
| SP.125 | SN76489 TestRom v1.00 (`.sms`, `.gg`, `.sg`) | JoppyFurr | 2024 | interactive piano, per-channel volume, constant-tone mode; GG build toggles stereo bits | github.com/JoppyFurr/SN76489-TestRom | MIT | devkitSMS C | `7d2068dfc171` |
| SP.126 | YM2413 TestRom | JoppyFurr | 2024 | FM instrument and rhythm parameter editor | github.com/JoppyFurr/YM2413-TestRom | MIT | devkitSMS C | `571018fe8bfb` |
| SP.127 | Game Gear Button Test v0.3.0 | szr | 2016 | GG pad and Start | github.com/GameGearSamples/ButtonTest | **none** | Z80 asm | `8c65ecd39ed6` |
| SP.128 | 240p Test Suite | Artemio | 2011– | **no SMS or GG port exists** in the repo (Dreamcast, Genesis, N64, NeoGeo, PCE, SNES, Saturn, Wii, X68000 only); sverx's suites are the SMS analogue, not a fork | github.com/ArtemioUrbina/240pTestSuite | GPL-2.0 | yes | `89adfdd66853` |
| SP.129 | Charles MacDonald test programs | Charles MacDonald | 2000s | scroll test, 64-colour palette test and VDP tests once on his site; `Development/TestPrograms` on SMS Power is a 404; no licensed copy located | none reachable | unknown | no | accessed 2026-10-18 |
| SP.130 | SMS/MD/GG build of FluBBa's VDP test | enik2 | 2019 | SP.122 on Mega Drive and 1- and 2-ASIC GG | SMS Power forum post 107039 | unknown (unverified) | unknown | accessed 2026-10-18 |

Named-but-not-found items: "PSG test", "Light Phaser test", "Line interrupt test", "Sprite test" do not appear as homebrew entries on page 1 of the SMS Power homebrew index (110 entries); the index is paginated and was not crawled exhaustively [SP.131]. No "zexall2" port and no SMS-specific undocumented-behaviour ROM were found [SP.131].

### 6.1 Z80 vector sets (consumed by CF)

| ID | Set | Licence | Size and shape | Notes | Pin |
|---|---|---|---|---|---|
| SP.132 | SingleStepTests/z80 | MIT (LICENSE, 2024) | 1604 JSON files in `v1/` × 1000 cases (256 base, 256 CB, 80 ED, 508 DD incl. DDCB, 508 FD) with per-cycle bus list and `ports`; counted via the git tree (tested) | generated from a translation of ares' Z80; CI validates finals, not cycles [SP.009]; regenerated 2026-04-10 after the DD/FD-zeroes-Q finding [SP.010] | `ebe1875d48f3` |
| SP.133 | raddad772/jsmoo | MIT | `misc/code_generation/z80_test_generator.js` is the origin of SP.132's generator | same format as Harte's ProcessorTests | `359d18e6ea8a` |
| SP.134 | raxoft/z80test v1.2a | MIT | z80full, z80doc, z80flags, z80docflags, z80ccf, z80ccfscr, z80memptr; CRCs from a real 48K Spectrum (Zilog) | needs a shim for the Spectrum ROM print routine and `IN $FE`; z80ccf assumes Zilog SCF/CCF | `c490c0ca94f1` |
| SP.135 | floooh/chips-test | MIT (ROMs inside are GPL-2 Cringle) | `z80-test.c`, `z80-fuse.c` (FUSE vectors), `z80-int.c`, `z80-timing.c`, `z80-zex.c` | CP/M BDOS-stub harness pattern for running ZEXALL headless | chips `9e88298ce563` |
| SP.136 | hoglet67/Z80Decoder | GPL-3.0 | logic-analyser decoder; its wiki carries the SCF/CCF variant tables [SP.014] | reference only, never opened as code | `c6de2d56f849` |
| SP.137 | redcode/Z80 | LGPL-3.0 | reference emulator used by SP.132's CI; passes z80test, Woodmass, zexall/zexall2 | docs only (LGPL) | accessed 2026-10-18 |

### 6.2 Homebrew fixture candidates (consumed by CF and AP)

All licences read from the LICENSE file at the pinned commit on 2026-10-18. devkitSMS games link the GPL-2+exception crt0 [SP.176]; the exception keeps the ROM under the game's own licence.

| ID | Repo | Licence | Machine | Toolchain | Exercises | Prebuilt | Pin |
|---|---|---|---|---|---|---|---|
| SP.140 | haroldo-ok/dragon-blaster | MIT | SMS | devkitSMS, PSGlib | shmup: sprites, scrolling, PSG music and SFX | v0.11 zip | `5b48f0570aeb` |
| SP.141 | haroldo-ok/food-fighter | MIT | SMS | devkitSMS, PSGlib | shmup, PSG SFX | v0.4 zip | `48fe2ea96795` |
| SP.142 | haroldo-ok/river-strike | MIT | SMS | devkitSMS, PSGlib | vertical scroller | v0.10 zip | `acfcc478c14e` |
| SP.143 | haroldo-ok/ck1-sms | MIT | SMS | devkitSMS, banked C (bank3–bank9) | ROM paging across 10+ banks, platformer scrolling; assets derive from a commercial game, so use with caution | v0.0.7 zip | `5dc1a3c3079f` |
| SP.144 | haroldo-ok/chicken-crossing | MIT | SMS | devkitSMS, PSGlib | 4-player multitap input | release v0.4, no asset | `e8209be88eee` |
| SP.145 | haroldo-ok/datastorm | Apache-2.0 | SMS | devkitSMS, PSGlib | Turmoil clone, PSG music | v1.00 zip | `27ef415096f9` |
| SP.146 | haroldo-ok/sms-rogue | Unlicense | SMS | devkitSMS | roguelike, tile text | none | `329d788feb01` |
| SP.147 | haroldo-ok/opensosaria-sms | MIT | SMS | devkitSMS | RPG, menus | v0.0.3 zip | `2229c339ae4b` |
| SP.148 | JoppyFurr/Snepzhen-Solitaire | MIT | SMS | devkitSMS | SRAM save (`save.c`), card UI | none | `94d9d454541a` |
| SP.149 | pw32x/ninjagirl | MIT | SMS | devkitSMS, Tiled | large action game, scrolling; daily ROM builds per README | no GH release | `aa93ba8567de` |
| SP.150 | retcon85/quickstart-sms-devkitsms | MIT | SMS | devkitSMS, Makefile | minimal VDP init, pad, PSG SFX | 2.0 `.sms` | `39c447d87bee` |
| SP.151 | Sanqui/2048-gb branch `sms` | Zlib | SMS | RGBDS-dialect asm via `gbtoz80` | puzzle, static tiles | none | `6f1bcfe7c154` |
| SP.152 | JoppyFurr/SN76489-TestRom `.gg` build | MIT | GG | devkitSMS | GG stereo bits, PSG | yes | `7d2068dfc171` |
| SP.153 | liamhays/tunnel | **GPL-3.0** (run only) | GG | devkitSMS `SMSlib_GG` | GG arcade game, GG window | v1.0 `tunnel.gg` | `79c0c21dffbe` |

Rejected: kusfo/astroforce and StevePro7/AstroForceSMS (no licence, "educational"), kusfo/mastersystembrawler, furrtek/ChappieEarsSMS (no licence); haroldo-ok's 2026 ports of commercial IPs (assets). Waimanu, Silver Valley, Flight of Pigarus, DARC, Galactic Revenge, Nine Pixels, Lucky Penguin, KunKun & KokoKun, Weka Invaders, Bara Buruu, Papi Commando: no source repo found; assume no licence [SP.154]. Permissively licensed Game Gear game source is scarce: the GG column above is the whole verified set [SP.154]. Supporting libraries: lajohnston/smslib (MIT, WLA-DX) `41c467d2ce45`; OrangeRevolt/SMS-devkitSMS-Examples (MIT) `bd021e7537f7`; sverx/GSLib (Unlicense) [SP.155].

## 7. Toolchains for self-authored fixtures (consumed by CF and ENG)

| ID | Tool | Licence (checked) | Latest | Build | Bytes injected | SMS support | Pin |
|---|---|---|---|---|---|---|---|
| SP.170 | WLA-DX (`wla-z80`, `wlalink`) | GPL-2.0-or-later (LICENSE SPDX line, README, doc/legal.rst); GitHub field NOASSERTION; no claim on assembled output anywhere in the docs | v10.7, 2026-06-28 (v10.6 was 2023-11-19) | CMake; CI covers linux gcc/clang, macOS, MSVC; release assets are Win32/Win64 zips and Amiga only, so macOS/Linux build from source | none; `.EMPTYFILL` pads (default $00); no timestamps | `.SMSHEADER … .ENDSMS` (product code, version, region 3–7, ROM size, checksum range, base $1FF0), `.SMSTAG`, `.COMPUTESMSCHECKSUM` ($7FFA/B), `.SDSCTAG`; mandatory `.MEMORYMAP`/`.ROMBANKMAP`/`.BANK n SLOT m` | tag `v10.7` = `91c52b1f4ef3` |
| SP.171 | sjasmplus | BSD-3-Clause | v1.24.0, 2026-08-28 | CMake or make; Windows exe only prebuilt | none | no SMS header or checksum directive; raw `SAVEBIN`; ZX-oriented DEVICE/SLOT model | `7dbf9394d935` |
| SP.172 | z88dk | Clarified Artistic (LICENSE: output is not covered, but library/crt code aggregated into the image stays covered) | v2.4, 2025-10-02; nightlies for Windows and macOS | make/`build.sh`, no CMake | **yes**: crt0 and libc always linked | `+sms`, subtype `gamegear`, appmake writes `.sms`/`.gg` | `801e03849d3b` |
| SP.173 | Pasmo | GPL-2 | 0.5.5, 2022-03-05 | autotools | none | none | accessed 2026-10-18 |
| SP.174 | SDCC | compiler GPL-2 (GPL-3 possible by build libs); sdas/sdld GPL-3; run-time libs GPL-2 with linking exception; manual §1.2: generated code and object files are **not licensed** | 4.6.0, 2026-06-22 | binaries for all three OSes | default `crt0.rel` for `-mz80` unless `--no-std-crt0`, plus libsdcc helpers the C pulls in; pure asm via `sdasz80`+`sdldz80` emits `.ihx` needing `ihx2sms` | none native | accessed 2026-10-18 |
| SP.175 | devkitSMS | no top-level LICENSE; `LICENSES.txt`: SMSlib, PSGlib, SGlib, MBMlib, assets2banks, folder2c, ihx2sms public domain (Unlicense text in `SMSlib/LICENSE`, `PSGlib/LICENSE`); SMSlib embeds ZX7 and aPLib decompressors; the separate sverx/PSGlib repo (WLA-DX asm) is BSD-3 | v1.2 2022-08-15; commit 2026-09-29 | needs SDCC ≥ 4.2.0; macOS binaries only via a third-party fork | see SP.176 | `SMSlib_GG` for Game Gear | `533ae572c897` |
| SP.176 | devkitSMS crt0 (`crt0_sms.s`, `crt0_sg.s`) | GPL-2 with a special exception (Michael Hope 2000, modified sverx 2022) | as above | | the exception's effect: linking this crt0 with other files, some compiled with SDCC, into an executable does not by itself put the executable under the GPL; it does not remove any other reason the executable might be GPL (another GPL object linked in); changes to the crt0 itself stay GPL-2 | | `533ae572c897` |
| SP.177 | vasm (`vasmz80_oldstyle`) | non-commercial-only text in the manual; not OSI-free | 2021 manual | make | none | none | accessed 2026-10-18 |

**WLA-DX v10.7 pin digest (tested).** Downloaded with `curl -sSL -o wla-dx-v10.7.tar.gz https://github.com/vhelin/wla-dx/archive/refs/tags/v10.7.tar.gz` and hashed with `shasum -a 256 wla-dx-v10.7.tar.gz` on 2026-10-18: `38296a96bc20be873d17e0e88be0c5b20a15ef2ec4da5279600e56af30ad925a` (1 402 817 bytes; extracts to `wla-dx-10.7/`, LICENSE = GPL-2.0-or-later). GitHub tag-archive bytes are stable for tags in practice but are not contractually guaranteed; keep the API tarball URL `https://api.github.com/repos/vhelin/wla-dx/tarball/v10.7` and the tag commit `91c52b1f4ef3` as the fallback identity [SP.178].

**Recommendation (SP.179).** Pin WLA-DX v10.7 built from the tag with CMake on macOS, Linux and Windows. Reasons: no bytes in the output that are not in our source (only `$00` fill), so fixture ROMs are wholly owner-authored and MIT-licensable; first-class `.SMSHEADER`/`.COMPUTESMSCHECKSUM`; the bank/slot model mirrors the Sega mapper; deterministic output; the GPL covers the tool, not what it assembles; the same assembler builds ZEXALL-SMS and FluBBa's test, so one toolchain serves fixtures and third-party ROM rebuilds. **Strongest alternative:** sjasmplus v1.24.0 (BSD-3, CMake, single-binary Windows release) with a 20-line macro for the header and a checksum post-step. **Would change if:** the WLA-DX CMake build proves flaky on the Windows runner across two phases, or a fixture needs C (then SDCC 4.6.0 + devkitSMS: output unlicensed, crt0 exception, SMSlib public domain). z88dk is excluded because every output carries its crt0 and libc bytes under the Clarified Artistic licence; vasm because it is not free for CI use.

## 8. Hardware measurement resources (consumed by SYS, VDP, AUD)

| ID | Resource | Who | Date | What it gives | Evidence |
|---|---|---|---|---|---|
| SP.185 | SMS Power forum "SMS Display Timing" (forums/8161) | Charles MacDonald, FluBBa, Christophe | 2006-02 → 2008-05 | logic-analyser timing on an NTSC SMS1 (Y1 pin for active display, HALT trigger); FluBBa's IRQ/HCount tester; see SP.041–SP.047 | HW |
| SP.186 | msvdp-20021112.txt | Charles MacDonald | 2002-11-12 | VDP register, counter and mode reference; author states timing was checked on an NTSC SMS2 with a PAL/NTSC switch and the H counter on an NTSC Genesis, and warns a real SMS may differ; says he had no intra-line event positions at the time | HW + RC |
| SP.187 | smstech-20021112.txt | Charles MacDonald | 2002-11-12 | I/O ports, interrupt bus value, Pause, region detection | HW |
| SP.188 | 315-5124 die shot | furrtek (scan), hosted siliconpr0n `map/sega/315-5124/furrtek_mz/` (about 66 MB); SMS Power `Development/315-5124Die`; forum thread 18855 (bsittler, asynchronous, furrtek, Calindro, sverx, Maxim, Trirosmos annotate PSG, CRAM, DAC and sprite-cache blocks) | 2022-01-09 → 2024-03 | the SMS1 VDP (Yamaha YM2602B) at transistor level; the only Sega SMS die shot found | die |
| SP.189 | 315-5216 die shot (SMS I/O chip) | furrtek, siliconpr0n | 2022 | cited by Nuked-SMS; siliconpr0n returned 403 to a direct fetch, so (unverified) | die |
| SP.190 | Z80 die (Z0840008PSC) and visual6502 JSSim expert-z80 | Antoine Bercovici; visual6502 | 2010s | netlist-level Z80 reference used by hoglet's anomaly notes [SP.031] | die/sim |
| SP.191 | 315-5246 (SMS2 VDP), 315-5377/5378 (GG), SN76489 | none found: searched siliconpr0n, zeptobars, visual6502, the SMS Power die-shot thread and NESdev's chip-image list on 2026-10-18; siliconpr0n's index itself answered 403 | | absence is (unverified) | |
| SP.192 | Nuked-SMS | nukeykt, ogamespec, andkorzh | pushed 2024-09-29 | gate-level SMS1 (YM2602B + 315-5216) emulator derived from the decap photos; GPL-2.0, so **never opened**; its README and issues are citable as T3 facts about the netlist | RTL/doc |
| SP.193 | MiSTer SMS_MiSTer | sorgelig (port), VDP VHDL by "Ben" (Papilio), T80 by Daniel Wallner (BSD), jt89 (GPL-3), VM2413 (non-commercial clause), contributors theflynn49, fbreve, gyurco, greyrogue | commit 2026-10-14 | no LICENSE file at root; `SMS.sv` header says GPL-2+; commits citing hardware: 2020-05-11 TH/HL behaviour changed "so VDP tests pass and lightgun games work" (Shane Lynch), 2021-09-29 PAL border and sync (Yimmers), 2026-08-20 CPU-visible V counter with early update (Fabricio Breve); no commit cites a scope capture | FPGA |
| SP.194 | Analogue Pocket openFPGA-SMS | spiritualized1997 | v1.2.0 2022-08-27 | no licence file and no RTL published; facts only | FPGA |
| SP.195 | SMS Power `Development/SMS1SMS2VDPs`, `Development/GGVDP` | wiki (no credits shown) | accessed 2026-10-18 | 315-5124 in Mark III and SMS1 (legacy TMS register bits affect VRAM addressing); 315-5246 in SMS2 adds 224/240-line modes; a few units swapped; GG VDP equals SMS2's except no TMS-mode palette init and the GG mode (12-bit CRAM, 160×144 crop); Brazilian Master System 3 integrated VDP differs (see `Development/TecToyVDP`, unfetched) | wiki |
| SP.196 | Sega Retro technical pages | | | behind Anubis bot challenge on 2026-10-18; only a search snippet confirmed "315-5124 / Yamaha YM2602 (Mark III, SMS); 315-5246 / NEC uPD9004G (SMS II)"; Sega Retro also mirrors msvdp (unfetched). Treat every Sega Retro number as (unverified) until fetched | |
| SP.197 | SMS Power `Development/ClockRate` | wiki | accessed 2026-10-18 | NTSC crystal 53.6931 MHz, Z80 clock 3 579 540 Hz ("3 579 545 commonly used, difference negligible"); PAL crystal 53.2034 MHz, Z80 3 546 893 Hz | wiki |
| SP.198 | Game Gear board | iFixit (secondary) and VA1 service manual snippet | | VA0 board 837-7996 carries 315-5377 and 315-5378; VA1 merges Z80A, VDP, I/O and LCD control into one custom LSI; which part is the CPU+VDP is (unverified) | RC |

Who to cite, by topic: Charles MacDonald (VDP timing, ports, interrupt bus value), FluBBa (VDP test ROM, counter and IRQ positions, PAL line count), Maxim (PSG research, site, ZEXALL port), Calindro (Emulicious hardware comparisons; no scope capture post located), sverx (test suites, devkitSMS, GG detection), furrtek (die shots), hoglet67 and TonyB (Z80 flag variants), Patrik Rak (SCF/CCF, z80test), redcode (SingleStepTests regeneration, Z80 wiki), Eke-Eke (315-5313 SMS-mode behaviour, precedent only) [SP.199].

## Sources

Each SP ID above is itself the citable unit; this table maps the ID ranges to the underlying page, its tier, and the pin. "Wayback 2024" means a web.archive.org capture of the SMS Power page read on 2026-10-18 because the live site answered 503.

| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| SP.001–SP.011 | T1 | github.com/SingleStepTests/z80 (README, `generation/z80_test_generator.js`, LICENSE, issues #3, #5, #11, git tree) | `ebe1875d48f3` | SingleStepTests format and model |
| SP.012 | T4 | worldofspectrum.org/forums/discussion/41704 | accessed 2026-10-18 | SCF/CCF discovery |
| SP.013, SP.134 | T1 | github.com/raxoft/z80test readme.txt, changes.txt, license.txt | `c490c0ca94f1` | z80test |
| SP.014, SP.015, SP.031 | T2 | github.com/hoglet67/Z80Decoder wiki: Undocumented-Flags, Unstable-CCF-SCF-Behaviour, NMI-during-EI-Anomaly | page edits 2026-07-09, 2024-05-26, 2022-02-25 | flag variants |
| SP.016, SP.023, SP.025, SP.137 | T2 | github.com/redcode/Z80_XCF_Flavor README; github.com/redcode/Z80 wiki Interrupts, README | accessed 2026-10-18 | variants, interrupt costs |
| SP.017, SP.027 | T3 | mamedev/mame `src/devices/cpu/z80/z80.cpp` header (BSD-3 file) | master 2026-10-18 | MAME model |
| SP.018–SP.020 | T4 | stardot.org.uk Z80 Protocol Decoder thread p.3 (Wayback); zexall-sms source | 2018-08; `8bfcd2ca594f` | interrupted block ops, SCF/CCF pre-test |
| SP.021, SP.023–SP.030, SP.032 | T2 | Sean Young, The Undocumented Z80 Documented v0.6 (sam.speccy.cz mirror) | 2003-11-20 | generic Z80 |
| SP.022 | T2 | MEMPTR document, zx.pk.ru 2006, gist drhelius/8497817 | accessed 2026-10-18 | MEMPTR |
| SP.033 | T4 | msx.org forum "Turbo-R and out(c),0" | 2022-05-07 | OUT (C),0 |
| SP.035, SP.036, SP.094, SP.107, SP.111, SP.112 | T2 | Charles MacDonald, smstech-20021112.txt (smspower.org/uploads/Development, Wayback) | 2002-11-12 | ports, INT byte, Pause, mapper mirror |
| SP.036, SP.037 | T2 | SMS Power `Development/InterruptMechanism`, `PauseButton`, `JoypadPauseButton` | Wayback 2024 | NMI/INT wiring |
| SP.038, SP.039 | T4 | copetti.org Master System article; consolemods and spritesmind snippets; Wikipedia Game Gear | 2026-08-08; snippets 2026-10-18 | CPU parts (partly unverified) |
| SP.040 | T4 | gendev.spritesmind.net viewtopic.php?p=36700 | 2020-08 | MD CMOS/open bus |
| SP.041, SP.042 | T4 | SMS Power forums/16466, forums/11361 (Wayback) | 2016-12; 2008-09 | Z80 reference consensus |
| SP.043 | T1 | this harvest's fetch log | 2026-10-18 | access note |
| SP.044–SP.048, SP.057 | T4 | SMS Power forums/8161 "SMS Display Timing" (Wayback) | 2006-02 → 2008-05 | measured VDP timing |
| SP.049, SP.055, SP.072, SP.093, SP.112, SP.115 | T2 | SMS Power `Development/GGOfficialDocs` (Sega GG manual transcription) | Wayback 2024 | official GG VDP text |
| SP.050–SP.054, SP.058, SP.066–SP.070, SP.073, SP.074 | T2 | Charles MacDonald, msvdp-20021112.txt (fetched live) | 2002-11-12 | VDP reference |
| SP.052 | T2 | SMS Power `Development/ScanlineCounter` | Wayback 2024 | V counter tables |
| SP.056, SP.060, SP.061, SP.063–SP.065 | T4 | SMS Power forums/11313, 11320, 15105, 10523, 11689, 14106 (Wayback) | 2007–2014 | line-0 phasing, flags, VRAM pacing |
| SP.059, SP.074 | T4 | SMS Power forums/1568, forums/12656 (Wayback) | 2001-08-31; 2010-10-25 | overflow tests, Genesis mode 4 |
| SP.062, SP.067–SP.070 | T2 | SMS Power `Development/CollisionDetection`, `Sprites`, `VDPRegisters` (live, partial), `SMS1SMS2VDPs` | 2026-10-18 / Wayback 2024 | sprites, modes |
| SP.067, SP.068 | T4 | SMS Power forums/1619, 1643, 11374 (Wayback) | 2001-10 → 2008-10 | zoom bug, Y+1 |
| SP.072, SP.073 | T2 | SMS Power `Development/GGVDP`, `GGLCD`, `Palette` | Wayback 2024 | GG window, CRAM |
| SP.076–SP.083, SP.085, SP.086, SP.089 | T2 | SMS Power `Development/SN76489` | Wayback 2024 (history to 2005-11-12) | PSG |
| SP.078, SP.084, SP.085 | T4 | SMS Power forums/6838 | 2005-05-31 → 07-24 | PSG polarity, LFSR step, GG Aleste |
| SP.087, SP.088 | T4 | SMS Power forums/11371, 11384 | 2008-09 | PSG counter, init pops |
| SP.090–SP.092, SP.095–SP.097 | T2 | SMS Power `Development/Mappers`, `MapperPinouts` | Wayback 2024 | mappers |
| SP.092, SP.093, SP.095–SP.099 | T4 | SMS Power forums/11482, 12191, 15013, 2404, 14545, 12395, 13589, 13548 | 2004–2014 | mapper details |
| SP.105, SP.106, SP.109, SP.110, SP.113, SP.114 | T2 | SMS Power `Development/BIOSes`, `Port3E`, `StartButton` | Wayback 2024 | BIOS |
| SP.106, SP.108, SP.109, SP.110, SP.114, SP.116 | T4 | SMS Power forums/1292, 2008, 10557, 1562, 10235, 10210, 4612, 11154 | 2001–2008 | BIOS hand-off |
| SP.120–SP.131 | T1/T2 | GitHub repos listed in §6 (`gh api`); SMS Power Homebrew pages | pins in table; 2026-10-18 | test ROMs |
| SP.132–SP.137 | T1 | GitHub repos listed in §6.1 | pins in table | vector sets |
| SP.140–SP.155 | T1 | GitHub repos listed in §6.2 (LICENSE files at pin) | pins in table | fixtures |
| SP.170–SP.179 | T1 | vhelin/wla-dx (LICENSE, README, doc/asmdiv.rst, doc/legal.rst, releases); z00m128/sjasmplus; z88dk/z88dk LICENSE; pasmo.speccy.org; sdcc.sourceforge.net manual §1.2; sverx/devkitSMS LICENSES.txt, crt0/src/crt0_sms.s; vasm manual | pins in table; digest computed here | toolchains |
| SP.185–SP.199 | T2/T3/T4 | forums/8161; msvdp; smstech; siliconpr0n.org/map/sega/315-5124 (index via Wayback); SMS Power `Development/315-5124Die`, forums/18855; MiSTer-devel/SMS_MiSTer (`8443a0d19a88`); nukeykt/Nuked-SMS; spiritualized1997/openFPGA-SMS; `Development/ClockRate`; Sega Retro (Anubis-blocked) | 2026-10-18 | hardware resources |
