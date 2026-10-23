# Specification

AC-001: HTTP 404/500, network errors, aborts, timeouts, empty/partial responses and download preparation errors preserve the source and stop the batch, show an error, restore the button and allow retry.
AC-002: A complete HTTP 200 nonempty Blob triggers a browser download followed by deletion of precisely the captured filename, then the established 100 ms continuation.
AC-003: Repeated button clicks, property updates and terminal callbacks cannot duplicate a transfer or deletion. Agent download-property Alert also stops safely.

Complexity: medium (asynchronous browser/property lifecycle); impact: high (source data loss). Browser download dispatch does not confirm a disk save; preserve the existing successful-transfer contract. No new dependencies or server-side authorization changes.
