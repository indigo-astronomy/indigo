# Test plan

Connection diagnostics: identify0a17:00ce, exact module version, model/firmware/status, connected OK then disconnected, no exposure. Existing baseline server is limited by timeout18s plus3s forced-stop grace. No pre-existing server/process may be stopped.

Current-version test: isolated3.0.0.4 build must compile/link against RPi framework; verify actual loaded version. Initially connection/status/disconnect only. Full existing short grouped hardware suite can cover controls, native image, bulb, abort/recovery and restoration after owner notification; no physical unplug. Final recorded verdict through tools/run_driver_test.py, not the diagnostic command.
