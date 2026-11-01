#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <indigo/indigo_json.h>
static double captured;
static int changes;
indigo_result audit_capture_change(indigo_client *client, indigo_property *property) {
	if (property->type == INDIGO_NUMBER_VECTOR && property->count == 1) {
		captured = property->items[0].number.value;
		changes++;
	}
	return INDIGO_OK;
}
static int run(const char *token, double expected) {
	int sockets[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets)) {
		return 2;
	}
	char request[512];
	int size = snprintf(request, sizeof(request), "{\"newNumberVector\":{\"device\":\"Audit\",\"name\":\"VALUE\",\"items\":[{\"name\":\"NUMBER\",\"value\":%s}]}}\n", token);
	write(sockets[1], request, size);
	shutdown(sockets[1], SHUT_WR);
	indigo_uni_handle handle = { .type = INDIGO_TCP_HANDLE, .fd = sockets[0], .log_level = 0 };
	indigo_adapter_context context = { .input = &handle, .output = &handle, .web_socket = false };
	indigo_client client = { .version = INDIGO_VERSION_CURRENT, .client_context = &context };
	captured = -999;
	changes = 0;
	indigo_json_parse(NULL, &client);
	printf("token=%s expected=%.17g actual=%.17g changes=%d\n", token, expected, captured, changes);
	close(sockets[0]);
	close(sockets[1]);
	return changes == 1 && captured == expected ? 0 : 1;
}
int main(void) {
	int ordinary = run("100", 100);
	int negative = run("1e-2", 0.01);
	int positive = run("1e+2", 100);
	printf("control=%s negative_exponent=%s positive_exponent=%s\n", ordinary ? "FAIL" : "PASS", negative ? "FAIL" : "PASS", positive ? "FAIL" : "PASS");
	return ordinary || negative ? 2 : positive ? 0 : 1;
}
