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
			//printf("HC: %c, %d C:%d\n", (unsigned char)reply[count-1], (unsigned char)reply[count-1], count);
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

static size_t strncpy_n(char *dest, const char *src, size_t n){
	size_t i;

	for (i = 0 ; i < n && src[i] != '\0' ; i++)
		dest[i] = src[i];

	if (i + 1 < n) dest[i+1] = '\0';

	return i;
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


int hms2dh(double *dh, const char *a) {
	int i;
	double 	hour,min,sec;
	char *buff,*b1;

	buff=(char*)a;                     //clear the spaces
	while (isspace(buff[0])) buff++;
	i=strlen(buff)-1;
	while (isspace(buff[i])) i--;
	buff[i+1]='\0';

	if ((buff=(char*)strtok(buff,":"))==NULL) return -1;
	hour=(double)strtoul(buff,&b1,10);
	if((buff[0]=='\0')||(b1[0]!='\0')) return -1;

	if ((buff=(char*)strtok(NULL,":"))==NULL) return -1;
	min=(double)strtoul(buff,&b1,10);
	if((buff[0]=='\0')||(b1[0]!='\0')) return -1;

	if ((buff=(char*)strtok(NULL,"\0"))==NULL) return -1;
	sec=(double)strtod(buff,&b1);
	if((buff[0]=='\0')||(b1[0]!='\0')) return -1;

	if((hour<0)||(hour>=24)||(min>=60)||(min<0)||(sec>=60)||(sec<0))
	   return -1;

	*dh=hour+min/60+sec/3600;

	return 0;
}
