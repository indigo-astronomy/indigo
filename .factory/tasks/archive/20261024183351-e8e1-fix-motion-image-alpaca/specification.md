# Specification

AC-001: Each affected focuser and shared Lunatico rotator reports ALERT when commanded motion becomes idle before its issued physical target; retain the requested target and respect existing backlash, reversal, quantization, abort and external-motion contracts. Alpaca absolute focuser completion semantics are excluded.
AC-002: Imager RAW processing, Alpaca image-array conversion and platesolver preview reject incomplete headers, unsupported signatures, zero or unsafe dimensions and truncated pixel spans before accessing pixels or allocating from unchecked shape. Preserve valid supported formats and trailing extension bytes.
AC-003: Alpaca Bayer mapping never creates an item beyond ALPACA_MAX_ITEMS; preserve existing mappings and safe null state when capacity prevents a mapping.
AC-004: Meaningful regressions demonstrate each repaired failure and preserved success behavior; affected driver versions increase, generated outputs remain reproducible, and findings close only after verification.

Complexity high: asynchronous hardware motion and protocol-specific expected positions, multiple image trust boundaries and cached device lifetimes. Risk medium: incorrect motion outcome or image memory access. No material unresolved product question. Hardware and Linux/Windows execution are unavailable; synthetic tests must identify this limitation.
