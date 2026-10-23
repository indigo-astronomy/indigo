# Specification

AC-001: Server guide lists all current help options with platform/build conditions and descriptions for buffering, compression, bus tracing and Raspberry Pi management; omit stale build metadata.

AC-002: Keep exactly one edited asynchronous property-handling introduction.

AC-003: Fix the bus-header target and service-discovery label using existing local paths.

AC-004: Explain root-generated Makefile.inc, component working directories and current automatic generator build/regeneration behavior, correcting copyable typos.

AC-005: Explicitly document inline and checked-in remote examples’ host/service/device defaults and lack of bounded failure handling.

AC-006: Close only these five findings after verification, retaining discovery and repair evidence.

Complexity: medium; requires reconciling imported evidence against current make rules and server options. Risk: low, reversible documentation/metadata changes. No unresolved product questions, dependency changes or runtime behavior changes. Existing example defects stay tracked separately (FND-000010).
