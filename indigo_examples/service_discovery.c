#include <stdio.h>
#include <indigo/indigo_service_discovery.h>

void resolve_callback(const char *name, uint32_t interface_index, const char *host, int port) {
	printf("= %s -> %s:%u (interface %d)\n", name, host, port, interface_index);
}

void discover_callback(indigo_service_discovery_event event, const char *service_name, uint32_t interface_index) {
	switch (event) {
        case INDIGO_SERVICE_ADDED:
            printf("+ %s (interface %d)\n", service_name, interface_index);
		    indigo_resolve_service(service_name, interface_index, resolve_callback);
            break;
        case INDIGO_SERVICE_REMOVED:
            printf("- %s (interface %d)\n", service_name, interface_index);
            break;
        case INDIGO_SERVICE_END_OF_RECORD:
            printf("___end___\n");
            break;
        default:
            break;
	}
}

int main() {
    indigo_start_service_browser(discover_callback);
	indigo_usleep(10 * ONE_SECOND_DELAY);
	indigo_stop_service_browser();
    return 0;
}