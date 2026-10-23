/* Task-local allocator fault probe; compile only the production object with -Drealloc=fits_test_realloc. */
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_fits.h>
static int allocation_budget;
void *fits_test_realloc(void *pointer, size_t size) {
	if (allocation_budget-- == 0) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(pointer, size);
}
int main(void) {
	struct { indigo_raw_header header; unsigned char pixel; } raw = { { INDIGO_RAW_MONO8, 1, 1 }, 42 };
	indigo_fits_keyword keywords[101] = { 0 };
	for (int i = 0; i < 100; i++) {
		keywords[i] = (indigo_fits_keyword){ .type = INDIGO_FITS_NUMBER, .name = "EXPTIME", .number = 1 };
	}
	for (int fail_at = 0; fail_at < 6; fail_at++) {
		char *fits = malloc(32);
		memset(fits, 'Q', 32);
		char *previous = fits;
		int size = 32;
		allocation_budget = fail_at;
		indigo_result result = indigo_raw_to_fits((char *)&raw, 13, &fits, &size, keywords);
		if (fail_at < 5) {
			assert(result == INDIGO_FAILED && errno == ENOMEM);
			assert(fits == previous && size == 32);
			for (int i = 0; i < 32; i++) {
				assert(fits[i] == 'Q');
			}
		} else {
			assert(result == INDIGO_OK && size == 5 * FITS_RECORD_SIZE);
			assert(fits[4 * FITS_RECORD_SIZE] == 42);
		}
		free(fits);
	}
	return 0;
}
