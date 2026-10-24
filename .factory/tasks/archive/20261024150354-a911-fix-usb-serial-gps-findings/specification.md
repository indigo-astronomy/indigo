# Specification

AC-USB: macOS USB serial enumeration writes at most caller capacity, including multiple serial descendants per USB device. Zero/negative capacity produces zero entries without acquiring an iterator object that cannot be released. Release every acquired IOKit object, iterator and CF property on success, capacity exhaustion and existing error paths. Preserve discovery order and metadata for entries that fit. Other platform enumeration behavior unchanged.

AC-GPS: absent optional UTC_TIME/GPS_STATUS/GPS_ADVANCED properties are skipped; present optional properties still require their mandated items and valid values. UTC_TIME.OFFSET remains optional. Mandatory GEOGRAPHIC_COORDINATES failures remain failures. Final script exit is nonzero if any test failed and zero otherwise, after connection restoration and summary. No actual GPS hardware/server calls in regressions.

AC-OMNI: start_omnisim returns a live process after successful readiness; readiness failure/interruption invokes the existing best-effort stop policy for the owned child/session before propagating the original failure. Popen failure also closes the parent output stream. Check that the child has not already exited after readiness; cleanup exceptions must preserve the original startup failure. Keep existing process-stop policy and readiness timeout unless required for ownership correctness; use pure fake-Popen/wait tests, no external OmniSim/network.

Complexity medium: three bounded, independent ownership/status fixes. Risk high for USB memory safety; scripts medium functional/quality. No unresolved owner decision. Preserve signatures/dependencies, no public property or driver version changes.
