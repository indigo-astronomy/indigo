# Security review

Verdict: pass
Reviewer: implementing agent (Claude Code, claude-opus-5-5), separate pass
Date: 2026-10-24
Revision and working-tree state: base 923bca9a214d4b5d9ba2982e2964e982f26c007a plus the ccd_rpi diff
Review independence: same agent as the implementer; the independent diff review is recorded in quality-review.md

## Scope, assets, and trust boundaries
Local driver code handling libcamera frame buffers, the LIBCAMERA_LOG_LEVELS environment value and camera ids. No network, credential or file-path handling is added.

## Risk coverage
- Memory safety: unbounded strcpy/strcat replaced by std::string (FND-000023); mmap failures checked and plane offsets bounded (FND-000024); frame size checked against the image buffer before memcpy; CCD_MODE resized before items are written (FND-000025); C++ object lifetime of private data fixed (audit-030).
- Concurrency: use-after-unmap and use-after-delete between abort, disconnect and exposure callbacks removed (quality-review.md).
- No new dependencies.
