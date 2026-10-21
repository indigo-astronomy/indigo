// Isolated audit reproducer of the bounded key handling in indigo_libs/indigo_server_tcp.c after the
// FND-000045 fix. Same inputs as 003-initial-repository-audit/repro-websocket-key.c. No server is
// started and no network or device is contacted.
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
int main(int argc, char **argv) {
	char header[1024] = "Sec-WebSocket-Key: ";
	unsigned count = argc > 1 ? 255 : 24;
	memset(header + 19, 'A', count);
	header[19 + count] = 0;
	char websocket_key[256] = "";
	bool websocket_key_too_long = strlen(header + 19) >= sizeof(websocket_key) - strlen(WEBSOCKET_GUID);
	if (websocket_key_too_long) {
		puts("refused: 400 Bad Request");
		return 0;
	}
	strcpy(websocket_key, header + 19);
	strcat(websocket_key, WEBSOCKET_GUID);
	puts(websocket_key);
	return 0;
}
