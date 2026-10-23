# Audit specification

AC-001: explicit file/component inventory, exclusions and reviewed snapshot; distinguish manual, automated and unavailable coverage, never equate a sample with a full audit.
AC-002: inspect maintained framework, driver/agent sources and DSL/generator contracts, platform/optional sources, server/UI, tools/examples, tests, build/CI/deployment/docs and Factory instructions. Vendor/data/generated exclusions must be evidenced.
AC-003: reconcile inherited FND aliases without duplicates or unsupported closure; register each new confirmed issue with exact locations, impact, confidence, ownership and concrete next action.
AC-004: run safe relevant local checks, capture actual counts/failures and isolate any reproduction. No ambiguous driver/hardware run or undocumented third-party access.
AC-005: separate audit and task diff/security review; update full-audit checkpoint only after complete coverage and finding dispositions. Preserve partial state if unfinished.
Complexity high because the repository mixes native concurrency/lifecycle contracts, generated source, SDK integrations, fixtures and several toolchains. Risk is primarily inaccurate findings or overclaimed coverage; no production edits are permitted. Maintain current host/model; independent review is unavailable under existing no-delegation constraints.
