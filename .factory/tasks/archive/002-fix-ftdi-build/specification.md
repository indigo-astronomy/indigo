# Specification

AC-001: a normal detach refusal retries force only for this attempt's uniquely created read-only mount; neither global mount paths nor unrelated processes are touched.
AC-002: target archive appears atomically only after every architecture is stripped, indexed, recombined and verified, headers installed and mount detached. Any failed operation leaves no newly published target and removes staging outputs; failed detach retains its mount/workspace with an actionable message.
AC-003: ordinary success and failure cleanup preserve existing output and correctly propagate exit status. Handle quoted paths and interrupted processing.
AC-004: the library-only build runs outside the sandbox; report its actual exit and diagnose any subsequent error without unrelated changes.
Complexity medium: shell traps, ownership, failure propagation, multi-architecture atomic output. Risk medium: force detach is restricted to a private read-only mount; no source or device behavior changes. No new runtime dependency.
