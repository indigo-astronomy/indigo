# Specification

AC-1: Removing an initialized registration clears its initialized flag after successful shutdown only.
AC-2: Adding another driver with init=false into the same slot starts uninitialized; its INIT has not run.
AC-3: Fresh INIT success/failure and rejected removal preserve truthful state; dynamic/static loading and solver conflict scope remain compatible.

Baseline b6d3ee192. The driver registry entries store callback/handle/name and initialized. Removal currently clears all except initialized. State assignment must precede publishing the new registration to make reused slots truthful. Medium complexity/high lifecycle consequence; no ABI/property change.
