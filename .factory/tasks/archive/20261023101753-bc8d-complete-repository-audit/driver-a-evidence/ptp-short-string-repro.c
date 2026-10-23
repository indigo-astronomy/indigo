#include <stdint.h>
#include <stdlib.h>
uint8_t *ptp_decode_string(uint8_t *source, char *target) {
	uint8_t length = *source++;
	for (int i = 0; i < length; i++) {
		*target++ = *source++;
		source++;
	}
	return source;
}

int main(void) { uint8_t *packet = malloc(1); char out[256]; packet[0] = 3; ptp_decode_string(packet, out); free(packet); return 0; }
