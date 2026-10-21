// Isolated audit reproducer of the exact buffer/copy/append operations in
// indigo_libs/indigo_server_tcp.c at revision 48dc893491f77cddb18fc3188d400a70f67cde19.
// No server is started and no network or device is contacted.
#include <string.h>
#include <stdio.h>
int main(int argc, char **argv) {
	char header[1024] = "Sec-WebSocket-Key: ";
	unsigned count = argc > 1 ? 255 : 24;
	memset(header + 19, 'A', count);
	header[19 + count] = 0;
	char websocket_key[256] = "";
	strncpy(websocket_key, header + 19, sizeof(websocket_key) - 1);
	websocket_key[sizeof(websocket_key) - 1] = '\0';
	strcat(websocket_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
	puts(websocket_key);
	return 0;
}
