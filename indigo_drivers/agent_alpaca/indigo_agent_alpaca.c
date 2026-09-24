// Copyright (c) 2021-2026 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO ASCOM ALPACA bridge agent
 \file indigo_agent_alpaca.c
 */

#define DRIVER_VERSION 0x0300000A
#define DRIVER_NAME	"indigo_agent_alpaca"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <limits.h>

#if defined(INDIGO_MACOS) || defined(INDIGO_LINUX)
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif defined(INDIGO_WINDOWS)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_server_tcp.h>

#include "indigo_agent_alpaca.h"
#include "indigo_alpaca_common.h"

#define PRIVATE_DATA													private_data

#define AGENT_DISCOVERY_PROPERTY							(PRIVATE_DATA->discovery_property)
#define AGENT_DISCOVERY_PORT_ITEM							(AGENT_DISCOVERY_PROPERTY->items+0)

#define AGENT_CAMERA_BAYERPAT_PROPERTY						(PRIVATE_DATA->camera_bayerpat_property)

#define AGENT_DEVICES_PROPERTY								(PRIVATE_DATA->devices_property)

#define DISCOVERY_REQUEST											"alpacadiscovery1"
#define DISCOVERY_RESPONSE										"{ \"AlpacaPort\":%d }"

typedef struct {
	indigo_property *discovery_property;
	indigo_property *devices_property;
	indigo_property *camera_bayerpat_property;
	indigo_timer *discovery_server_timer;
	pthread_mutex_t mutex;
} alpaca_agent_private_data;

static alpaca_agent_private_data *private_data = NULL;

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#define INVALID_SOCKET -1
static int discovery_server_socket = INVALID_SOCKET;
#elif defined(INDIGO_WINDOWS)
static SOCKET discovery_server_socket = INVALID_SOCKET;
#endif

static indigo_alpaca_device *alpaca_devices = NULL;
static uint32_t server_transaction_id = 1;

indigo_device *indigo_agent_alpaca_device = NULL;
indigo_client *indigo_agent_alpaca_client = NULL;

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		pthread_mutex_lock(&private_data->mutex);
		indigo_save_property(device, NULL, AGENT_DEVICES_PROPERTY);
		indigo_save_property(device, NULL, AGENT_CAMERA_BAYERPAT_PROPERTY);
		if (DEVICE_CONTEXT->property_save_file_handle != NULL) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			indigo_uni_close(&DEVICE_CONTEXT->property_save_file_handle);
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
		pthread_mutex_unlock(&private_data->mutex);
	}
}

// -------------------------------------------------------------------------------- ALPACA bridge implementation

#if defined(INDIGO_MACOS) || defined(INDIGO_LINUX)
#define LAST_ERROR	strerror(errno)
#elif defined(INDIGO_WINDOWS)
#define LAST_ERROR indigo_last_wsa_error()
#endif

static void start_discovery_server(indigo_device *device) {
	int port = (int)AGENT_DISCOVERY_PORT_ITEM->number.value;
	discovery_server_socket = socket(PF_INET, SOCK_DGRAM, 0);
	if (discovery_server_socket == INVALID_SOCKET) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create socket (%s)", LAST_ERROR);
		return;
	}
	int reuse = 1;
	if (setsockopt(discovery_server_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) < 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		close(discovery_server_socket);
#elif defined(INDIGO_WINDOWS)
		closesocket(discovery_server_socket);
#endif
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "setsockopt() failed (%s)", LAST_ERROR);
		return;
	}
	struct sockaddr_in server_address;
	unsigned int server_address_length = sizeof(server_address);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(discovery_server_socket, (struct sockaddr *)&server_address, server_address_length) < 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		close(discovery_server_socket);
#elif defined(INDIGO_WINDOWS)
		closesocket(discovery_server_socket);
#endif
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "bind() failed (%s)", LAST_ERROR);
		return;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Discovery server started on port %d", port);
	fd_set readfd;
	struct sockaddr_in client_address;
	unsigned int client_address_length = sizeof(client_address);
	char buffer[128];
	struct timeval tv;
	while (discovery_server_socket != INVALID_SOCKET) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&readfd);
		FD_SET(discovery_server_socket, &readfd);
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		int ret = select(discovery_server_socket + 1, &readfd, NULL, NULL, &tv);
#elif defined(INDIGO_WINDOWS)
		int ret = select(0, &readfd, NULL, NULL, &tv);
#endif
		if (ret > 0) {
			if (FD_ISSET(discovery_server_socket, &readfd)) {
				recvfrom(discovery_server_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_address, &client_address_length);
				if (strstr(buffer, DISCOVERY_REQUEST)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Discovery request from %s", inet_ntoa(client_address.sin_addr));
					sprintf(buffer, DISCOVERY_RESPONSE, indigo_server_tcp_port);
					sendto(discovery_server_socket, buffer, (int)strlen(buffer), 0, (struct sockaddr*)&client_address, client_address_length);
				}
			}
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Discovery server stopped on port %d", port);
	return;
}

static void shutdown_discovery_server() {
	if (discovery_server_socket > 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		shutdown(discovery_server_socket, SHUT_RDWR);
		close(discovery_server_socket);
#elif defined(INDIGO_WINDOWS)
		shutdown(discovery_server_socket, SD_BOTH);
		closesocket(discovery_server_socket);
#endif
		discovery_server_socket = INVALID_SOCKET;
	}
}

#define ALPACA_MAX_PARAMS		16

typedef enum {
	ALPACA_NONE = 0,
	ALPACA_BOOL,
	ALPACA_INT,
	ALPACA_DOUBLE,
	ALPACA_STRING
} alpaca_param_type;

typedef struct {
	const char *name;
	alpaca_param_type type;
} alpaca_param_spec;

typedef struct {
	const char *name;
	bool get;
	bool put;
	alpaca_param_spec get_params[2];
	alpaca_param_spec put_params[2];
} alpaca_member_spec;

typedef struct {
	char key[64];
	char value[INDIGO_VALUE_SIZE];
} alpaca_param;

typedef struct {
	uint32_t client_id;
	uint32_t client_transaction_id;
	int count;
	alpaca_param params[ALPACA_MAX_PARAMS];
} alpaca_request;

#define ALPACA_GET(name)																{ name, true, false }
#define ALPACA_PUT(name)																{ name, false, true }
#define ALPACA_GET_1(name, p1, t1)											{ name, true, false, { { p1, t1 } } }
#define ALPACA_GET_2(name, p1, t1, p2, t2)							{ name, true, false, { { p1, t1 }, { p2, t2 } } }
#define ALPACA_PUT_1(name, p1, t1)											{ name, false, true, { { NULL } }, { { p1, t1 } } }
#define ALPACA_PUT_2(name, p1, t1, p2, t2)							{ name, false, true, { { NULL } }, { { p1, t1 }, { p2, t2 } } }
#define ALPACA_GET_PUT_1(name, p1, t1)									{ name, true, true, { { NULL } }, { { p1, t1 } } }

// Alpaca device API members as defined in https://ascom-standards.org/api (AlpacaDeviceAPI_v1.yaml, Platform 7).
// PUT parameter names are case sensitive, GET parameter names are not.

static alpaca_member_spec alpaca_common_members[] = {
	ALPACA_PUT_2("action", "Action", ALPACA_STRING, "Parameters", ALPACA_STRING),
	ALPACA_PUT_2("commandblind", "Command", ALPACA_STRING, "Raw", ALPACA_BOOL),
	ALPACA_PUT_2("commandbool", "Command", ALPACA_STRING, "Raw", ALPACA_BOOL),
	ALPACA_PUT_2("commandstring", "Command", ALPACA_STRING, "Raw", ALPACA_BOOL),
	ALPACA_GET_PUT_1("connected", "Connected", ALPACA_BOOL),
	ALPACA_PUT("connect"),
	ALPACA_PUT("disconnect"),
	ALPACA_GET("connecting"),
	ALPACA_GET("devicestate"),
	ALPACA_GET("description"),
	ALPACA_GET("driverinfo"),
	ALPACA_GET("driverversion"),
	ALPACA_GET("interfaceversion"),
	ALPACA_GET("name"),
	ALPACA_GET("supportedactions"),
	{ NULL }
};

static alpaca_member_spec alpaca_camera_members[] = {
	ALPACA_GET("bayeroffsetx"),
	ALPACA_GET("bayeroffsety"),
	ALPACA_GET_PUT_1("binx", "BinX", ALPACA_INT),
	ALPACA_GET_PUT_1("biny", "BinY", ALPACA_INT),
	ALPACA_GET("camerastate"),
	ALPACA_GET("cameraxsize"),
	ALPACA_GET("cameraysize"),
	ALPACA_GET("canabortexposure"),
	ALPACA_GET("canasymmetricbin"),
	ALPACA_GET("canfastreadout"),
	ALPACA_GET("cangetcoolerpower"),
	ALPACA_GET("canpulseguide"),
	ALPACA_GET("cansetccdtemperature"),
	ALPACA_GET("canstopexposure"),
	ALPACA_GET("ccdtemperature"),
	ALPACA_GET_PUT_1("cooleron", "CoolerOn", ALPACA_BOOL),
	ALPACA_GET("coolerpower"),
	ALPACA_GET("electronsperadu"),
	ALPACA_GET("exposuremax"),
	ALPACA_GET("exposuremin"),
	ALPACA_GET("exposureresolution"),
	ALPACA_GET_PUT_1("fastreadout", "FastReadout", ALPACA_BOOL),
	ALPACA_GET("fullwellcapacity"),
	ALPACA_GET_PUT_1("gain", "Gain", ALPACA_INT),
	ALPACA_GET("gainmax"),
	ALPACA_GET("gainmin"),
	ALPACA_GET("gains"),
	ALPACA_GET("hasshutter"),
	ALPACA_GET("heatsinktemperature"),
	ALPACA_GET("imagearray"),
	ALPACA_GET("imagearrayvariant"),
	ALPACA_GET("imageready"),
	ALPACA_GET("ispulseguiding"),
	ALPACA_GET("lastexposureduration"),
	ALPACA_GET("lastexposurestarttime"),
	ALPACA_GET("maxadu"),
	ALPACA_GET("maxbinx"),
	ALPACA_GET("maxbiny"),
	ALPACA_GET_PUT_1("numx", "NumX", ALPACA_INT),
	ALPACA_GET_PUT_1("numy", "NumY", ALPACA_INT),
	ALPACA_GET_PUT_1("offset", "Offset", ALPACA_INT),
	ALPACA_GET("offsetmax"),
	ALPACA_GET("offsetmin"),
	ALPACA_GET("offsets"),
	ALPACA_GET("percentcompleted"),
	ALPACA_GET("pixelsizex"),
	ALPACA_GET("pixelsizey"),
	ALPACA_GET_PUT_1("readoutmode", "ReadoutMode", ALPACA_INT),
	ALPACA_GET("readoutmodes"),
	ALPACA_GET("sensorname"),
	ALPACA_GET("sensortype"),
	ALPACA_GET_PUT_1("setccdtemperature", "SetCCDTemperature", ALPACA_DOUBLE),
	ALPACA_GET_PUT_1("startx", "StartX", ALPACA_INT),
	ALPACA_GET_PUT_1("starty", "StartY", ALPACA_INT),
	ALPACA_GET_PUT_1("subexposureduration", "SubExposureDuration", ALPACA_DOUBLE),
	ALPACA_PUT("abortexposure"),
	ALPACA_PUT_2("pulseguide", "Direction", ALPACA_INT, "Duration", ALPACA_INT),
	ALPACA_PUT_2("startexposure", "Duration", ALPACA_DOUBLE, "Light", ALPACA_BOOL),
	ALPACA_PUT("stopexposure"),
	{ NULL }
};

static alpaca_member_spec alpaca_covercalibrator_members[] = {
	ALPACA_GET("brightness"),
	ALPACA_GET("calibratorchanging"),
	ALPACA_GET("calibratorstate"),
	ALPACA_GET("covermoving"),
	ALPACA_GET("coverstate"),
	ALPACA_GET("maxbrightness"),
	ALPACA_PUT("calibratoroff"),
	ALPACA_PUT_1("calibratoron", "Brightness", ALPACA_INT),
	ALPACA_PUT("closecover"),
	ALPACA_PUT("haltcover"),
	ALPACA_PUT("opencover"),
	{ NULL }
};

static alpaca_member_spec alpaca_dome_members[] = {
	ALPACA_GET("altitude"),
	ALPACA_GET("athome"),
	ALPACA_GET("atpark"),
	ALPACA_GET("azimuth"),
	ALPACA_GET("canfindhome"),
	ALPACA_GET("canpark"),
	ALPACA_GET("cansetaltitude"),
	ALPACA_GET("cansetazimuth"),
	ALPACA_GET("cansetpark"),
	ALPACA_GET("cansetshutter"),
	ALPACA_GET("canslave"),
	ALPACA_GET("cansyncazimuth"),
	ALPACA_GET("shutterstatus"),
	ALPACA_GET_PUT_1("slaved", "Slaved", ALPACA_BOOL),
	ALPACA_GET("slewing"),
	ALPACA_PUT("abortslew"),
	ALPACA_PUT("closeshutter"),
	ALPACA_PUT("findhome"),
	ALPACA_PUT("openshutter"),
	ALPACA_PUT("park"),
	ALPACA_PUT("setpark"),
	ALPACA_PUT_1("slewtoaltitude", "Altitude", ALPACA_DOUBLE),
	ALPACA_PUT_1("slewtoazimuth", "Azimuth", ALPACA_DOUBLE),
	ALPACA_PUT_1("synctoazimuth", "Azimuth", ALPACA_DOUBLE),
	{ NULL }
};

static alpaca_member_spec alpaca_filterwheel_members[] = {
	ALPACA_GET("focusoffsets"),
	ALPACA_GET("names"),
	ALPACA_GET_PUT_1("position", "Position", ALPACA_INT),
	{ NULL }
};

static alpaca_member_spec alpaca_focuser_members[] = {
	ALPACA_GET("absolute"),
	ALPACA_GET("ismoving"),
	ALPACA_GET("maxincrement"),
	ALPACA_GET("maxstep"),
	ALPACA_GET("position"),
	ALPACA_GET("stepsize"),
	ALPACA_GET_PUT_1("tempcomp", "TempComp", ALPACA_BOOL),
	ALPACA_GET("tempcompavailable"),
	ALPACA_GET("temperature"),
	ALPACA_PUT("halt"),
	ALPACA_PUT_1("move", "Position", ALPACA_INT),
	{ NULL }
};

static alpaca_member_spec alpaca_rotator_members[] = {
	ALPACA_GET("canreverse"),
	ALPACA_GET("ismoving"),
	ALPACA_GET("mechanicalposition"),
	ALPACA_GET("position"),
	ALPACA_GET_PUT_1("reverse", "Reverse", ALPACA_BOOL),
	ALPACA_GET("stepsize"),
	ALPACA_GET("targetposition"),
	ALPACA_PUT("halt"),
	ALPACA_PUT_1("move", "Position", ALPACA_DOUBLE),
	ALPACA_PUT_1("moveabsolute", "Position", ALPACA_DOUBLE),
	ALPACA_PUT_1("movemechanical", "Position", ALPACA_DOUBLE),
	ALPACA_PUT_1("sync", "Position", ALPACA_DOUBLE),
	{ NULL }
};

static alpaca_member_spec alpaca_switch_members[] = {
	ALPACA_GET("maxswitch"),
	ALPACA_GET_1("canasync", "Id", ALPACA_INT),
	ALPACA_GET_1("canwrite", "Id", ALPACA_INT),
	ALPACA_GET_1("getswitch", "Id", ALPACA_INT),
	ALPACA_GET_1("getswitchdescription", "Id", ALPACA_INT),
	ALPACA_GET_1("getswitchname", "Id", ALPACA_INT),
	ALPACA_GET_1("getswitchvalue", "Id", ALPACA_INT),
	ALPACA_GET_1("maxswitchvalue", "Id", ALPACA_INT),
	ALPACA_GET_1("minswitchvalue", "Id", ALPACA_INT),
	ALPACA_GET_1("statechangecomplete", "Id", ALPACA_INT),
	ALPACA_GET_1("switchstep", "Id", ALPACA_INT),
	ALPACA_PUT_1("cancelasync", "Id", ALPACA_INT),
	ALPACA_PUT_2("setasync", "Id", ALPACA_INT, "State", ALPACA_BOOL),
	ALPACA_PUT_2("setasyncvalue", "Id", ALPACA_INT, "Value", ALPACA_DOUBLE),
	ALPACA_PUT_2("setswitch", "Id", ALPACA_INT, "State", ALPACA_BOOL),
	ALPACA_PUT_2("setswitchname", "Id", ALPACA_INT, "Name", ALPACA_STRING),
	ALPACA_PUT_2("setswitchvalue", "Id", ALPACA_INT, "Value", ALPACA_DOUBLE),
	{ NULL }
};

static alpaca_member_spec alpaca_telescope_members[] = {
	ALPACA_GET("alignmentmode"),
	ALPACA_GET("altitude"),
	ALPACA_GET("aperturearea"),
	ALPACA_GET("aperturediameter"),
	ALPACA_GET("athome"),
	ALPACA_GET("atpark"),
	ALPACA_GET("azimuth"),
	ALPACA_GET("canfindhome"),
	ALPACA_GET("canpark"),
	ALPACA_GET("canpulseguide"),
	ALPACA_GET("cansetdeclinationrate"),
	ALPACA_GET("cansetguiderates"),
	ALPACA_GET("cansetpark"),
	ALPACA_GET("cansetpierside"),
	ALPACA_GET("cansetrightascensionrate"),
	ALPACA_GET("cansettracking"),
	ALPACA_GET("canslew"),
	ALPACA_GET("canslewaltaz"),
	ALPACA_GET("canslewaltazasync"),
	ALPACA_GET("canslewasync"),
	ALPACA_GET("cansync"),
	ALPACA_GET("cansyncaltaz"),
	ALPACA_GET("canunpark"),
	ALPACA_GET("declination"),
	ALPACA_GET_PUT_1("declinationrate", "DeclinationRate", ALPACA_DOUBLE),
	ALPACA_GET_PUT_1("doesrefraction", "DoesRefraction", ALPACA_BOOL),
	ALPACA_GET("equatorialsystem"),
	ALPACA_GET("focallength"),
	ALPACA_GET_PUT_1("guideratedeclination", "GuideRateDeclination", ALPACA_DOUBLE),
	ALPACA_GET_PUT_1("guideraterightascension", "GuideRateRightAscension", ALPACA_DOUBLE),
	ALPACA_GET("ispulseguiding"),
	ALPACA_GET("rightascension"),
	ALPACA_GET_PUT_1("rightascensionrate", "RightAscensionRate", ALPACA_DOUBLE),
	ALPACA_GET_PUT_1("sideofpier", "SideOfPier", ALPACA_INT),
	ALPACA_GET("siderealtime"),
	ALPACA_GET_PUT_1("siteelevation", "SiteElevation", ALPACA_DOUBLE),
	ALPACA_GET_PUT_1("sitelatitude", "SiteLatitude", ALPACA_DOUBLE),
	ALPACA_GET_PUT_1("sitelongitude", "SiteLongitude", ALPACA_DOUBLE),
	ALPACA_GET("slewing"),
	ALPACA_GET_PUT_1("slewsettletime", "SlewSettleTime", ALPACA_INT),
	ALPACA_GET_PUT_1("targetdeclination", "TargetDeclination", ALPACA_DOUBLE),
	ALPACA_GET_PUT_1("targetrightascension", "TargetRightAscension", ALPACA_DOUBLE),
	ALPACA_GET_PUT_1("tracking", "Tracking", ALPACA_BOOL),
	ALPACA_GET_PUT_1("trackingrate", "TrackingRate", ALPACA_INT),
	ALPACA_GET("trackingrates"),
	ALPACA_GET_PUT_1("utcdate", "UTCDate", ALPACA_STRING),
	ALPACA_GET_1("axisrates", "Axis", ALPACA_INT),
	ALPACA_GET_1("canmoveaxis", "Axis", ALPACA_INT),
	ALPACA_GET_2("destinationsideofpier", "RightAscension", ALPACA_DOUBLE, "Declination", ALPACA_DOUBLE),
	ALPACA_PUT("abortslew"),
	ALPACA_PUT("findhome"),
	ALPACA_PUT_2("moveaxis", "Axis", ALPACA_INT, "Rate", ALPACA_DOUBLE),
	ALPACA_PUT("park"),
	ALPACA_PUT_2("pulseguide", "Direction", ALPACA_INT, "Duration", ALPACA_INT),
	ALPACA_PUT("setpark"),
	ALPACA_PUT_2("slewtoaltaz", "Azimuth", ALPACA_DOUBLE, "Altitude", ALPACA_DOUBLE),
	ALPACA_PUT_2("slewtoaltazasync", "Azimuth", ALPACA_DOUBLE, "Altitude", ALPACA_DOUBLE),
	ALPACA_PUT_2("slewtocoordinates", "RightAscension", ALPACA_DOUBLE, "Declination", ALPACA_DOUBLE),
	ALPACA_PUT_2("slewtocoordinatesasync", "RightAscension", ALPACA_DOUBLE, "Declination", ALPACA_DOUBLE),
	ALPACA_PUT("slewtotarget"),
	ALPACA_PUT("slewtotargetasync"),
	ALPACA_PUT_2("synctoaltaz", "Azimuth", ALPACA_DOUBLE, "Altitude", ALPACA_DOUBLE),
	ALPACA_PUT_2("synctocoordinates", "RightAscension", ALPACA_DOUBLE, "Declination", ALPACA_DOUBLE),
	ALPACA_PUT("synctotarget"),
	ALPACA_PUT("unpark"),
	{ NULL }
};

static struct {
	const char *device_type;
	alpaca_member_spec *members;
} alpaca_device_members[] = {
	{ "camera", alpaca_camera_members },
	{ "covercalibrator", alpaca_covercalibrator_members },
	{ "dome", alpaca_dome_members },
	{ "filterwheel", alpaca_filterwheel_members },
	{ "focuser", alpaca_focuser_members },
	{ "rotator", alpaca_rotator_members },
	{ "switch", alpaca_switch_members },
	{ "telescope", alpaca_telescope_members },
	{ NULL }
};

static alpaca_member_spec *find_member(const char *device_type, const char *name) {
	for (alpaca_member_spec *member = alpaca_common_members; member->name; member++) {
		if (!strcmp(member->name, name)) {
			return member;
		}
	}
	for (int i = 0; alpaca_device_members[i].device_type; i++) {
		if (!strcmp(alpaca_device_members[i].device_type, device_type)) {
			for (alpaca_member_spec *member = alpaca_device_members[i].members; member->name; member++) {
				if (!strcmp(member->name, name)) {
					return member;
				}
			}
			break;
		}
	}
	return NULL;
}

static int hex_digit(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

static void url_decode(char *target, long target_size, const char *source, long source_length) {
	long j = 0;
	for (long i = 0; i < source_length && j < target_size - 1; i++) {
		char c = source[i];
		if (c == '+') {
			c = ' ';
		} else if (c == '%' && i + 2 < source_length && hex_digit(source[i + 1]) >= 0 && hex_digit(source[i + 2]) >= 0) {
			c = (char)(hex_digit(source[i + 1]) * 16 + hex_digit(source[i + 2]));
			i += 2;
		}
		target[j++] = c;
	}
	target[j] = 0;
}

// ClientID and ClientTransactionID are uint32; anything else (empty, negative, non-numeric, too large) is treated as not supplied.
static uint32_t parse_transaction_id(const char *value) {
	if (*value < '0' || *value > '9') {
		return 0;
	}
	char *end;
	unsigned long long result = strtoull(value, &end, 10);
	if (*end || result > UINT32_MAX) {
		return 0;
	}
	return (uint32_t)result;
}

// GET parameter names are case insensitive, PUT (form) parameter names are case sensitive and wrongly cased ClientID / ClientTransactionID are ignored.
static void parse_params(const char *text, alpaca_request *request, bool case_sensitive) {
	memset(request, 0, sizeof(alpaca_request));
	if (text == NULL) {
		return;
	}
	const char *pnt = text;
	while (*pnt) {
		const char *end = strchr(pnt, '&');
		long length = end ? end - pnt : (long)strlen(pnt);
		if (length > 0 && request->count < ALPACA_MAX_PARAMS) {
			alpaca_param *param = request->params + request->count;
			const char *equal = memchr(pnt, '=', length);
			long key_length = equal ? equal - pnt : length;
			url_decode(param->key, sizeof(param->key), pnt, key_length);
			if (equal) {
				url_decode(param->value, sizeof(param->value), equal + 1, length - key_length - 1);
			} else {
				*param->value = 0;
			}
			if (case_sensitive ? !strcmp(param->key, "ClientID") : !strcasecmp(param->key, "ClientID")) {
				request->client_id = parse_transaction_id(param->value);
			} else if (case_sensitive ? !strcmp(param->key, "ClientTransactionID") : !strcasecmp(param->key, "ClientTransactionID")) {
				request->client_transaction_id = parse_transaction_id(param->value);
			} else {
				request->count++;
			}
		}
		if (end == NULL) {
			break;
		}
		pnt = end + 1;
	}
}

static const char *find_param(alpaca_request *request, const char *name, bool case_sensitive) {
	for (int i = 0; i < request->count; i++) {
		if (case_sensitive ? !strcmp(request->params[i].key, name) : !strcasecmp(request->params[i].key, name)) {
			return request->params[i].value;
		}
	}
	return NULL;
}

static bool validate_param(const char *value, alpaca_param_type type) {
	char *end;
	switch (type) {
		case ALPACA_BOOL:
			return !strcasecmp(value, "true") || !strcasecmp(value, "false");
		case ALPACA_INT: {
			if (*value == 0) {
				return false;
			}
			errno = 0;
			long result = strtol(value, &end, 10);
			return *end == 0 && errno == 0 && result >= INT_MIN && result <= INT_MAX;
		}
		case ALPACA_DOUBLE: {
			if (*value == 0) {
				return false;
			}
			double result = strtod(value, &end);
			return *end == 0 && isfinite(result);
		}
		default:
			return true;
	}
}

// Checks that all parameters required by the member are present and well formed.
static bool validate_params(alpaca_request *request, alpaca_param_spec *specs, bool case_sensitive, char *message, int message_size) {
	for (int i = 0; i < 2 && specs[i].name; i++) {
		const char *value = find_param(request, specs[i].name, case_sensitive);
		if (value == NULL) {
			snprintf(message, message_size, "Missing parameter %s", specs[i].name);
			return false;
		}
		if (!validate_param(value, specs[i].type)) {
			snprintf(message, message_size, "Invalid value of parameter %s", specs[i].name);
			return false;
		}
	}
	return true;
}

static void send_json_response(indigo_uni_handle *handle, char *path, int status_code, const char *status_text, char *body) {
	if (indigo_uni_printf(handle,
			"HTTP/1.1 %3d %s\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: %d\r\n"
			"\r\n"
			"%s", status_code, status_text, strlen(body), body
	)) {
		if (status_code == 200) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> 200 %s", path, status_text);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s -> %3d %s", path, status_code, status_text);
		}
		INDIGO_DRIVER_TRACE(DRIVER_NAME, "%s", body);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s -> Failed", path);
	}
}

static void send_text_response(indigo_uni_handle *handle, char *path, int status_code, const char *status_text, char *body) {
	if (indigo_uni_printf(handle,
			"HTTP/1.1 %3d %s\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: %d\r\n"
			"\r\n"
			"%s", status_code, status_text, strlen(body), body)) {
		if (status_code == 200) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> 200 %s", path, status_text);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s -> %3d %s", path, status_code, status_text);
		}
		INDIGO_DRIVER_TRACE(DRIVER_NAME, "%s", body);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s -> Failed", path);
	}
}

static bool alpaca_setup_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	if (indigo_uni_printf(handle,
			"HTTP/1.1 301 Moved Permanently\r\n"
			"Location: /mng.html\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 0\r\n"
			"\r\n"
	)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> OK", path);
	} else {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%s -> Failed", path);
	}
	return true;
}

static bool alpaca_apiversions_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	alpaca_request request;
	char buffer[128];
	parse_params(params, &request, false);
	snprintf(buffer, sizeof(buffer), "{ \"Value\": [ 1 ], \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", request.client_transaction_id, server_transaction_id++);
	send_json_response(handle, path, 200, "OK", buffer);
	return true;
}

static bool alpaca_v1_description_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	alpaca_request request;
	char buffer[512];
	parse_params(params, &request, false);
	snprintf(buffer, sizeof(buffer), "{ \"Value\": { \"ServerName\": \"INDIGO-Alpaca Bridge\", \"Manufacturer\": \"The INDIGO Initiative\", \"ManufacturerVersion\": \"%d.%d-%s\", \"Location\": \"\" }, \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, request.client_transaction_id, server_transaction_id++);
	send_json_response(handle, path, 200, "OK", buffer);
	return true;
}

static bool alpaca_v1_configureddevices_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	alpaca_request request;
	char *buffer = indigo_alloc_large_buffer();
	parse_params(params, &request, false);
	long index = snprintf(buffer, INDIGO_BUFFER_SIZE, "{ \"Value\": [ ");
	indigo_alpaca_device *alpaca_device = alpaca_devices;
	bool comma_needed = false;
	while (alpaca_device) {
		if (alpaca_device->device_type) {
			if (comma_needed) {
				buffer[index++] = ',';
				buffer[index++] = ' ';
			} else {
				comma_needed = true;
			}
			index += snprintf(buffer + index, INDIGO_BUFFER_SIZE - index, "{ \"DeviceName\": \"%s\", \"DeviceType\": \"%s\", \"DeviceNumber\": %d, \"UniqueID\": \"%s\" }", alpaca_device->device_name, alpaca_device->device_type, alpaca_device->device_number, alpaca_device->device_uid);
		}
		alpaca_device = alpaca_device->next;
	}
	snprintf(buffer + index, INDIGO_BUFFER_SIZE - index, "], \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", request.client_transaction_id, server_transaction_id++);
	send_json_response(handle, path, 200, "OK", buffer);
	indigo_free_large_buffer(buffer);
	return true;
}

static int string_cmp(const void * a, const void * b) {
	 return strncasecmp((char *)a, (char *)b, 128);
}

static bool alpaca_v1_api_request(indigo_uni_handle *handle, char *method, char *path, char *params, char *body) {
	char message[128];
	char *device_type = strstr(path, "/api/v1/");
	if (device_type == NULL) {
		send_text_response(handle, path, 400, "Bad Request", "Wrong API prefix");
		return true;
	}
	device_type += 8;
	char *device_number = strchr(device_type, '/');
	if (device_number == NULL) {
		send_text_response(handle, path, 400, "Bad Request", "Missing device type");
		return true;
	}
	*device_number++ = 0;
	char *command = strchr(device_number, '/');
	if (command == NULL) {
		send_text_response(handle, path, 400, "Bad Request", "Missing device number");
		return true;
	}
	*command++ = 0;
	if (*device_number == 0 || strspn(device_number, "0123456789") != strlen(device_number)) {
		send_text_response(handle, path, 400, "Bad Request", "Invalid device number");
		return true;
	}
	indigo_alpaca_device *alpaca_device = alpaca_devices;
	int number = atoi(device_number);
	while (alpaca_device) {
		if (alpaca_device->device_type && alpaca_device->device_number == number) {
			break;
		}
		alpaca_device = alpaca_device->next;
	}
	if (alpaca_device == NULL) {
		send_text_response(handle, path, 400, "Bad Request", "No such device");
		return true;
	}
	// device type, device number and member name are case sensitive and lower case
	char type[32] = { 0 };
	for (int i = 0; alpaca_device->device_type[i] && i < (int)sizeof(type) - 1; i++) {
		type[i] = (char)tolower(alpaca_device->device_type[i]);
	}
	if (strcmp(type, device_type)) {
		send_text_response(handle, path, 400, "Bad Request", "Device type doesn't match");
		return true;
	}
	alpaca_member_spec *member = find_member(type, command);
	if (member == NULL) {
		send_text_response(handle, path, 400, "Bad Request", "Unrecognised command");
		return true;
	}
	bool keep_alive = true;
	alpaca_request *request = indigo_safe_malloc(sizeof(alpaca_request));
	if (!strncmp(method, "GET", 3)) {
		parse_params(params, request, false);
		if (!member->get) {
			send_text_response(handle, path, 400, "Bad Request", "Invalid method");
		} else if (!validate_params(request, member->get_params, false, message, sizeof(message))) {
			send_text_response(handle, path, 400, "Bad Request", message);
		} else if (!strcmp(command, "imagearray") || !strcmp(command, "imagearrayvariant")) {
			indigo_alpaca_ccd_get_imagearray(alpaca_device, 1, handle, request->client_transaction_id, server_transaction_id++, !strcmp(method, "GET/GZIP"), !strcmp(method, "GET/IMAGEBYTES"));
			keep_alive = false;
		} else {
			const char *id = find_param(request, "Id", false);
			char *buffer = indigo_alloc_large_buffer();
			long index = snprintf(buffer, INDIGO_BUFFER_SIZE, "{ ");
			long length;
			if (!strcmp(command, "destinationsideofpier")) {
				length = indigo_alpaca_mount_get_destinationsideofpier(alpaca_device, 1, atof(find_param(request, "RightAscension", false)), atof(find_param(request, "Declination", false)), buffer + index, INDIGO_BUFFER_SIZE - index);
			} else {
				length = indigo_alpaca_get_command(alpaca_device, 1, command, id ? atoi(id) : 0, buffer + index, INDIGO_BUFFER_SIZE - index);
			}
			if (length <= 0) {
				length = indigo_alpaca_append_error(buffer + index, INDIGO_BUFFER_SIZE - index, indigo_alpaca_error_NotImplemented);
			}
			index += length;
			snprintf(buffer + index, INDIGO_BUFFER_SIZE - index, ", \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", request->client_transaction_id, server_transaction_id++);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %s", buffer);
			send_json_response(handle, path, 200, "OK", buffer);
			indigo_free_large_buffer(buffer);
		}
	} else if (!strcmp(method, "PUT")) {
		parse_params(body, request, true);
		if (!member->put) {
			send_text_response(handle, path, 400, "Bad Request", "Invalid method");
		} else if (!validate_params(request, member->put_params, true, message, sizeof(message))) {
			send_text_response(handle, path, 400, "Bad Request", message);
		} else {
			// only the parameters defined for the member are passed to the handlers, as "Name=value" sorted by name
			char args[2][128] = { { 0 } };
			int count = 0;
			for (int i = 0; i < 2 && member->put_params[i].name; i++) {
				snprintf(args[count++], 128, "%s=%s", member->put_params[i].name, find_param(request, member->put_params[i].name, true));
			}
			if (count > 1) {
				qsort(args, count, 128, string_cmp);
			}
			char *buffer = indigo_alloc_large_buffer();
			long index = snprintf(buffer, INDIGO_BUFFER_SIZE, "{ ");
			long length = indigo_alpaca_set_command(alpaca_device, 1, command, buffer + index, INDIGO_BUFFER_SIZE - index, args[0], args[1]);
			if (length <= 0) {
				length = indigo_alpaca_append_error(buffer + index, INDIGO_BUFFER_SIZE - index, indigo_alpaca_error_NotImplemented);
			}
			index += length;
			snprintf(buffer + index, INDIGO_BUFFER_SIZE - index, ", \"ClientTransactionID\": %u, \"ServerTransactionID\": %u }", request->client_transaction_id, server_transaction_id++);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %s", buffer);
			send_json_response(handle, path, 200, "OK", buffer);
			indigo_free_large_buffer(buffer);
		}
	} else {
		send_text_response(handle, path, 400, "Bad Request", "Invalid method");
	}
	indigo_safe_free(request);
	return keep_alive;
}

static bool alpaca_v1_api_handler(indigo_uni_handle *handle, char *method, char *path, char *params) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %s %s %s", method, path, params);
	char *body = NULL;
	if (!strcmp(method, "PUT")) {
		// headers and body are read before the request is validated to keep the connection in sync
		int content_length = 0;
		body = indigo_alloc_large_buffer();
		while (indigo_uni_read_line(handle, body, INDIGO_BUFFER_SIZE - 1) > 0) {
			if (!strncasecmp(body, "Content-Length:", 15)) {
				content_length = atoi(body + 15);
			}
		}
		// Content-Length is client supplied, so it has to be clamped to the buffer before it is
		// used as a read length and as an index.
		if (content_length < 0) {
			content_length = 0;
		} else if (content_length > INDIGO_BUFFER_SIZE - 1) {
			content_length = INDIGO_BUFFER_SIZE - 1;
		}
		indigo_uni_read_line(handle, body, content_length);
		body[content_length] = 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %s", body);
	}
	bool keep_alive = alpaca_v1_api_request(handle, method, path, params, body);
	if (body) {
		indigo_free_large_buffer(body);
	}
	return keep_alive;
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AGENT) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		AGENT_DISCOVERY_PROPERTY = indigo_init_number_property(NULL, device->name, "AGENT_ALPACA_DISCOVERY", MAIN_GROUP, "Discovery Configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_DISCOVERY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AGENT_DISCOVERY_PORT_ITEM, "PORT", "Discovery port", 0, 0xFFFF, 0, 32227);
		AGENT_DEVICES_PROPERTY = indigo_init_text_property(NULL, device->name, "AGENT_ALPACA_DEVICES", MAIN_GROUP, "Device mapping", INDIGO_OK_STATE, INDIGO_RW_PERM, ALPACA_MAX_ITEMS);
		if (AGENT_DISCOVERY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		for (int i = 0; i < ALPACA_MAX_ITEMS; i++) {
			sprintf(AGENT_DEVICES_PROPERTY->items[i].name, "%d", i);
			sprintf(AGENT_DEVICES_PROPERTY->items[i].label, "Device #%d", i);
		}
		AGENT_DEVICES_PROPERTY->count = 0;

		AGENT_CAMERA_BAYERPAT_PROPERTY = indigo_init_text_property(NULL, device->name, "AGENT_ALPACA_CAMERA_BAYERPAT", MAIN_GROUP, "Camera Bayer pattern", INDIGO_OK_STATE, INDIGO_RW_PERM, ALPACA_MAX_ITEMS);
		if (AGENT_CAMERA_BAYERPAT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		for (int i = 0; i < ALPACA_MAX_ITEMS; i++) {
			AGENT_CAMERA_BAYERPAT_PROPERTY->items[i].name[0] = '\0';
			AGENT_CAMERA_BAYERPAT_PROPERTY->items[i].label[0] = '\0';
			AGENT_CAMERA_BAYERPAT_PROPERTY->items[i].text.value[0] = '\0';
		}
		AGENT_CAMERA_BAYERPAT_PROPERTY->count = 0;
		// --------------------------------------------------------------------------------
		srand((unsigned)time(0));
		indigo_set_timer(device, 0, start_discovery_server, &private_data->discovery_server_timer);
		indigo_server_add_handler("/setup", &alpaca_setup_handler);
		indigo_server_add_handler("/management/apiversions", &alpaca_apiversions_handler);
		indigo_server_add_handler("/management/v1/description", &alpaca_v1_description_handler);
		indigo_server_add_handler("/management/v1/configureddevices", &alpaca_v1_configureddevices_handler);
		indigo_server_add_handler("/api/v1", &alpaca_v1_api_handler);
		CONNECTION_PROPERTY->hidden = true;
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client == indigo_agent_alpaca_client) {
		return INDIGO_OK;
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_DISCOVERY_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_DEVICES_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_CAMERA_BAYERPAT_PROPERTY);
	return indigo_device_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == indigo_agent_alpaca_client) {
		return INDIGO_OK;
	}
	if (indigo_property_match(AGENT_DISCOVERY_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_DISCOVERY_PROPERTY, property, false);
		shutdown_discovery_server();
		indigo_set_timer(device, 0, start_discovery_server, &private_data->discovery_server_timer);
		AGENT_DISCOVERY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_DISCOVERY_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_DEVICES_PROPERTY, property)) {
		int count = AGENT_DEVICES_PROPERTY->count;
		AGENT_DEVICES_PROPERTY->count = ALPACA_MAX_ITEMS;
		indigo_property_copy_values(AGENT_DEVICES_PROPERTY, property, false);
		for (int i = ALPACA_MAX_ITEMS; i; i--) {
			indigo_item *item = AGENT_DEVICES_PROPERTY->items + i - 1;
			if (*item->text.value) {
				AGENT_DEVICES_PROPERTY->count = i;
				break;
			}
		}
		AGENT_DEVICES_PROPERTY->state = INDIGO_OK_STATE;
		if (count == AGENT_DEVICES_PROPERTY->count) {
			indigo_update_property(device, AGENT_DEVICES_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, AGENT_DEVICES_PROPERTY, NULL);
			indigo_define_property(device, AGENT_DEVICES_PROPERTY, NULL);
		}
		save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_CAMERA_BAYERPAT_PROPERTY, property)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!get_bayer_RGGB_offsets(item->text.value, NULL, NULL) && item->text.value[0] != '\0') {
				AGENT_CAMERA_BAYERPAT_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_CAMERA_BAYERPAT_PROPERTY, "Bayer pattern '%s' is not supported", item->text.value);
				return INDIGO_OK;
			}
		}
		indigo_property_copy_values(AGENT_CAMERA_BAYERPAT_PROPERTY, property, false);
		AGENT_CAMERA_BAYERPAT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_CAMERA_BAYERPAT_PROPERTY, NULL);
		save_config(device);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	shutdown_discovery_server();
	indigo_server_remove_resource("/setup");
	indigo_server_remove_resource("/management/apiversions");
	indigo_server_remove_resource("/management/v1/description");
	indigo_server_remove_resource("/management/v1/configureddevices");
	indigo_server_remove_resource("/api/v1");
	indigo_cancel_timer_sync(device, &private_data->discovery_server_timer);
	indigo_release_property(AGENT_DISCOVERY_PROPERTY);
	indigo_release_property(AGENT_DEVICES_PROPERTY);
	indigo_release_property(AGENT_CAMERA_BAYERPAT_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

// Guider of a mount is by convention an INDIGO device named "<mount name> (guider)".

static bool is_mount_guider(indigo_alpaca_device *mount, indigo_alpaca_device *guider) {
	char name[INDIGO_NAME_SIZE + 16];
	snprintf(name, sizeof(name), "%s (guider)", mount->indigo_device);
	return !strcmp(name, guider->indigo_device);
}

static void pair_guider(indigo_alpaca_device *alpaca_device) {
	for (indigo_alpaca_device *other = alpaca_devices; other; other = other->next) {
		if (other == alpaca_device) {
			continue;
		}
		if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_MOUNT) && IS_DEVICE_TYPE(other, INDIGO_INTERFACE_GUIDER) && !IS_DEVICE_TYPE(other, INDIGO_INTERFACE_MOUNT) && is_mount_guider(alpaca_device, other)) {
			alpaca_device->guider_device = other;
		} else if (IS_DEVICE_TYPE(other, INDIGO_INTERFACE_MOUNT) && is_mount_guider(other, alpaca_device)) {
			other->guider_device = alpaca_device;
		}
	}
}

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == indigo_agent_alpaca_device) {
		return INDIGO_OK;
	}
	indigo_alpaca_device *alpaca_device = alpaca_devices;
	while (alpaca_device) {
		if (!strcmp(property->device, alpaca_device->indigo_device))
			break;
		alpaca_device = alpaca_device->next;
	}
	if (alpaca_device == NULL) {
		unsigned char digest[15] = { 0 };
		for (int i = 0, j = 0; property->device[i]; i++, j = (j + 1) % 15) {
			digest[j] = digest[j] ^ property->device[i];
		}
		alpaca_device = indigo_safe_malloc(sizeof(indigo_alpaca_device));
		strcpy(alpaca_device->indigo_device, property->device);
		alpaca_device->device_number = -1;
		strcpy(alpaca_device->device_uid, "xxxxxxxx-xxxx-4xxx-8xxx-xxxxxxxxxxxx");
		static char *hex = "0123456789ABCDEF";
		int i = 0;
		for (char *c = alpaca_device->device_uid; *c; c++) {
			if (*c == 'x') {
				int r = i % 2 == 0 ? digest[i / 2] % 15 : digest[i / 2] / 15;
				*c = hex[r];
				i++;
			}
		}
		pthread_mutex_init(&alpaca_device->mutex, NULL);
		alpaca_device->next = alpaca_devices;
		alpaca_devices = alpaca_device;
	}
	if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, INFO_DEVICE_INTERFACE_ITEM_NAME)) {
				alpaca_device->indigo_interface = (indigo_device_interface)atol(item->text.value);
				if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AGENT)) {
					alpaca_device->device_type = NULL;
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_CCD)) {
					alpaca_device->ccd.ccdtemperature = NAN;
					alpaca_device->device_type = "Camera";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_DOME)) {
					alpaca_device->device_type = "Dome";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_WHEEL)) {
					alpaca_device->device_type = "FilterWheel";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_FOCUSER)) {
					alpaca_device->device_type = "Focuser";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_ROTATOR)) {
					alpaca_device->device_type = "Rotator";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_POWERBOX) || IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_GPIO)) {
					alpaca_device->device_type = "Switch";
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_MOUNT)) {
					alpaca_device->device_type = "Telescope";
					pair_guider(alpaca_device);
				} else if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_LIGHTBOX)) {
					alpaca_device->device_type = "CoverCalibrator";
				} else {
					// standalone guiders and AO units can't implement mandatory ITelescope members, guider of a mount is used by its Telescope
					alpaca_device->device_type = NULL;
					if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_GUIDER)) {
						pair_guider(alpaca_device);
					}
				}
				if (alpaca_device->device_type) {
					int device_number;
					for (device_number = 0; device_number < AGENT_DEVICES_PROPERTY->count; device_number++) {
						indigo_item *item = AGENT_DEVICES_PROPERTY->items + device_number;
						if (!strcmp(property->device, item->text.value)) {
							alpaca_device->device_number = device_number;
							break;
						}
					}
					if (alpaca_device->device_number < 0) {
						for (device_number = 0; device_number < AGENT_DEVICES_PROPERTY->count; device_number++) {
							item = AGENT_DEVICES_PROPERTY->items + device_number;
							if (*AGENT_DEVICES_PROPERTY->items[device_number].text.value == 0) {
								break;
							}
						}
						if (device_number < ALPACA_MAX_ITEMS) {
							indigo_item *item = AGENT_DEVICES_PROPERTY->items + device_number;
							strcpy(item->text.value, property->device);
							alpaca_device->device_number = device_number;
							indigo_debug("Device %s mapped to #%d", property->device, device_number);
							indigo_delete_property(indigo_agent_alpaca_device, AGENT_DEVICES_PROPERTY, NULL);
							if (device_number == AGENT_DEVICES_PROPERTY->count) {
								AGENT_DEVICES_PROPERTY->count++;
							}
							indigo_define_property(indigo_agent_alpaca_device, AGENT_DEVICES_PROPERTY, NULL);
							save_config(indigo_agent_alpaca_device);
						} else {
							indigo_send_message(indigo_agent_alpaca_device, ALERT_PROPERTY, "Too many Alpaca devices configured");
						}
					}
					if (IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_CCD)) {
						int cam_number;
						for (cam_number = 0; cam_number < AGENT_CAMERA_BAYERPAT_PROPERTY->count; cam_number++) {
							indigo_item *item = AGENT_CAMERA_BAYERPAT_PROPERTY->items + cam_number;
							if (!strcmp(property->device, item->label)) {
								indigo_debug("=== Camera %s already mapped to #%d", property->device, cam_number);
								break;
							}
						}
						if (cam_number == AGENT_CAMERA_BAYERPAT_PROPERTY->count) {
							indigo_debug("+++ Mapping camera %s to #%d", property->device, cam_number);
							indigo_item *item = AGENT_CAMERA_BAYERPAT_PROPERTY->items + cam_number;
							strcpy(item->label, property->device);
							sprintf(item->name, "%d", alpaca_device->device_number);
							alpaca_device->ccd.bayer_matrix = item;
							indigo_delete_property(indigo_agent_alpaca_device, AGENT_CAMERA_BAYERPAT_PROPERTY, NULL);
							AGENT_CAMERA_BAYERPAT_PROPERTY->count ++;
							indigo_define_property(indigo_agent_alpaca_device, AGENT_CAMERA_BAYERPAT_PROPERTY, NULL);
							indigo_load_properties(indigo_agent_alpaca_device, false);
						}
					}
				}
			} else if (!strcmp(item->name, INFO_DEVICE_NAME_ITEM_NAME)) {
				pthread_mutex_lock(&alpaca_device->mutex);
				INDIGO_COPY_NAME(alpaca_device->device_name, item->text.value);
				pthread_mutex_unlock(&alpaca_device->mutex);
			} else if (!strcmp(item->name, INFO_DEVICE_DRIVER_ITEM_NAME)) {
				pthread_mutex_lock(&alpaca_device->mutex);
				strcpy(alpaca_device->driver_info, item->text.value);
				pthread_mutex_unlock(&alpaca_device->mutex);
			} else if (!strcmp(item->name, INFO_DEVICE_VERSION_ITEM_NAME)) {
				pthread_mutex_lock(&alpaca_device->mutex);
				strcpy(alpaca_device->driver_version, item->text.value);
				pthread_mutex_unlock(&alpaca_device->mutex);
			}
		}
	} else {
		indigo_alpaca_update_property(alpaca_device, property);
	}
	return INDIGO_OK;
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_alpaca_device *alpaca_device = alpaca_devices;
	while (alpaca_device) {
		if (!strcmp(property->device, alpaca_device->indigo_device)) {
			indigo_alpaca_update_property(alpaca_device, property);
			break;
		}
		alpaca_device = alpaca_device->next;
	}
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_alpaca_device *alpaca_device = alpaca_devices, *previous = NULL;
	while (alpaca_device) {
		if (!strcmp(property->device, alpaca_device->indigo_device)) {
			if (*property->name == 0 || !strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
				if (previous == NULL) {
					alpaca_devices = alpaca_device->next;
				} else {
					previous->next = alpaca_device->next;
				}
				for (indigo_alpaca_device *other = alpaca_devices; other; other = other->next) {
					if (other->guider_device == alpaca_device) {
						other->guider_device = NULL;
					}
				}
				indigo_safe_free(alpaca_device);
			}
			break;
		}
		previous = alpaca_device;
		alpaca_device = alpaca_device->next;
	}
	return INDIGO_OK;
}

static indigo_result agent_attach(indigo_client *client) {
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- Initialization

indigo_result indigo_agent_alpaca(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		ALPACA_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		ALPACA_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		agent_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		NULL
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ASCOM Alpaca bridge agent", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(alpaca_agent_private_data));
			indigo_agent_alpaca_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			indigo_agent_alpaca_device->private_data = private_data;
			indigo_agent_alpaca_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			indigo_agent_alpaca_client->client_context = indigo_agent_alpaca_device->device_context;
			indigo_attach_device(indigo_agent_alpaca_device);
			indigo_attach_client(indigo_agent_alpaca_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (indigo_agent_alpaca_client != NULL) {
				indigo_detach_client(indigo_agent_alpaca_client);
				free(indigo_agent_alpaca_client);
				indigo_agent_alpaca_client = NULL;
			}
			if (indigo_agent_alpaca_device != NULL) {
				indigo_detach_device(indigo_agent_alpaca_device);
				free(indigo_agent_alpaca_device);
				indigo_agent_alpaca_device = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			indigo_alpaca_device *alpaca_device = alpaca_devices;
			while (alpaca_device) {
				indigo_alpaca_device *tmp = alpaca_device;
				alpaca_device = alpaca_device->next;
				indigo_safe_free(tmp);
			}
			alpaca_devices = NULL;
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
