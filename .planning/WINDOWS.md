---
schema_version: 1
open_count: 0
waived_count: 0
fixed_count: 5
total_count: 5
last_updated: 2026-10-20T22:28:03.831Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 03 | unrun-verify | cmake/toolchains/wasm32-wasi.cmake |  | Optional wasm32 workflow could not configure locally because the Linux WASI compiler toolchain is unavailable; CI portable registrations are present. | fixed |  | 2026-10-20T18:20:01.551Z | 2026-10-20T19:10:22.500Z |
| 2 | 03 | deviation | tests/retroarch/checkpoint.c |  | Released frontend replay needs an initial checkpoint; a test-only module forwards the existing state visitor while input and video remain production source. | fixed |  | 2026-10-20T19:10:22.575Z | 2026-10-20T19:11:53.125Z |
| 3 | 03 | deviation | tests/policy/e2e_cases.cmake |  | Register hosted helper accounting in CI as well as hygiene, including the shared independent oracle. | fixed |  | 2026-10-20T19:10:22.654Z | 2026-10-20T19:11:53.195Z |
| 4 | 03 | deviation | tests/hygiene/tests.cmake |  | Exact-SHA hosted integration exposed earlier inventory, binary text scanning, MSVC, emulator quoting, newline fixture and WASI scratch-stack issues; targeted corrections now pass all lanes. | fixed |  | 2026-10-20T19:10:22.742Z | 2026-10-20T19:11:53.265Z |
| 5 | 03 | deviation | tests/cmake/budgets.cmake |  | Budget blocker resolved through standard aggregate counting, direct checked output and removal of unused COST configuration; complete parser and update code remain counted. | fixed |  | 2026-10-20T22:27:00.522Z | 2026-10-20T22:28:03.831Z |

````json
[
  {
    "id": 1,
    "kind": "unrun-verify",
    "phase": "03",
    "file": "cmake/toolchains/wasm32-wasi.cmake",
    "line": null,
    "description": "Optional wasm32 workflow could not configure locally because the Linux WASI compiler toolchain is unavailable; CI portable registrations are present.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-10-20T18:20:01.551Z",
    "resolved_at": "2026-10-20T19:10:22.500Z",
    "milestone": "v1.0"
  },
  {
    "id": 2,
    "kind": "deviation",
    "phase": "03",
    "file": "tests/retroarch/checkpoint.c",
    "line": null,
    "description": "Released frontend replay needs an initial checkpoint; a test-only module forwards the existing state visitor while input and video remain production source.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-10-20T19:10:22.575Z",
    "resolved_at": "2026-10-20T19:11:53.125Z",
    "milestone": "v1.0"
  },
  {
    "id": 3,
    "kind": "deviation",
    "phase": "03",
    "file": "tests/policy/e2e_cases.cmake",
    "line": null,
    "description": "Register hosted helper accounting in CI as well as hygiene, including the shared independent oracle.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-10-20T19:10:22.654Z",
    "resolved_at": "2026-10-20T19:11:53.195Z",
    "milestone": "v1.0"
  },
  {
    "id": 4,
    "kind": "deviation",
    "phase": "03",
    "file": "tests/hygiene/tests.cmake",
    "line": null,
    "description": "Exact-SHA hosted integration exposed earlier inventory, binary text scanning, MSVC, emulator quoting, newline fixture and WASI scratch-stack issues; targeted corrections now pass all lanes.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-10-20T19:10:22.742Z",
    "resolved_at": "2026-10-20T19:11:53.265Z",
    "milestone": "v1.0"
  },
  {
    "id": 5,
    "kind": "deviation",
    "phase": "03",
    "file": "tests/cmake/budgets.cmake",
    "line": null,
    "description": "Budget blocker resolved through standard aggregate counting, direct checked output and removal of unused COST configuration; complete parser and update code remain counted.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-10-20T22:27:00.522Z",
    "resolved_at": "2026-10-20T22:28:03.831Z",
    "milestone": "v1.0"
  }
]
````
