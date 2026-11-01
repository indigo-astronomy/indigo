# Implementation specification

Status: approved for implementation by owner on 2026-11-01.

## Contract and acceptance
- AC-001: In the normal serialized, registry-managed server lifecycle, successfully initializing Astrometry prevents Solver initialization and vice versa. Return INDIGO_FAILED and identify the conflicting driver in a log. Exactly one of the two agents is attached.
- AC-002: Refuse the second INIT before changing last_action or creating resources, validating executables, attaching devices/clients or modifying its cache. After successfully unloading the active driver, the previously rejected driver can initialize. A freshly registered inactive competitor, as in normal server static discovery, does not block initialization. A separately observed preexisting recycled-slot initialized-flag defect is outside this task and must be tracked explicitly.
- AC-003: Repeated INIT/INFO and ordinary independent-agent solving behavior remain compatible. Every changed driver version must increase; no properties are added, removed or renamed.
- AC-004: Meaningful registry-based hardware-free tests cover both orderings, failure/no-side-effects and unload/retry; existing relevant suites remain passing. Do not change ASTAP, generator, framework locking or unrelated drivers.

## Scope, baseline audit and constraints
Both are hand-written entry points sharing the platesolver base. Astrometry invokes external solve-field/image2xy/curl; Solver uses the native quad backend and curl for optional downloads. Each has per-device solve/config mutexes but neither excludes the other at initialization. Shared platesolver code enables automatic image solving and exposes mount/PA workflows; excluding the whole initialized driver avoids covering every operation path. The existing indigo_driver_initialized API searches local initialized flags and requires synchronized lifecycle callers. Server DRIVERS/LOAD/UNLOAD use a shared serial queue; command-line loading is sequential. Unregistered direct INIT, concurrent arbitrary library callers, separate processes and remote agents are outside this contract. Same-driver additional instances are unchanged.

Tests: existing Astrometry fake-executable suites and Solver synthetic-index integration. New regression must use real agent entrypoints and registry rather than a fake conflict query. Build both agents narrowly before production change; record baseline and expected new-test failure. No physical hardware tests: these agents' changed behavior is process-local driver initialization. Linux and Windows runtime/build validation are unavailable on this macOS arm64 host unless an actual environment is discovered. The change must use portable existing API and add no dependency or platform conditional.

No authentication boundary changes, outbound calls, migrations or persistent lock files. No generic global-lock API changes. No blocking product ambiguity remains. Lifecycle concurrency analysis is high complexity; implementation itself is a bounded symmetric check with low change risk.
