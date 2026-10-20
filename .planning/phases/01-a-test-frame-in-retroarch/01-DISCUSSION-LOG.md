# Phase 1: A test frame in RetroArch - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-10-19
**Phase:** 01-a-test-frame-in-retroarch
**Areas discussed:** Phase-1 extras, Header breadth, Test card design

The owner selected all three areas and asked for a fan-out per point (every stakeholder lens, primary sources,
adversarial pass, one coherent recommendation, auto-followed; "another copy and paste beats another
dependency"). Advisor mode (calibration `minimal_decisive`) ran one researcher per area.

---

## Phase-1 extras

| Option | Description | Selected |
|--------|-------------|----------|
| A. Each item lands with the phase that first gives it data | Phase 1 gets the internal state visitor and the first-fault struct, recorder and getter; runner options move to phases 2–4, v2.0, v3.0 | ✓ |
| B. Whole v1.0 runner surface as phase-1 stubs | `--probe`, `--expect`, `--capabilities`, `--input`, `--hash-audio`, `--trace`, `--boot`, `--port`, `--region` stubbed | |

**User's choice:** auto-follow the recommendation (A).
**Notes:** The header researcher put the public state API in phase 1; the extras researcher kept the visitor
internal until STATE-01. The orchestrator chose internal only (`retro_serialize_size` stays 0, no run-ahead or
rewind claim).

## Header breadth

| Option | Description | Selected |
|--------|-------------|----------|
| A. Whole ARCHITECTURE §4 API now with UNSUPPORTED stubs | ~13 stubs, layouts frozen early, NOT_IMPLEMENTED status | |
| B. Phase-grown header | 9 functions in phase 1; each declaration lands with code and a test | ✓ |

**User's choice:** auto-follow the recommendation (B).
**Notes:** The adversarial pass found the size-tag rule ("smaller than the library's → error") would break older
callers whenever a field is appended; it was made precise with first-released-size semantics (D-11).

## Test card design

| Option | Description | Selected |
|--------|-------------|----------|
| A. Static software card | Pure function of (x, y, machine); frame advance proven by callback counts and frame counter | ✓ |
| B. Animated card | Moving bar or counter; the hash depends on N and the RetroArch shot needs frame alignment | |

**User's choice:** auto-follow the recommendation (A).
**Notes:** GG shows all 4,096 RGB444 colours (resolves FRAME-02 vs ARCHITECTURE §9). The researcher's audio-total
assertion was dropped because the audio API arrives in phase 4.

## Claude's Discretion

- Plan breakdown and waves; how much muhtari code to copy at its pin; silence vs no audio in the phase-1 adapter.

## Deferred Ideas

- Public state API (STATE-01); runner options per D-04..D-08; animated test card (rejected).
