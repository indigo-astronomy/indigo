# Plan

1. Retain baseline: make -C indigo_libs all exited 2 at hdiutil detach, blocked by Xcode; installed target still matched the original vendor archive and contained all nine libusb members. The prior diagnostic removed that incomplete output and force-detached the owned read-only image.
2. Add disposable command-boundary regressions for successful/blocked detach, archive-processing failures, attach failure, existing target preservation, paths and Makefile wiring.
3. Move macOS FTDI work into a small POSIX shell helper invoked by the existing target. Unique work/mount, same-directory temporary archive, explicit error propagation and traps, force fallback on this mount only, publish last. Leave Linux recipe unchanged.
4. Run all focused regressions; register every new persistent path in Xcode. Run library-only build outside sandbox, diagnose next failure if present.
5. Separate verifier/quality/security passes in this same agent; archive completed evidence and commit locally. No independent review, integration or publication is claimed. Preserve the initial audit checkpoint.
