# Test plan

AC-001: repeated cached BLOB definition/update/deletion and EOF under allocation/leak instrumentation; preserve delivered bytes.
AC-002: RO PUT rejected and snapshot unchanged; writable upload/consume/re-upload behavior and existing concurrent cache tests.
AC-003: isolated file resource GET including early peer closure and truncated source; observe handle reclamation and successful download.
AC-004: loopback idle deadline, admission boundary/recovery, failed thread-creation cleanup where feasible; established session survives initial timeout.
AC-005: XML and JSON item boundary/boundary+1, mixed long text allocation budget, malformed/EOF cleanup, no partial dispatch; existing large BLOB and text tests preserved.

Use existing opt-in sanitizer security target for network tests; normal integration suite does not open sockets. Test only local synthetic fixtures. Record exact commands/results in verification.md.
