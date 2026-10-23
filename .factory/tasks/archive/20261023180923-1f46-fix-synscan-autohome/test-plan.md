# Test design

AC-001: existing AUTOHOME success/readback and queued encoders regression; retain original ordered simulator events and property transitions. Exercise both preparation directions and optional second offset using simulator physical initial positions.
AC-002: stage-selected abort and disconnect tests observe protocol edges, assert response latency, L1/L2, no later J/E, and fresh lifecycle restart. Stages include initial stopping, preparation offset/extra offset, reverse index/dwell/reverse stop, forward search, backoff/return and final stopping.
AC-003: inject refused/malformed/silent commands and stuck motion/index in simulator; assert ALERT, cleared switch, stop edges and bounded deadline. Keep production timeout values; use test clock acceleration only through existing test-driver mechanism if supported.
AC-004: full existing 47-case driver simulator suite including guiding timing, serial/UDP and shared logical interfaces. Hardware runs 0; unavailable Linux/Windows explicitly reported.
