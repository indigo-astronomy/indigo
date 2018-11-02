/**************************************************************
	ASCOL telescope control library

	(C)2018 by Rumen G.Bogdanovski
***************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libascol.h>


static size_t strncpy_n(char *dest, const char *src, size_t n){
	size_t i;

	for (i = 0 ; i < n && src[i] != '\0' ; i++)
		dest[i] = src[i];

	if (i + 1 < n) dest[i+1] = '\0';

	return i;
 }


int parse_devname(char *device, char *host, int *port) {
	char *strp;
	int n;

	n = sscanf(device,"tcp://%s",host);
	if (n < 1) {
		n = sscanf(device,"ascol://%s",host);
		if (n < 1) {
			return 0;
		}
	}

	strp = host;
	strsep(&strp, ":");
	if (strp == NULL) {
		*port = DEFAULT_PORT;
	} else {
		*port = atoi(strp);
	}
	return 1;
}


int open_telescope(char *host, int port) {
	struct sockaddr_in srv_info;
	struct hostent *he;
	int sock;
	struct timeval timeout;

	timeout.tv_sec = 6;
	timeout.tv_usec = 0;

	if ((he = gethostbyname(host))==NULL) {
		return -1;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0))== -1) {
		return -1;
	}

	memset(&srv_info, 0, sizeof(srv_info));
	srv_info.sin_family = AF_INET;
	srv_info.sin_port = htons(port);
	srv_info.sin_addr = *((struct in_addr *)he->h_addr);
	if (connect(sock, (struct sockaddr *)&srv_info, sizeof(struct sockaddr))<0) {
		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		close(sock);
		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		close(sock);
		return -1;
	}
	return sock;
}


int close_telescope(int devfd) {
	return close(devfd);
}


int read_telescope(int devfd, char *reply, int len) {
	char c;
	int res;
	int count=0;

	while ((count < len) && ((res=read(devfd,&c,1)) != -1 )) {
		if (res == 1) {
			reply[count] = c;
			count++;
			if ((c == '\n') || (c == '\r')) {
				reply[count-1] = '\0';
				return count;
			}
		} else {
			return -1;
		}
	}
	return -1;
}


int dms2dd(double *dd, const char *dms) {
	int i;
	double deg, min, sec, sign = 1;
	char *buff, *b1;
	char buff1[3];

	buff = (char*)dms;    //clear the spaces
	while (isspace(buff[0])) buff++;
	i = strlen(buff)-1;
	while (isspace(buff[i])) i--;
	buff[i+1] = '\0';

	if (buff[0] == '-') { sign = -1; buff++; }
	if (buff[0] == '+') buff++;

	if (2 != strncpy_n(buff1, buff, 2)) return -1;
	buff1[2] = '\0';
	buff += 2;
	deg = (double)strtoul(buff1, &b1, 10);
	if((buff[0] == '\0') || (b1[0] != '\0')) return -1;

	if (2 != strncpy_n(buff1, buff, 2)) return -1;
	buff1[2] = '\0';
	buff += 2;
	min = (double)strtoul(buff1, &b1, 10);
	if ((buff[0] == '\0') || (b1[0] != '\0')) return -1;

	if ((buff = (char*)strtok(buff, "\0")) == NULL) return -1;
	if ((2 != strchr(buff, '.') - buff) && (2 != strlen(buff))) return -1;
	sec = (double)strtod(buff, &b1);
	if ((buff[0] == '\0') || (b1[0] != '\0')) return -1;

	if((min >= 60) || (min < 0) || (sec >= 60) || (sec < 0)) return -1;

	*dd = sign * (deg + min/60 + sec/3600);

	return 0;
}


int hms2dd(double *dd, const char *hms) {
	int i;
	double hour, min, sec, sign = 1;
	char *buff, *b1;
	char buff1[3];

	buff = (char*)hms;    //clear the spaces
	while (isspace(buff[0])) buff++;
	i = strlen(buff)-1;
	while (isspace(buff[i])) i--;
	buff[i+1] = '\0';

	if (2 != strncpy_n(buff1, buff, 2)) return -1;
	buff1[2] = '\0';
	buff += 2;
	hour = (double)strtoul(buff1, &b1, 10);
	if((buff[0] == '\0') || (b1[0] != '\0')) return -1;

	if (2 != strncpy_n(buff1, buff, 2)) return -1;
	buff1[2] = '\0';
	buff += 2;
	min = (double)strtoul(buff1, &b1, 10);
	if ((buff[0] == '\0') || (b1[0] != '\0')) return -1;

	if ((buff = (char*)strtok(buff, "\0")) == NULL) return -1;
	if ((2 != strchr(buff, '.') - buff) && (2 != strlen(buff))) return -1;
	sec = (double)strtod(buff, &b1);
	if ((buff[0] == '\0') || (b1[0] != '\0')) return -1;

	if((hour < 0) || (hour >= 24) || (min >= 60) || (min < 0) || (sec >= 60) || (sec < 0))
		return -1;

	*dd = (hour + min/60 + sec/3600) * 15.0;

	return 0;
}

int ascol_GLLG(int devfd, char *password) {
	char cmd[80] = {0};
	char resp[80] = {0};

	sprintf(cmd, "GLLG %s\n", password);
	int res = write_telescope(devfd, cmd);
	printf("%s() -> %2d %s", __FUNCTION__, res, cmd);
	if (res != strlen(cmd)) return ASCOL_WRITE_ERROR;

	res = read_telescope(devfd, resp, 80);
	printf("%s() <- %2d %s\n", __FUNCTION__, res, resp);
	if (res <= 0) return ASCOL_READ_ERROR;

	if (strcmp("1",resp)) return ASCOL_COMMAND_ERROR;

	return ASCOL_OK;
}

int ascol_0_param_cmd(int devfd, char *cmd_name) {
	char cmd[80] = {0};
	char resp[80] = {0};

	snprintf(cmd, 80, "%s\n", cmd_name);
	int res = write_telescope(devfd, cmd);
	printf("%s() -> %2d %s", __FUNCTION__, res, cmd);
	if (res != strlen(cmd)) return ASCOL_WRITE_ERROR;

	res = read_telescope(devfd, resp, 80);
	printf("%s() <- %2d %s\n", __FUNCTION__, res, resp);
	if (res <= 0) return ASCOL_READ_ERROR;

	if (strcmp("1",resp)) return ASCOL_COMMAND_ERROR;

	return ASCOL_OK;
}

int ascol_1_int_param_cmd(int devfd, char *cmd_name, int param) {
	char cmd[80] = {0};
	char resp[80] = {0};

	snprintf(cmd, 80, "%s %d\n", cmd_name, param);
	int res = write_telescope(devfd, cmd);
	printf("%s() -> %2d %s", __FUNCTION__, res, cmd);
	if (res != strlen(cmd)) return ASCOL_WRITE_ERROR;

	res = read_telescope(devfd, resp, 80);
	printf("%s() <- %2d %s\n", __FUNCTION__, res, resp);
	if (res <= 0) return ASCOL_READ_ERROR;

	if (strcmp("1",resp)) return ASCOL_COMMAND_ERROR;

	return ASCOL_OK;
}

int ascol_1_double_param_cmd(int devfd, char *cmd_name, double param, int precision) {
	char cmd[80] = {0};
	char resp[80] = {0};

	snprintf(cmd, 80, "%s %.*f\n", cmd_name, precision, param);
	int res = write_telescope(devfd, cmd);
	printf("%s() -> %2d %s", __FUNCTION__, res, cmd);
	if (res != strlen(cmd)) return ASCOL_WRITE_ERROR;

	res = read_telescope(devfd, resp, 80);
	printf("%s() <- %2d %s\n", __FUNCTION__, res, resp);
	if (res <= 0) return ASCOL_READ_ERROR;

	if (strcmp("1",resp)) return ASCOL_COMMAND_ERROR;

	return ASCOL_OK;
}


int ascol_2_double_param_cmd(int devfd, char *cmd_name, double param1, int precision1, double param2, int precision2) {
	char cmd[80] = {0};
	char resp[80] = {0};

	snprintf(cmd, 80, "%s %.*f %.*f\n", cmd_name, precision1, param1, precision2, param2);
	int res = write_telescope(devfd, cmd);
	printf("%s() -> %2d %s", __FUNCTION__, res, cmd);
	if (res != strlen(cmd)) return ASCOL_WRITE_ERROR;

	res = read_telescope(devfd, resp, 80);
	printf("%s() <- %2d %s\n", __FUNCTION__, res, resp);
	if (res <= 0) return ASCOL_READ_ERROR;

	if (strcmp("1",resp)) return ASCOL_COMMAND_ERROR;

	return ASCOL_OK;
}


int ascol_2_double_1_int_param_cmd(int devfd, char *cmd_name, double param1, int precision1, double param2, int precision2, int east) {
	char cmd[80] = {0};
	char resp[80] = {0};

	snprintf(cmd, 80, "%s %.*f %.*f %d\n", cmd_name, precision1, param1, precision2, param2, east);
	int res = write_telescope(devfd, cmd);
	printf("%s() -> %2d %s", __FUNCTION__, res, cmd);
	if (res != strlen(cmd)) return ASCOL_WRITE_ERROR;

	res = read_telescope(devfd, resp, 80);
	printf("%s() <- %2d %s\n", __FUNCTION__, res, resp);
	if (res <= 0) return ASCOL_READ_ERROR;

	if (strcmp("1",resp)) return ASCOL_COMMAND_ERROR;

	return ASCOL_OK;
}


int ascol_TRRD(int devfd, double *ra, double *de, char *east) {
	const char cmd[] = "TRRD\n";
	char resp[80] = {0};
	char ra_s[80];
	char de_s[80];
	int east_c;

	int res = write_telescope(devfd, cmd);
	printf("%s() -> %2d %s", __FUNCTION__, res, cmd);
	if (res != strlen(cmd)) return ASCOL_WRITE_ERROR;

	res = read_telescope(devfd, resp, 80);
	printf("%s() <- %2d %s\n", __FUNCTION__, res, resp);
	if (res <= 0) return ASCOL_READ_ERROR;

	res = sscanf(resp, "%s %s %d", ra_s, de_s, &east_c);
	if (res != 3) return ASCOL_RESPONCE_ERROR;

	res = 0;
	if (ra) res = hms2dd(ra, ra_s);
	if (res) return ASCOL_RESPONCE_ERROR;

	if (de) res = dms2dd(de, de_s);
	if (res) return ASCOL_RESPONCE_ERROR;

	if (east) *east = east_c;

	printf("%s() == %2d return: %lf %lf %d\n", __FUNCTION__, ASCOL_OK, *ra, *de, *east);
	return ASCOL_OK;
}


int ascol_1_double_return_cmd(int devfd, char *cmd_name, double *val) {
	char cmd[80] = {0};
	char resp[80] = {0};
	double buf;

	snprintf(cmd, 80, "%s\n", cmd_name);
	int res = write_telescope(devfd, cmd);
	printf("%s() -> %2d %s", __FUNCTION__, res, cmd);
	if (res != strlen(cmd)) return ASCOL_WRITE_ERROR;

	res = read_telescope(devfd, resp, 80);
	printf("%s() <- %2d %s\n", __FUNCTION__, res, resp);
	if (res <= 0) return ASCOL_READ_ERROR;

	res = sscanf(resp, "%lf", &buf);
	if (res != 1) return ASCOL_RESPONCE_ERROR;

	if (val) *val = buf;

	printf("%s() == %2d return: %lf\n", __FUNCTION__, ASCOL_OK, *val);
	return ASCOL_OK;
}


int ascol_2_double_return_cmd(int devfd, char *cmd_name, double *val1, double *val2) {
	char cmd[80] = {0};
	char resp[80] = {0};
	double buf1;
	double buf2;

	snprintf(cmd, 80, "%s\n", cmd_name);
	int res = write_telescope(devfd, cmd);
	printf("%s() -> %2d %s", __FUNCTION__, res, cmd);
	if (res != strlen(cmd)) return ASCOL_WRITE_ERROR;

	res = read_telescope(devfd, resp, 80);
	printf("%s() <- %2d %s\n", __FUNCTION__, res, resp);
	if (res <= 0) return ASCOL_READ_ERROR;

	res = sscanf(resp, "%lf %lf", &buf1, &buf2);
	if (res != 2) return ASCOL_RESPONCE_ERROR;

	if (val1) *val1 = buf1;
	if (val2) *val2 = buf2;

	printf("%s() == %2d return: %lf %lf\n", __FUNCTION__, ASCOL_OK, *val1, *val2);
	return ASCOL_OK;
}
