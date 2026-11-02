# Specification

AC-1: Editor frontend/backend recognize polaralign as a supported device class; structural edits and previews for both checked-in definitions succeed without changing source or generated repository outputs.
AC-2: Native catalogue rejects foreign, one-past and unaligned star addresses before subtraction; valid layouts roundtrip.
AC-3: Removed driver registration metadata cannot make a later uninitialized registration look initialized; existing successful init/unload/exclusivity preserved.
AC-4: PrimaLuce LED refusal produces failure/ALERT and preserves previous accepted state; healthy generic/named acknowledgement compatibility retained.

Baseline current main b6d3ee192. Editor change is medium complexity/low impact, loader/membership/transport boundaries medium/high risk. No physical hardware. Delegate separate files/worktrees; no source-map or generator lifecycle redesign.
