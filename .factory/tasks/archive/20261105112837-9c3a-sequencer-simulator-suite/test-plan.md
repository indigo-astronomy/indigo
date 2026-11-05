# Test plan

AC-001: Compare the public prototype inventory with per-case method declarations and passed runtime cases. Report positive, negative and missing coverage separately. Existing baseline has 63 cases and 108 constructor calls; nine methods have no runtime call outside construction.

AC-002: New opt-in JavaScript suite evaluated unmodified by the real Scripting Agent on an isolated native server. Real CCD/DSLR, mount, dome, GPS, rotator simulators; actual Imager/Mount/Guider/Configuration/Platesolver agents. Copy existing solver indexes only when explicitly provided; never download automatically.

AC-003: Per-step observed readbacks, trace assertions, expected Alert paths, finite deadlines, real pause/resume/abort controls and restart checks. Capture tests verify fresh files and frame statistics; FITS header checks run on generated files. Include all four acquisition overloads, both sky-branch directions, loops/reset modes, all policies, DSLR setters, actual calibration/guiding and legacy alias. Complete flip acceptance remains distinct from feature-toggle coverage.

AC-004: Launch a private server with new HOME and output directory; refuse attaching to existing hardware servers. Shutdown only the owned process. Preserve machine-readable JSON and per-case messages. Existing native regression suite remains complementary, including injected solver/flip races.

No physical hardware tests. No production change planned. Root owns all writing; read-only coverage analyst supplied method inventory and gap analysis.

Final additions: recovery independently inside repeated blocks; pause/resume during actual capture; exact JNow-to-J2000 geometry for natural transit; both solver-free and native solver-corrected flip. Focus restoration uses an actual two-frame/.07s baseline distinct from the one-frame/.05s focus helper. FITS artifact checks verify the exact custom value and removal before END. All are executed through the prepared script; only primitive request authentication is relayed.
