// Copyright (c) 2023 Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

#include <stdio.h>
#include <indigo/indigo_service_discovery.h>

/* This function will be called every time a service is resolved by indigo_resolve_service() */
void resolve_callback(const char *service_name, uint32_t interface_index, const char *host, int port) {
	if (host != NULL) {
		printf("= %s -> %s:%u (interface %d)\n", service_name, host, port, interface_index);
	} else {
		fprintf(stderr, "! %s -> service can not be resolved\n", service_name);
	}
}

/* This function will be called every time a service is discovered, removed or at the end of the record */
void discover_callback(indigo_service_discovery_event event, const char *service_name, uint32_t interface_index) {
	switch (event) {
		case INDIGO_SERVICE_ADDED:
			printf("+ %s (interface %d)\n", service_name, interface_index);
			/* we have a new indigo service discovered, we need to resolve it
			   to get the hostname and the port of the service
			*/
			indigo_resolve_service(service_name, interface_index, resolve_callback);
			break;
		case INDIGO_SERVICE_REMOVED:
			/* service is gone indigo_server providing it is stopped */
			printf("- %s (interface %d)\n", service_name, interface_index);
			break;
		case INDIGO_SERVICE_END_OF_RECORD:
			/* these are all the sevises available at the moment,
			   but we will continue listening for updates
			*/
			printf("___end___\n");
			break;
		default:
			break;
	}
}

int main() {
	/* start service browser, the discover_callback() will be called
	   everytime a service is discoveres, removed or end of the record occured
	*/
	indigo_start_service_browser(discover_callback);

	/* give it 10 seconds to work, in a real life
	   it may work while the application is working
	   to get imediate updates on the available services.
	*/
	indigo_usleep(10 * ONE_SECOND_DELAY);

	/* stop the service browser and cleanuo the memory it used,
	   as we do not need it any more
	*/
	indigo_stop_service_browser();
	return 0;
}
