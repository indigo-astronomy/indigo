# Specification
AC-001: Gaia directory and entry names are processed literally; no shell or subprocess is used for enumeration.
AC-002: enumerate visible case-sensitive .csv basenames with deterministic ordering, preserving parsed stars for ordinary catalogs, spaces/newlines and shell punctuation. Ignore unrelated/hidden files as the former *.csv selection did.
AC-003: enumeration/open/path-construction failures produce explicit nonzero failure; never open a truncated path. Release enumeration allocations on normal completion and handled failures.
AC-004: regression reproduces the pre-fix shell-boundary flaw and verifies the actual changed parser with synthetic fixtures; no large index or real catalog is needed.
AC-005: independent quality/security review and relevant checks pass; close finding with retained provenance. No generator/driver/property behavior changes, no hardware or external network tests.
Complexity medium: portable API ownership, deterministic enumeration and parser harness integration; risk medium at CLI path→shell boundary. Strong qualified security reasoning is recommended; inherit current host model/settings (exact observed metadata unknown), no provider switch/new charges/budget configured.
