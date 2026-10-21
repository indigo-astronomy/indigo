# Task brief

Task ID: 002-fix-ftdi-build. Owner: Peter Polakovic. Date: 2026-10-21, Europe/Bratislava.
Fix macOS FTDI dependency installation so blocked DMG detach is handled and an incomplete archive is never published. Then rerun the library-only build outside the sandbox and diagnose further failures. No driver/vendor-source/README changes, merge or push.
The owner's explicit request prioritizes this bounded build repair ahead of the still-due initial audit; it does not mark that audit complete. The task starts from e5f25534910b7f51d3e1e87daace13dcd9820d90, including the unmerged Factory prerequisite, without merging it into master.
