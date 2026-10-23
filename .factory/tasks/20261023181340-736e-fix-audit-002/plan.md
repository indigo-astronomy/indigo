# Plan

1. Test actual compiled library sources with a failing synthetic resolver to distinguish accepted parsing from pre-resolution rejection, without DNS or sockets. Capture pre-fix regression and sanitizer evidence.
2. Add equivalent bounded parsing checks in each helper. Keep existing protocol dispatch and public ABI.
3. Run regression, sanitizers and configured Factory checks, review quality/security in separate same-agent passes. Incorporate verified predecessor target after 005, reverify, close finding, archive and commit. Coordinator integrates.
