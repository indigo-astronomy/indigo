# Temporary sanitizer build evidence

Working directory: /Users/polakovic/Development/indigo; macOS arm64. Test developer compiled the existing test and both production agents/native solver sources with ASan/UBSan, linking the existing uninstrumented framework library. No persistent Makefile target was added.

```sh
make -C indigo_test -f Makefile -f /tmp/indigo-solver-exclusive-sanitize.mk /tmp/indigo-test-agent-solver-exclusive-sanitize > /tmp/solver-exclusive-sanitize-build2.log 2>&1
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/indigo-test-agent-solver-exclusive-sanitize > /tmp/solver-exclusive-final-sanitize.log 2>&1
```

Both exited0 after the unsigned-byte fixture fix; full26/26 passed. Leak detection disabled; no claim about framework sanitizer or leak coverage. ASTROMETRY_SANITIZE_CFLAGS uses current TEST_CFLAGS stripped of universal architectures, then -arch arm64 -O1 -fsanitize=address,undefined -fno-omit-frame-pointer. Temporary makefile retained below for reproduction:

```make
/tmp/indigo-test-agent-solver-exclusive-sanitize: integration/test_agent_solver.c ../indigo_drivers/agent_solver/indigo_agent_solver.c ../indigo_drivers/agent_solver/indigo_solver.c ../indigo_drivers/agent_solver/indigo_solver_utils.c ../indigo_drivers/agent_solver/indigo_solver_healpix.c ../indigo_drivers/agent_astrometry/indigo_agent_astrometry.c
	$(CC) $(ASTROMETRY_SANITIZE_CFLAGS) -o $@ $^ $(filter-out -arch arm64 x86_64,$(TEST_LDFLAGS)) $(SANITIZE_ARCH) -fsanitize=address,undefined -lindigocat
```
