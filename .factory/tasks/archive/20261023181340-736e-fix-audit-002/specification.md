# Specification

AC-1: Both public URL helpers reject empty hosts and host segments of INDIGO_NAME_SIZE or more before copying or resolving; accept INDIGO_NAME_SIZE-1 without truncation, with or without explicit port.
AC-2: Explicit ports contain only ASCII decimal digits, represent 1..65535, and consume the remainder. Reject empty, signed, whitespace, overflow, zero, extra colon, path and trailing junk. Reject host whitespace, control characters, slash, query, fragment, userinfo and bracket syntax unsupported by the existing parser.
AC-3: Preserve tcp/udp scheme selection and custom scheme with ASCII letter first and subsequent ASCII letters/digits/+/./- with caller protocol hint; preserve default port when no explicit port is present, validating its range before connecting. Return established failure values (-1 / NULL).

Complexity medium: two public implementations and preserving custom scheme hints. Risk high: stack corruption on external input. No new public API/dependencies, IPv6 support or driver changes.
