#include <locale.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_bus.h>
static bool astromechanics_measurement(const char *text, double *value) {
			const char *cursor = text;
			while (isspace((unsigned char)*cursor)) {
				cursor++;
			}
			if (*cursor == '+' || *cursor == '-') {
				cursor++;
			}
			bool digits = false;
			while (isdigit((unsigned char)*cursor)) {
				digits = true;
				cursor++;
			}
			if (*cursor == '.' || *cursor == ',') {
				cursor++;
				while (isdigit((unsigned char)*cursor)) {
					digits = true;
					cursor++;
				}
			}
			if (!digits) {
				return false;
			}
			if (*cursor == 'e' || *cursor == 'E') {
				cursor++;
				if (*cursor == '+' || *cursor == '-') {
					cursor++;
				}
				if (!isdigit((unsigned char)*cursor)) {
					return false;
				}
				unsigned exponent = 0;
				while (isdigit((unsigned char)*cursor)) {
					exponent = exponent * 10 + *cursor++ - '0';
					if (exponent > 308) {
						return false;
					}
				}
			}
			while (isspace((unsigned char)*cursor)) {
				cursor++;
			}
			if (*cursor) {
				return false;
			}
			*value = indigo_atod(text);
			return isfinite(*value);
		}
int main(void) { assert(setlocale(LC_NUMERIC, "de_DE.UTF-8")); double v; assert(astromechanics_measurement("20.3", &v) && fabs(v-20.3)<.001); assert(astromechanics_measurement("20,3", &v) && fabs(v-20.3)<.001); assert(!astromechanics_measurement("20.3junk", &v)); return 0; }
