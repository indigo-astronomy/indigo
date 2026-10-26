# Test plan

- Per finding: add a regression at the lowest layer (unit, in-process integration, serial simulator with fault option, fake SDK) and capture its pre-fix failure.
- Merged candidate: `make -k -j8 all`, `make -C indigo_test test-unit`, the new/affected integration targets (`test-indigo-js-formatting`, `test-server-static-driver`, `test-server-security-sanitize`, `test-imager-download`, `test-raw-image-sanitize`) and `python3 tools/run_driver_test.py <driver> --no-record` for every changed driver.
- Failures are compared with a master baseline build before being attributed.
- No coverage: ccd_dsi (no fake), Arduino firmware simulators (no toolchain), Temma/NexStar long deadlines; hardware, Windows and Linux not run.
