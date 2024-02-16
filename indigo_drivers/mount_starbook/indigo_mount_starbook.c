// Copyright (c) 2017 CloudMakers, s. r. o.
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
// 2.0 by Koji TSUNODA <tsunoda@astroarts.co.jp>

/** INDIGO Vixen StarBook driver
 \file indigo_mount_starbook.c
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME	"indigo_mount_starbook"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_align.h>

#include "indigo_mount_starbook.h"

#define PRIVATE_DATA        ((starbook_private_data *)device->private_data)

#define STARBOOK_DEFAULT_IP_ADDR           "169.254.0.1"

#define MOUNT_ADVANCED_GROUP         "Advanced"

#define TIMEZONE_PROPERTY                  (PRIVATE_DATA->timezone_property)
#define TIMEZONE_VALUE_ITEM                (TIMEZONE_PROPERTY->items+0)
#define TIMEZONE_PROPERTY_NAME             "STARBOOK_TIMEZONE"
#define TIMEZONE_VALUE_ITEM_NAME           "VALUE"

#define RESET_PROPERTY                    (PRIVATE_DATA->reset_property)
#define RESET_CTRL_ITEM                   (RESET_PROPERTY->items+0)
#define RESET_PROPERTY_NAME               "STARBOOK_RESET"
#define RESET_CTRL_ITEM_NAME              "RESET"


typedef struct {
	int device_count;
	double version;
	int errorCounter;
	double currentRA;
	double currentDec;
	char telescopeSide;
	bool isBusy, startTracking, stopTracking;
	bool resetWaiting;
	indigo_timer *position_timer, *ra_guider_timer, *dec_guider_timer;
	pthread_mutex_t port_mutex;
	indigo_property *timezone_property;
	indigo_property *reset_property;
} starbook_private_data;

// -------------------------------------------------------------------------------- Ethernet support

//static bool get_host_ip(char *hostname , unsigned long *ip) {
//	struct addrinfo hints, *servinfo, *p;
//	int rv;
//
//	memset(&hints, 0, sizeof hints);
//	hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
//	hints.ai_socktype = SOCK_STREAM;
//
//	if ((rv = getaddrinfo(hostname, NULL, &hints, &servinfo)) != 0) {
//		INDIGO_DRIVER_ERROR(DRIVER_NAME, "getaddrinfo(): %s\n", gai_strerror(rv));
//		return false;
//	}
//
//	for(p = servinfo; p != NULL; p = p->ai_next) {
//		if(p->ai_family == AF_INET) {
//			*ip = ((struct sockaddr_in *)(p->ai_addr))->sin_addr.s_addr;
//			/* ip should be litle endian */
//			*ip = (*ip >> 24) | ((*ip << 8) & 0x00ff0000) | ((*ip >> 8) & 0x0000ff00) | (*ip << 24);
//			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "IP: 0x%X\n", *ip);
//			freeaddrinfo(servinfo);
//			return true;
//		}
//	}
//	freeaddrinfo(servinfo);
//	return false;
//}


static int connect_with_timeout(int sock, const struct sockaddr *name, int namelen, struct timeval timeout) {
	// set async
	int flags = fcntl(sock, F_GETFL);
	if (flags == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "GETFL failed.");
		return -1;
	}
	int result = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	if (flags == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SETFL failed.");
		return -1;
	}

	// connect
	result = connect(sock, name, namelen);
	if (result < 0) {
		if (errno == EINPROGRESS) {
			// succeed. wait by select()
			errno = 0;
		} else {
			// failed.
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: %d", errno);
			// reset flags
			fcntl(sock, F_SETFL, flags);
			if (errno == 64) {  // Linux:ENONET, Mac:EHOSTDOWN
				// TODO: mount was disconnected?
			}
			return -1;
		}
	}

	// set await
	result = fcntl(sock, F_SETFL, flags);
	if (result == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SETFL failed.");
		return -1;
	}

	// wait connect
	fd_set readFd, writeFd, errFd;
	FD_ZERO(&readFd);
	FD_ZERO(&writeFd);
	FD_ZERO(&errFd);
	FD_SET(sock, &readFd);
	FD_SET(sock, &writeFd);
	FD_SET(sock, &errFd);
	int sockNum = select(sock + 1, &readFd, &writeFd, &errFd, &timeout);
	if (sockNum == 0) {
		// timeout
		return -1;
	} else if (FD_ISSET(sock, &readFd) || FD_ISSET(sock, &writeFd)) {
		// read/writable
		// do nothing here
	} else {
		// error
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "select failed.");
		return -1;
	}

	// check socket error
	int optval = 0;
	socklen_t optlen = (socklen_t)sizeof(optval);
	errno = 0;
	result = getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)&optval, &optlen);
	if (result < 0) {
		// error
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "getsockopt failed.");
		return -1;
	} else if (optval != 0) {
		// error
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid optval: %d", optval);
		return -1;
	}
	return 0;
}

//
// verbose options
//
//#define DEBUG_HTTP_RESPONSE
#define DEBUG_API_RESPONSE

// timeout: milliseconds
static bool starbook_http_get(indigo_device *device, const char *path, char *result, int length, int timeout) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);

	const char *host = DEVICE_PORT_ITEM->text.value;
	const char *port = "80";
	//INDIGO_DRIVER_LOG(DRIVER_NAME, "starbook_http_get(\"%s\", \"%s\", \"%s\")", host, port, path);

	struct addrinfo hints = {};
	struct addrinfo *res = NULL;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	int err = -1;
	if ((err = getaddrinfo(host, port, &hints, &res)) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "getaddrinfo(): %s", gai_strerror(err));
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return false;
	}

	int sock = -1;
	if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: cannot create socket.");
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return false;
	}

	struct timeval tv = {};
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	if (connect_with_timeout(sock, res->ai_addr, res->ai_addrlen, tv) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: cannot connect %s:%s", host, port);
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return false;
	}

	// build message:
	//   GET /path HTTP/1.0\r\n
	//   Host: example.com\r\n
	//   \r\n
	const int port_len = (strcmp(port, "80") == 0) ? 0 : (1 + (int)strlen(port));
	int message_length = (
		4     // "GET "
		+ (int)strlen(path)  // /path
		+ 11  // " HTTP/1.0\r\n"
		+ 6   // "Host: "
		+ (int)strlen(host) + port_len
		+ 4   // "\r\n\r\n"
	);
	char *message = (char *)malloc(message_length + 1);
	sprintf(
		message,
		"GET %s HTTP/1.0\r\n"
		"Host: %s%s%s\r\n"
		"\r\n",
		path,
		host,
		(port_len == 0 ? "" : ":"),
		(port_len == 0 ? "" : port)
	);
	write(sock, message, message_length);
	free(message);

	// set read timeout
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(tv));

	int read_size = 0;
	while (true) {
		int size = (int)read(sock, &result[read_size], length - read_size);
		if (size == 0) {
			break;
		} else if (size < 0) {
			// network was disconnected?
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "read failed.");
			close(sock);
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		read_size += size;
		if (read_size == length) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "not enough buffer.");
			close(sock);
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
	}
	close(sock);
#ifdef DEBUG_HTTP_RESPONSE
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", result);
#endif
	if (strlen(result) < 13) {
		// empty?
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: bad response: %d", strlen(result));
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return false;
	}
	if (memcmp(result, "HTTP/1.0 200 OK\r\n", 17) != 0) {
		// not 200 OK
		char code[4] = {};
		memcpy(code, (char *)&result[9], 3);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: code: %s", code);
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return false;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return true;
}


static bool starbook_get(indigo_device *device, const char *path, char *buffer, int length) {
	char temp[4096];
	const int timeout = 3000;  // milliseconds
	if (!starbook_http_get(device, path, temp, sizeof(temp), timeout)) {
		return false;
	}
	// find comment part
	char *beg = strstr(temp, "<!--");
	char *end = strstr(temp, "-->");
	int beg_tag_len = 4;
	if (beg == NULL || end == NULL) {
		// new API?
		beg = strstr(temp, "</HEAD>");
		end = strstr(temp, "</html>");
		beg_tag_len = 7;
		if (beg == NULL || end == NULL) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: Unknown response: %s", temp);
			return false;
		}
	}
	// check size of response
	const int response_size = (int)(end - (beg + beg_tag_len));
	if (response_size >= length) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: not enough memory");
		return false;
	}
	else if (response_size < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parse error: %s", temp);
		return false;
	}

	memcpy(buffer, (const char *)&beg[beg_tag_len], response_size);
	buffer[response_size] = '\0';
#ifdef DEBUG_API_RESPONSE
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s : %s", path, buffer);
#endif
	return true;
}


#define STARBOOK_NO_ERROR 0
#define STARBOOK_ERROR_ILLEGAL_STATE 1
#define STARBOOK_ERROR_FORMAT 2
#define STARBOOK_ERROR_BELOW_HORIZON 3
#define STARBOOK_WARNING_NEAR_SUN 4
#define STARBOOK_ERROR_UNKNOWN 5

static const char *STARBOOK_NO_ERROR_STR = "NO ERROR";
static const char *STARBOOK_ERROR_ILLEGAL_STATE_STR = "ILLEGAL STATE";
static const char *STARBOOK_ERROR_FORMAT_STR = "FORMAT";
static const char *STARBOOK_ERROR_BELOW_HORIZON_STR = "BELOW HORIZON";
static const char *STARBOOK_WARNING_NEAR_SUN_STR = "NEAR SUN";
static const char *STARBOOK_ERROR_UNKNOWN_STR = "UNKNOWN";

static const char *starbook_error_text(int code) {
	switch (code) {
	case STARBOOK_NO_ERROR:
		return STARBOOK_NO_ERROR_STR;
	case STARBOOK_ERROR_ILLEGAL_STATE:
		return STARBOOK_ERROR_ILLEGAL_STATE_STR;
	case STARBOOK_ERROR_FORMAT:
		return STARBOOK_ERROR_FORMAT_STR;
	case STARBOOK_ERROR_BELOW_HORIZON:
		return STARBOOK_ERROR_BELOW_HORIZON_STR;
	case STARBOOK_WARNING_NEAR_SUN:
		return STARBOOK_WARNING_NEAR_SUN_STR;
	case STARBOOK_ERROR_UNKNOWN:
	default:
		return STARBOOK_ERROR_UNKNOWN_STR;
	}
}


static bool starbook_set(indigo_device *device, const char *path, int *error) {
	if (error) {
		*error = STARBOOK_NO_ERROR;
	}
	char buffer[1024];
	if (!starbook_get(device, path, buffer, sizeof(buffer))) {
		return false;
	}
	if (strcmp(buffer, "OK") != 0) {
		if (error) {
			if (strcmp(buffer, "ERROR:ILLEGAL STATE") == 0) {
				*error = STARBOOK_ERROR_ILLEGAL_STATE;
			}
			else if (strcmp(buffer, "ERROR:FORMAT") == 0) {
				*error = STARBOOK_ERROR_FORMAT;
			}
			else if (strcmp(buffer, "ERROR:BELOW HORIZON") == 0
							 || strcmp(buffer, "ERROR:BELOW HORIZONE") == 0) {  // maybe typo
				*error = STARBOOK_ERROR_BELOW_HORIZON;
			}
			else if (strcmp(buffer, "WARNING:NEAR SUN") == 0) {
				*error = STARBOOK_WARNING_NEAR_SUN;
			}
			else {
				*error = STARBOOK_ERROR_UNKNOWN;
			}
		}
		return false;
	}
	return true;
}


static bool starbook_parse_query_value(const char *query, const char *key, char *value, int size) {
	// search query key
	const char *beg = strstr(query, key);
	if (beg == NULL) {
		// not found
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: key \"%s\" was not found: %s", key, query);
		return false;
	}
	if (value == NULL) {
		// do nothing
		return true;
	}
	// search query delimiter
	int len = 0;
	const char *end = strstr(beg, "&");
	const int offset = (int)(beg - query + strlen(key));
	if (end == NULL) {
		// last query
		len = (int)(strlen(query) - offset);
	} else {
		// found delimiter
		len = (int)(end - beg - strlen(key));
	}
	// check memory size
	if (len >= size) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: starbook_parse_query_value() has not enough memory: %d", len);
		return false;
	}
	// copy query value
	memcpy(value, &query[offset], len);
	value[len] = 0;
	return true;
}


// parse query value as double
static bool starbook_parse_query_double(const char *query, const char *key, double *result) {
	char temp[128];
	if (!starbook_parse_query_value(query, key, temp, sizeof(temp))) {
		// parse failed
		return false;
	}
	*result = atof(temp);
	return true;
}


// parse query value as int
static bool starbook_parse_query_int(const char *query, const char *key, int *result) {
	char temp[128];
	if (!starbook_parse_query_value(query, key, temp, sizeof(temp))) {
		// parse failed
		return false;
	}
	*result = atoi(temp);
	return true;
}


// parse degree-minute format value
//   RA=01+23.4 => (+1, 1, 23.4)
//   DEC=012+34 => (+1, 12, 34.0)
static bool starbook_parse_query_degree_minute(const char *query, const char *key, int *sign, int *degree, double *minute) {
	char temp[128];
	if (!starbook_parse_query_value(query, key, temp, sizeof(temp))) {
		// parse failed
		return false;
	}
	int s = (temp[0] == '-') ? -1 : 1;
	if (sign) {
		*sign = s;
	}
	const char *beg = (s < 0) ? &temp[1] : temp;
	const char *sep = strstr(beg, "+");
	if (sep == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: Delimiter was not found: %s", temp);
		return false;
	}
	if (strlen(sep) <= 1) {
		// format error
		// e.g. "01+"
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: Unknown format: %s", temp);
		return false;
	}
	if (degree) {
		*degree = atoi(beg);
	}
	if (minute) {
		*minute = atof(&sep[1]);
	}
	return true;
}


// 12.5 => (+1, 12, 30.0)
static void parse_degree_minute(double value, int *sign, int *degree, double *minute) {
	if (sign) {
		*sign = value >= 0 ? 1 : -1;
	}
	if (value < 0) {
		value *= -1;
	}
	double num = 0;
	double frac = modf(value, &num);
	if (degree) {
		*degree = (int)num;
	}
	if (minute) {
		*minute = frac * 60;
	}
}


static bool starbook_align(indigo_device *device, double ra, double dec, int *error) {
	int ra_deg = 0, dec_deg = 0;
	double ra_min = 0, dec_min = 0;
	int dec_sign = 0;
	parse_degree_minute(ra, NULL, &ra_deg, &ra_min);
	parse_degree_minute(dec, &dec_sign, &dec_deg, &dec_min);
	const char sign[2] = { dec_sign == 1 ? '+' : '-', 0 };

	char path[1024];
	if (PRIVATE_DATA->version >= 4.20) {
		// high-precision
		sprintf(path,
			"/ALIGN?ra=%d+%04.3f&dec=%s%d+%02.2f",
			ra_deg, ra_min,
			sign, dec_deg, dec_min
		);
	} else {
		// traditional
		sprintf(path,
			"/ALIGN?ra=%d+%02.1f&dec=%s%d+%02d",
			ra_deg, ra_min,
			sign, dec_deg, (int)dec_min
		);
	}
	int temp = STARBOOK_NO_ERROR;
	if (!error) {
		error = &temp;
	}
	if (!starbook_set(device, path, error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", *error);
		return false;
	}
	return true;
}


static bool starbook_goto_radec(indigo_device *device, double ra, double dec, int *error) {
	int ra_deg = 0, dec_deg = 0;
	double ra_min = 0, dec_min = 0;
	int dec_sign = 0;
	parse_degree_minute(ra, NULL, &ra_deg, &ra_min);
	parse_degree_minute(dec, &dec_sign, &dec_deg, &dec_min);
	const char sign[2] = { dec_sign == 1 ? '+' : '-', 0 };

	char path[1024];
	if (PRIVATE_DATA->version >= 4.20) {
		// high-precision
		sprintf(path,
			"/GOTORADEC?ra=%d+%04.3f&dec=%s%d+%02.2f",
			ra_deg, ra_min,
			sign, dec_deg, dec_min
		);
	} else {
		// traditional
		sprintf(path,
			"/GOTORADEC?ra=%d+%02.1f&dec=%s%d+%02d",
			ra_deg, ra_min,
			sign, dec_deg, (int)dec_min
		);
	}
	int temp = STARBOOK_NO_ERROR;
	if (!error) {
		error = &temp;
	}
	if (!starbook_set(device, path, error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", *error);
		return false;
	}
	return true;
}


static bool starbook_get_place(indigo_device *device, double *lng, double *lat, int *timezone) {
	char buffer[1024] = {};
	if (!starbook_get(device, "/GETPLACE", buffer, sizeof(buffer))) {
		return false;
	}
	if (lng) {
		char temp[128];
		if (!starbook_parse_query_value(buffer, "LONGITUDE=", temp, sizeof(temp))) {
			return false;
		}
		// response format is "Ddd+mm"; "E139+00"
		// get East or West
		if (temp[0] != 'E' && temp[0] != 'W') {
			return false;
		}
		int sign = 1;
		if (temp[0] == 'W') {
			sign = -1;
		}
		const char *pos = strstr(temp, "+");
		if (pos == NULL || pos - temp <= 0) {
			return false;
		}
		int len = (int)(pos - temp);
		char temp2[128];
		if (len >= sizeof(temp2)) {
			return false;
		}
		// get degree part; "139"
		memcpy(temp2, &temp[1], len-1);
		temp2[len-1] = 0;
		*lng = atoi(temp2);
		// get minute part; "00"
		int len2 = (int)strlen(temp) - len - 1;  // -1 means skip the "+"
		memcpy(temp2, &temp[len+1], len2+1);  // len2+1 means include NULL letter
		*lng += atof(temp2) / 60.0;
		// apply sign
		*lng *= sign;
	}
	if (lat) {
		char temp[128];
		if (!starbook_parse_query_value(buffer, "LATITUDE=", temp, sizeof(temp))) {
			return false;
		}
		// response format is "Ddd+mm"; "N35+00"
		// get sign
		if (temp[0] != 'N' && temp[0] != 'S') {
			return false;
		}
		int sign = 1;
		if (temp[0] == 'S') {
			sign = -1;
		}
		const char *pos = strstr(&temp[1], "+");
		if (pos == NULL || pos - temp <= 0) {
			return false;
		}
		int len = (int)(pos - temp);
		char temp2[128];
		if (len >= sizeof(temp2)) {
			return false;
		}
		// get degree part; "35"
		memcpy(temp2, &temp[1], len-1);
		temp2[len-1] = 0;
		*lat = atoi(temp2);
		// get minute part; "00"
		int len2 = (int)strlen(temp) - len - 1;  // -1 means skip the "+"
		memcpy(temp2, &temp[len+1], len2+1);  // len2+1 means include NULL letter
		*lat += atof(temp2) / 60.0;
		// apply sign
		*lat *= sign;
	}
	if (timezone) {
		if (!starbook_parse_query_int(buffer, "TIMEZONE=", timezone)) {
			return false;
		}
	}
	return true;
}


static bool starbook_get_time(indigo_device *device, int *year, int *month, int *day, int *hour, int *minute, int *second) {
	char buffer[1024] = {};
	if (!starbook_get(device, "/GETTIME", buffer, sizeof(buffer))) {
		return false;
	}
	char temp[128];
	if (!starbook_parse_query_value(buffer, "TIME=", temp, sizeof(temp))) {
		return false;
	}
	char *pos = strchr(temp, '+');
	if (pos == NULL) {
		return false;
	}
	char *cur = temp;
	*pos = '\0';
	if (year) {
		*year = atoi(cur);
	}
	cur = pos + 1;
	pos = strchr(temp, '+');
	if (pos == NULL) {
		return false;
	}
	*pos = '\0';
	if (month) {
		*month = atoi(cur);
	}
	cur = pos + 1;
	pos = strchr(temp, '+');
	if (pos == NULL) {
		return false;
	}
	*pos = '\0';
	if (day) {
		*day = atoi(cur);
	}
	cur = pos + 1;
	pos = strchr(temp, '+');
	if (pos == NULL) {
		return false;
	}
	*pos = '\0';
	if (hour) {
		*hour = atoi(cur);
	}
	cur = pos + 1;
	pos = strchr(temp, '+');
	if (pos == NULL) {
		return false;
	}
	*pos = '\0';
	if (minute) {
		*minute = atoi(cur);
	}
	cur = pos + 1;
	if (second) {
		*second = atoi(cur);
	}
	return true;
}


#define STARBOOK_STATE_INIT 1
#define STARBOOK_STATE_SCOPE 2
#define STARBOOK_STATE_CHART 3
#define STARBOOK_STATE_STOP 4
#define STARBOOK_STATE_UNKNOWN 5

static bool starbook_get_status(indigo_device *device, double *ra, double *dec, int *now_on_goto, int *state) {
	char buffer[1024] = {};
	if (PRIVATE_DATA->version >= 4.20) {
		// high-precision API
		if (!starbook_get(device, "/GETSTATUS2", buffer, sizeof(buffer))) {
			return false;
		}
		if (ra) {
			if (!starbook_parse_query_double(buffer, "RA=", ra)) {
				return false;
			}
		}
		if (dec) {
			if (!starbook_parse_query_double(buffer, "DEC=", dec)) {
				return false;
			}
		}
	} else {
		// traditional API
		if (!starbook_get(device, "/GETSTATUS", buffer, sizeof(buffer))) {
			return false;
		}
		if (ra) {
			int sign = 0;
			int ra_h = 0;
			double ra_min = 0;
			if (!starbook_parse_query_degree_minute(buffer, "RA=", &sign, &ra_h, &ra_min)) {
				return false;
			}
			*ra = sign * (ra_h + ra_min / 60.0);
		}
		if (dec) {
			int sign = 0;
			int dec_deg = 0;
			double dec_min = 0;
			if (!starbook_parse_query_degree_minute(buffer, "DEC=", &sign, &dec_deg, &dec_min)) {
				return false;
			}
			*dec = sign * (dec_deg + dec_min / 60.0);
		}
	}
	// common part
	if (now_on_goto) {
		if (!starbook_parse_query_int(buffer, "GOTO=", now_on_goto)) {
			return false;
		}
	}
	if (state) {
		char temp[128] = {};
		if (!starbook_parse_query_value(buffer, "STATE=", temp, sizeof(temp))) {
			return false;
		}
		if (strcmp(temp, "SCOPE") == 0) {
			*state = STARBOOK_STATE_SCOPE;
		}
		else if (strcmp(temp, "STOP") == 0) {
			*state = STARBOOK_STATE_STOP;
		}
		else if (strcmp(temp, "INIT") == 0) {
			*state = STARBOOK_STATE_INIT;
		}
		else if (strcmp(temp, "CHART") == 0) {
			*state = STARBOOK_STATE_CHART;
		}
		else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unknown state: %s", temp);
			*state = STARBOOK_STATE_UNKNOWN;
		}
	}
	return true;
}


#define STARBOOK_TRACK_STATE_STOP 0
#define STARBOOK_TRACK_STATE_TRACK 1
#define STARBOOK_TRACK_STATE_GOTO 2

static bool starbook_get_track_status(indigo_device *device, int *state) {
	char buffer[1024] = {};
	if (!starbook_get(device, "/GETTRACKSTATUS", buffer, sizeof(buffer))) {
		return false;
	}
	if (state) {
		if (!starbook_parse_query_int(buffer, "TRACK=", state)) {
			return false;
		}
	}
	return true;
}


static bool starbook_get_version(indigo_device *device, double *version) {
	if (version) {
		*version = 0.0;
	}
	char buffer[1024] = {};
	if (!starbook_get(device, "/VERSION", buffer, sizeof(buffer))) {
		return false;
	}
	if (version) {
		if (!starbook_parse_query_double(buffer, "VERSION=", version)) {
			// parse error
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unknown response: %s", buffer);
			return false;
		}
	}
	return true;
}


#define STARBOOK_PIERSIDE_EAST 0
#define STARBOOK_PIERSIDE_WEST 1
#define STARBOOK_PIERSIDE_UNKNOWN -1

static bool starbook_get_pierside(indigo_device *device, int *side) {
	if (side) {
		*side = STARBOOK_PIERSIDE_UNKNOWN;
	}
	char buffer[1024] = {};
	if (!starbook_get(device, "/GET_PIERSIDE", buffer, sizeof(buffer))) {
		return false;
	}
	if (side) {
		if (!starbook_parse_query_int(buffer, "PIERSIDE=", side)) {
			// parse error
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unknown response: %s", buffer);
			return false;
		}
	}
	return true;
}


static bool starbook_getround(indigo_device *device, int *value) {
	if (value) {
		*value = 0;
	}
	char buffer[1024] = {};
	if (!starbook_get(device, "/GETROUND", buffer, sizeof(buffer))) {
		return false;
	}
	if (value) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "buffer: %s", buffer);
		if (!starbook_parse_query_int(buffer, "ROUND=", value)) {
			// parse error
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unknown response: %s", buffer);
			return false;
		}
	}
	return true;
}


#define STARBOOK_MOVE_ON  1
#define STARBOOK_MOVE_OFF 0

static bool starbook_move(indigo_device *device, bool north, bool south, bool east, bool west) {
	char path[1024];
	sprintf(path,
		"/MOVE?NORTH=%d&SOUTH=%d&EAST=%d&WEST=%d",
		north ? STARBOOK_MOVE_ON : STARBOOK_MOVE_OFF,
		south ? STARBOOK_MOVE_ON : STARBOOK_MOVE_OFF,
		east ? STARBOOK_MOVE_ON : STARBOOK_MOVE_OFF,
		west ? STARBOOK_MOVE_ON : STARBOOK_MOVE_OFF
	);
	int error = 0;
	if (!starbook_set(device, path, &error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


#define STARBOOK_MOVE_PULSE_NORTH 0
#define STARBOOK_MOVE_PULSE_SOUTH 1
#define STARBOOK_MOVE_PULSE_EAST 2
#define STARBOOK_MOVE_PULSE_WEST 3

static bool starbook_move_pulse(indigo_device *device, int direction, int duration) {
	if (direction != STARBOOK_MOVE_PULSE_NORTH
	    && direction != STARBOOK_MOVE_PULSE_SOUTH
			&& direction != STARBOOK_MOVE_PULSE_EAST
			&& direction != STARBOOK_MOVE_PULSE_WEST) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid direction: %d", direction);
		return false;
	}
	// milliseconds
	if (duration < 0 || duration > 100000) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid duration: %d", duration);
		return false;
	}
	char path[1024];
	sprintf(path,
		"/MOVEPULSE?DIRECT=%d&DURATION=%d",
		direction,
		duration
	);
	int error = 0;
	if (!starbook_set(device, path, &error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


static bool starbook_set_place(indigo_device *device, double lng, double lat, int timezone) {
	int lng_deg = 0, lat_deg = 0;
	double lng_min = 0, lat_min = 0;
	int lng_sign = 0, lat_sign = 0;
	parse_degree_minute(lng, &lng_sign, &lng_deg, &lng_min);
	parse_degree_minute(lat, &lat_sign, &lat_deg, &lat_min);
	const char east_west[2] = { lng_sign == 1 ? 'E' : 'W' , 0 };
	const char north_south[2] = { lat_sign == 1 ? 'N' : 'S', 0 };

	char path[1024];
	sprintf(path,
		"/SETPLACE?LONGITUDE=%s%d+%d&LATITUDE=%s%d+%d&TIMEZONE=%d",
		east_west, lng_deg, (int)lng_min,
		north_south, lat_deg, (int)lat_min,
		timezone
	);
	int error = 0;
	if (!starbook_set(device, path, &error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


static bool starbook_set_pulse_speed(indigo_device *device, int ra_sec, int dec_sec) {
	if (ra_sec < 0 || ra_sec > 300) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid input: starbook_set_pulse_speed(): ra_sec=%d", ra_sec);
		return false;
	}
	if (dec_sec < 0 || dec_sec > 300) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid input: starbook_set_pulse_speed(): dec_sec=%d", dec_sec);
		return false;
	}
	char path[1024];
	sprintf(path, "/SETPULSESPEED?RA=%d&DEC=%d", ra_sec, dec_sec);
	int error = 0;
	if (!starbook_set(device, path, &error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


#define STARBOOK_RADEC_TYPE_NOW 0
#define STARBOOK_RADEC_TYPE_J2000 1

static bool starbook_set_radec_type(indigo_device *device, int type) {
	bool ret = false;
	int error = 0;
	switch (type) {
	case STARBOOK_RADEC_TYPE_NOW:
		ret = starbook_set(device, "/SETRADECTYPE?TYPE=NOW", &error);
		break;
	case STARBOOK_RADEC_TYPE_J2000:
		ret = starbook_set(device, "/SETRADECTYPE?TYPE=J2000", &error);
		break;
	default:
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: Invalid radec type: %d", type);
		return false;
	}
	if (!ret) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


//
// for /MOVE (see starbook_move)
//
static bool starbook_set_speed(indigo_device *device, int speed) {
	if (speed < 0 || speed > 8) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid input: starbook_set_speed(): speed=%d", speed);
		return false;
	}
	char path[1024];
	sprintf(path, "/SETSPEED?speed=%d", speed);
	int error = 0;
	if (!starbook_set(device, path, &error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


static bool starbook_set_time(indigo_device *device, int year, int month, int day, int hour, int minute, int second) {
	char path[1024];
	sprintf(path,
		"/SETTIME?TIME=%d+%02d+%02d+%02d+%02d+%02d",
		year, month, day, hour, minute, second
	);
	int error = 0;
	if (!starbook_set(device, path, &error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


static bool starbook_reset(indigo_device *device) {
	int error = 0;
	if (!starbook_set(device, "/RESET", &error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


static bool starbook_start(indigo_device *device, bool init) {
	int error = 0;
	if (!starbook_set(device, init ? "/START?INIT=ON" : "/START?INIT=OFF", &error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


static bool starbook_stop(indigo_device *device) {
	int error = 0;
	if (!starbook_set(device, "/STOP", &error)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Error: %d", error);
		return false;
	}
	return true;
}


static bool starbook_open(indigo_device *device) {
	CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CONNECTION_PROPERTY, NULL);
	bool ok;
	double version = 0;
	ok = starbook_get_version(device, &version);
	if (ok) {
		PRIVATE_DATA->version = version;
		PRIVATE_DATA->errorCounter = 0;

		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, true);
	} else {
		PRIVATE_DATA->version = 0;
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		return false;
	}
	return true;
}


static void starbook_close(indigo_device *device) {
	return;
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->device_count > 0) {
		// MOUNT_EQUATORIAL_COORDINATES
		double ra, dec;
		int now_on_goto, state;
		if (starbook_get_status(device, &ra, &dec, &now_on_goto, &state)) {
			PRIVATE_DATA->resetWaiting = false;
			PRIVATE_DATA->currentRA = ra;
			PRIVATE_DATA->currentDec = dec;
			PRIVATE_DATA->isBusy = now_on_goto;
			if (PRIVATE_DATA->isBusy) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			} else {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			}
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = PRIVATE_DATA->currentRA;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = PRIVATE_DATA->currentDec;
			indigo_update_coordinates(device, NULL);
		} else {
			// maybe network error
			indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
			return;
		}
		// MOUNT_TRACKING
		int track_state = 0;
		if (starbook_get_track_status(device, &track_state)) {
			bool now = MOUNT_TRACKING_OFF_ITEM->sw.value;
			bool off = track_state == STARBOOK_TRACK_STATE_STOP;
			if (now != off) {
				MOUNT_TRACKING_OFF_ITEM->sw.value = off;
				MOUNT_TRACKING_ON_ITEM->sw.value = !off;
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			}
		} else {
			// maybe network error
			indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
			return;
		}
		// MOUNT_SIDE_OF_PIER
		if (!MOUNT_SIDE_OF_PIER_PROPERTY->hidden) {
			int side = STARBOOK_PIERSIDE_UNKNOWN;
			if (starbook_get_pierside(device, &side) && side != STARBOOK_PIERSIDE_UNKNOWN) {
				bool is_east = side == STARBOOK_PIERSIDE_EAST;
				if (MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value != is_east) {
					// changed
					MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = is_east;
					MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = !is_east;
					// notify update
					indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
				}
			}
		}
		indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
	}
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		SIMULATION_PROPERTY->hidden = true;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
		MOUNT_UTC_TIME_PROPERTY->hidden = true;
		MOUNT_TRACK_RATE_PROPERTY->hidden = true;
		MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
		MOUNT_PARK_PROPERTY->count = 1;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		DEVICE_PORT_PROPERTY->hidden = false;
		strcpy(DEVICE_PORT_ITEM->text.value, STARBOOK_DEFAULT_IP_ADDR);

		// TIMEZONE
		TIMEZONE_PROPERTY = indigo_init_number_property(NULL, device->name, TIMEZONE_PROPERTY_NAME, MOUNT_SITE_GROUP, "Timezone", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (TIMEZONE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(TIMEZONE_VALUE_ITEM, TIMEZONE_VALUE_ITEM_NAME, "Timezone", -12, 12, 1, 0);

		// RESET
		RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, RESET_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Reset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (RESET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(RESET_CTRL_ITEM, RESET_CTRL_ITEM_NAME, "Reset", false);

		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(TIMEZONE_PROPERTY, property))
			indigo_define_property(device, TIMEZONE_PROPERTY, NULL);
		if (indigo_property_match(RESET_PROPERTY, property))
		indigo_define_property(device, RESET_PROPERTY, NULL);
	}
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}

static void mount_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = starbook_open(device);
		}
		if (result) {
			// MOUNT_INFO
			indigo_copy_value(MOUNT_INFO_VENDOR_ITEM->text.value, "Vixen");
			indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, "StarBook");
			char temp[128];
			sprintf(temp, "v%.02f", PRIVATE_DATA->version);
			indigo_copy_value(MOUNT_INFO_FIRMWARE_ITEM->text.value, temp);
			// MOUNT_TRACKING
			MOUNT_TRACKING_PROPERTY->perm = INDIGO_RO_PERM;
			int track_state = 0;
			if (starbook_get_track_status(device, &track_state)) {
				MOUNT_TRACKING_ON_ITEM->sw.value = track_state != STARBOOK_TRACK_STATE_STOP;
				MOUNT_TRACKING_OFF_ITEM->sw.value = track_state == STARBOOK_TRACK_STATE_STOP;
			}
			// MOUNT_GEOGRAPHIC_COORDINATES
			double lng = 0, lat = 0;
			int timezone = 0;
			if (starbook_get_place(device, &lng, &lat, &timezone)) {
				MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lng;
				MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
				TIMEZONE_VALUE_ITEM->number.value = timezone;
			}
			// MOUNT_SIDE_OF_PIER
			int side = STARBOOK_PIERSIDE_UNKNOWN;
			if (starbook_get_pierside(device, &side) && side != STARBOOK_PIERSIDE_UNKNOWN) {
				MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
				MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
			}
			// TIMEZONE
			indigo_define_property(device, TIMEZONE_PROPERTY, NULL);
			// RESET
			indigo_define_property(device, RESET_PROPERTY, NULL);

			indigo_set_timer(device, 0, position_timer_callback, &PRIVATE_DATA->position_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open serial port");
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		indigo_delete_property(device, TIMEZONE_PROPERTY, NULL);
		indigo_delete_property(device, RESET_PROPERTY, NULL);
		if (--PRIVATE_DATA->device_count == 0) {
			starbook_move(device, false, false, false, false);
			starbook_stop(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			starbook_stop(device);
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
		}
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
		double lat = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
		int timezone = TIMEZONE_VALUE_ITEM->number.value;
		if (starbook_set_place(device, lng, lat, timezone)) {
			if (starbook_get_place(device, &lng, &lat, &timezone)) {
				MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lng;
				MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
				TIMEZONE_VALUE_ITEM->number.value = timezone;
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				TIMEZONE_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "GETPLACE failed.");
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				TIMEZONE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SETPLACE failed. (%f, %f, %d)", lng, lat, timezone);
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			TIMEZONE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		indigo_update_property(device, TIMEZONE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		int error = STARBOOK_NO_ERROR;
		if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
			if (!starbook_align(device, ra, dec, &error)) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
			if (!starbook_goto_radec(device, ra, dec, &error)) {
				if (error == STARBOOK_WARNING_NEAR_SUN) {
					// retry (force continue)
					starbook_goto_radec(device, ra, dec, &error);
				}
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported MOUNT_ON_COORDINATES_SET");
		}
		if (error == STARBOOK_NO_ERROR) {
			indigo_update_coordinates(device, NULL);
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_coordinates(device, starbook_error_text(error));
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
			bool ret1 = starbook_stop(device);
			bool ret2 = starbook_move(device, false, false, false, false);
			if (ret1 && ret2) {
				MOUNT_ABORT_MOTION_ITEM->sw.value = false;
				MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
			} else {
				MOUNT_ABORT_MOTION_ITEM->sw.value = false;
				MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Failed");
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
		bool north = MOUNT_MOTION_NORTH_ITEM->sw.value;
		bool south = MOUNT_MOTION_SOUTH_ITEM->sw.value;
		bool east = MOUNT_MOTION_EAST_ITEM->sw.value;
		bool west = MOUNT_MOTION_WEST_ITEM->sw.value;
		if (starbook_move(device, north, south, east, west)) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			starbook_stop(device);
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
		bool north = MOUNT_MOTION_NORTH_ITEM->sw.value;
		bool south = MOUNT_MOTION_SOUTH_ITEM->sw.value;
		bool east = MOUNT_MOTION_EAST_ITEM->sw.value;
		bool west = MOUNT_MOTION_WEST_ITEM->sw.value;
		if (starbook_move(device, north, south, east, west)) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			starbook_stop(device);
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
			starbook_set_speed(device, 0);
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
			starbook_set_speed(device, 3);
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
			starbook_set_speed(device, 5);
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
			starbook_set_speed(device, 8);
		} else {
			MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// TODO:
	} else if (indigo_property_match_changeable(TIMEZONE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- TIMEZONE
		indigo_property_copy_values(TIMEZONE_PROPERTY, property, false);
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		TIMEZONE_PROPERTY->state = INDIGO_OK_STATE;
		double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
		double lat = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
		int timezone = TIMEZONE_VALUE_ITEM->number.value;
		if (!starbook_set_place(device, lng, lat, timezone)) {
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			TIMEZONE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_update_property(device, TIMEZONE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(RESET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- RESET
		indigo_property_copy_values(RESET_PROPERTY, property, false);
		RESET_PROPERTY->state = INDIGO_OK_STATE;
		if (RESET_CTRL_ITEM->sw.value) {
			RESET_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, RESET_PROPERTY, NULL);
			PRIVATE_DATA->resetWaiting = true;
			if (starbook_reset(device)) {
				RESET_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				RESET_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_usleep(1000 * 1000);  // 1sec
			RESET_CTRL_ITEM->sw.value = false;
			indigo_update_property(device, RESET_PROPERTY, NULL);
		}
		return INDIGO_OK;
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connect_callback(device);
	}
	indigo_release_property(TIMEZONE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = starbook_open(device->master_device);
		}
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (--PRIVATE_DATA->device_count == 0) {
			starbook_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void guider_ra_timer_callback(indigo_device *device) {
	if (GUIDER_GUIDE_EAST_ITEM->number.value != 0 || GUIDER_GUIDE_WEST_ITEM->number.value != 0) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
}

static void guider_dec_timer_callback(indigo_device *device) {
	if (GUIDER_GUIDE_NORTH_ITEM->number.value != 0 || GUIDER_GUIDE_SOUTH_ITEM->number.value != 0) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
			starbook_move_pulse(device, STARBOOK_MOVE_PULSE_NORTH, GUIDER_GUIDE_NORTH_ITEM->number.value);
			indigo_set_timer(device, GUIDER_GUIDE_NORTH_ITEM->number.value/1000.0, guider_dec_timer_callback, &PRIVATE_DATA->dec_guider_timer);
		} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
			starbook_move_pulse(device, STARBOOK_MOVE_PULSE_SOUTH, GUIDER_GUIDE_SOUTH_ITEM->number.value);
			indigo_set_timer(device, GUIDER_GUIDE_SOUTH_ITEM->number.value/1000.0, guider_dec_timer_callback, &PRIVATE_DATA->dec_guider_timer);
		} else {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
			starbook_move_pulse(device, STARBOOK_MOVE_PULSE_WEST, GUIDER_GUIDE_WEST_ITEM->number.value);
			indigo_set_timer(device, GUIDER_GUIDE_WEST_ITEM->number.value/1000.0, guider_ra_timer_callback, &PRIVATE_DATA->ra_guider_timer);
		} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
			starbook_move_pulse(device, STARBOOK_MOVE_PULSE_EAST, GUIDER_GUIDE_EAST_ITEM->number.value);
			indigo_set_timer(device, GUIDER_GUIDE_EAST_ITEM->number.value/1000.0, guider_ra_timer_callback, &PRIVATE_DATA->ra_guider_timer);
		} else {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static starbook_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_starbook(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_STARBOOK_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_STARBOOK_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Vixen StarBook Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(starbook_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			mount->master_device = mount;
			indigo_attach_device(mount);
			mount_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_guider_template);
			mount_guider->private_data = private_data;
			mount_guider->master_device = mount;
			indigo_attach_device(mount_guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(mount_guider);
			last_action = action;
			if (mount != NULL) {
				indigo_detach_device(mount);
				free(mount);
				mount = NULL;
			}
			if (mount_guider != NULL) {
				indigo_detach_device(mount_guider);
				free(mount_guider);
				mount_guider = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
