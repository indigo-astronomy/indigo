# Regression test plan

AC-1: open failure has no subsequent USB calls; valid open and retry.
AC-2: immediately ready, delayed ready, both permanently busy modes, device error; deterministic fake monotonic clock.
AC-3: complete success transfers; CBW/data/CSW truncation and errors; lengths zero, below minimum, above 16-bit and maximum; all model-state field extents; both endian identity/firmware and valid state.
AC-4: repeated fake attach/removal tracks allocations, detach ordering and USB reference release.
AC-5: original success characterization, native build, sanitizer, recorded runner. No hardware calls.

AC-6: fake hotplug with missing descriptor indexes, failed string reads and maximum-length non-terminated string payload; check attached name and teardown ownership.
