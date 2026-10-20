---
schema_version: 1
open_count: 1
waived_count: 0
fixed_count: 0
total_count: 1
last_updated: 2026-10-20T18:20:01.551Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 03 | unrun-verify | cmake/toolchains/wasm32-wasi.cmake |  | Optional wasm32 workflow could not configure locally because the Linux WASI compiler toolchain is unavailable; CI portable registrations are present. | open |  | 2026-10-20T18:20:01.551Z |  |

````json
[
  {
    "id": 1,
    "kind": "unrun-verify",
    "phase": "03",
    "file": "cmake/toolchains/wasm32-wasi.cmake",
    "line": null,
    "description": "Optional wasm32 workflow could not configure locally because the Linux WASI compiler toolchain is unavailable; CI portable registrations are present.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-10-20T18:20:01.551Z",
    "resolved_at": null,
    "milestone": "v1.0"
  }
]
````
