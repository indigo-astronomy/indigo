# Findings and supporting evidence

## FND-20261101183631-1a1e-solver-exclusive-init-001 — Recycled loader slots retain initialized state

Preexisting functional issue in indigo_libs/indigo_client.c: remove_driver clears description/name/driver/handle but leaves initialized, and add_driver(init=false) writes a new name into the slot without resetting initialized. Reproduce: register and initialize a synthetic driver, remove it, then register another with init=false. The second INIT is never called, but indigo_driver_initialized(second_name) returns true. Impact: custom hosts can misreport inactive registration or falsely refuse a conflicting driver. Normal fresh server static discovery and dynamic init=true are unaffected; independent review confirmed this is nonblocking for the approved two-agent server contract. Owner/coordinator must assign a separate loader repair and regression; no risk acceptance or closure is asserted.

Working directory: /Users/polakovic/Development/indigo. Source baseline e5e77e58edd20f102577a8f931651f802bdfc612, macOS arm64. Test developer executed:

```sh
clang -DINDIGO_MACOS -Iindigo_libs -Ibuild/include /tmp/indigo-recycled-slot-reproducer.c -Lbuild/lib -lindigo -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -o /tmp/indigo-recycled-slot-reproducer
/tmp/indigo-recycled-slot-reproducer
```

Compile exit0; reproducer exit1 with `second INIT calls=0, registry initialized=true`. Reproducer retained inline:

```c
#include <stdio.h>
#include <string.h>
#include <indigo/indigo_client.h>

static int second_init_calls;

static indigo_result first_driver(indigo_driver_action action, indigo_driver_info *info) {
	if (info) {
		memset(info, 0, sizeof(*info));
		strcpy(info->name, "recycled_first");
		strcpy(info->description, "first synthetic lifecycle entry");
	}
	return INDIGO_OK;
}

static indigo_result second_driver(indigo_driver_action action, indigo_driver_info *info) {
	if (info) {
		memset(info, 0, sizeof(*info));
		strcpy(info->name, "recycled_second");
		strcpy(info->description, "second synthetic lifecycle entry");
	}
	if (action == INDIGO_DRIVER_INIT) {
		second_init_calls++;
	}
	return INDIGO_OK;
}

int main(void) {
	indigo_driver_entry *first = NULL;
	indigo_driver_entry *second = NULL;
	if (indigo_add_driver(first_driver, true, &first) != INDIGO_OK || indigo_remove_driver(first) != INDIGO_OK || indigo_add_driver(second_driver, false, &second) != INDIGO_OK) {
		return 2;
	}
	bool stale = indigo_driver_initialized("recycled_second");
	printf("second INIT calls=%d, registry initialized=%s\n", second_init_calls, stale ? "true" : "false");
	indigo_remove_driver(second);
	return stale && second_init_calls == 0 ? 1 : 0;
}
```

## FND-20261101183631-1a1e-solver-exclusive-init-002 — Sanitizer fixture issue
The full Solver ASan/UBSan run initially passed25/26. Existing unsupported_image_rejected creates bytes by converting float values up to255 to signed char; UBSan flags values above127 as unrepresentable. The byte fixture should be unsigned char, preserving all assertions and rejection behavior. Changed the fixture buffer and cast to unsigned char, preserving its arbitrary-byte distribution and all assertions. Full ASan/UBSan rerun passed26/26, including the unchanged invalid-image rejection case; final ordinary suite also passed26/26. Logs /tmp/solver-exclusive-final-sanitize.log and /tmp/solver-exclusive-final-solver.log. This finding is fixed and verified. No production issue is implied; final disposition will be retained after verification.
