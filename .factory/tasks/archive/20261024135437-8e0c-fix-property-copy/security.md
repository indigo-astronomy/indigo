# Security review

Verdict: pass

Reviewed base: ce3a972818fb2875a7a901678c5c68e406f4c703, combined task working tree; final Git content identities in process.json. Date: 2026-10-24. Independent review contexts and role separation are documented in quality-review.md. Exact runtime model/effort unobservable; owner-selected current host retained, no claim of a switched model or external review.

## Scope and risk analysis

FND-20261024121920-3bd2-repository-audit-001: preserve actual allocation capacity and text ownership through reuse, release inactive allocated text slots, deep-copy active long text, preserve self assignment and existing borrowed BLOB behavior. No generic input-validation duplication in drivers.

002: resource lookup acquires response ownership under registry mutex. All success/error paths release it. Owned metadata survives caller mutation. Single/all removal withdraws entries and drains users, serialized against overlapping removers; registry lock is released during waits. Handler code and state must remain live until removal returns, and callback self-removal or holding callback-needed locks is explicitly forbidden in the public header. Alpaca removes handlers before its client/device teardown; version 3.0.0.16. Borrowed static response bytes remain caller-owned and unchanged until removal completes. No unrelated BLOB payload ownership redesign is claimed.

003: all enumerated prefix/suffix families reserve actual capacity or reject unrepresentable identities/paths; display names/groups may be bounded. 010: already formatted/raw strings pass to raw APIs, preserving ordinary percent-bearing data through wrappers/snoop/FITS/server/default-directory paths. 011: every filename append/render is bounded, numeric formatting uses constant formats, token substitution parses only original prefix controls, replacement/directory text remains literal, all three output callers supply capacity, and overlong paths use existing failure/ALERT behavior.

042: capture ties manual motion to its initiating pointer; release/cancel/lost capture/window or page loss sends one matching stop independent of BUSY changes. Explicit abort is retained. Browser crash, disconnected network or physical hardware behavior is outside this frontend event-handling guarantee.

Authentication/authorization, token semantics, existing route matching, public property definitions and protocol formats are preserved. No new dependency, secret, production dataset, third-party request, deployment or provider call. Filesystem tests use isolated private output/HOME and tiny synthetic images; network tests use isolated local loopback only. No exploit construction or third-party vulnerability reproduction was performed.

## Checks and disposition

Runtime and exact commands are in verification.md and retained logs; final combined property/core/CCD and HTTP sanitizer checks pass, and local leak inspection reported0 leaks. Independent source review corrected two draft regressions described in quality-review.md before final acceptance. Factory security boundary checker is a procedural check, not proof of application security.

All six target findings are eligible for fixed closure after the documented checks; canonical IDs and original audit evidence are retained, with this report and verification added as repair evidence. No risk acceptance, waivers, new unresolved security finding or whole-system certification. Remaining audit findings remain open and unchanged. Linux/Windows, hardware/ConformU, full Alpaca shutdown-concurrency execution and real-browser pointer routing remain explicit coverage limits.
