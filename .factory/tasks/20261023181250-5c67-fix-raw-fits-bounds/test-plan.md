# Test plan

AC-001: short heap buffers of every length below header, negative/null args, unsupported signature, zero/huge dimensions, one-byte-short payload for all four formats.
AC-002: exactly 36 cards and 37 cards, multiple records of caller/embedded/combined metadata; assert END position, padding, intact pixel planes and size.
AC-003: RGB extension starts after all components; non-NUL bounded extension, short field, oversized value/card, unterminated quote and missing semicolon.
AC-004: exact mono8/mono16/RGB24/RGB48 bytes, odd input address, retained preexisting output on rejection; allocator fault injection if feasible without production hooks. ASan/UBSan runs exercise actual production object, not included .c source.
