/**************************************************************
        Celestron NexStar compatible telescope control library
        
        (C)2013-2016 by Rumen G.Bogdanovski
***************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nex_open.h>


int parse_devname(char *device, char *host, int *port) {
	char *strp;
	int n;

	n=sscanf(device,"tcp://%s",host);
	if (n < 1) return 0;
	strp = host;	
	strsep(&strp, ":");
	if (strp == NULL) {
		*port = DEFAULT_PORT;
	} else {
		*port = atoi(strp);
	}
	return 1;
}


int open_telescope_rs(char *dev_file) {
	int dev_fd;
	struct termios options;

	if ((dev_fd = open(dev_file, O_RDWR | O_NOCTTY | O_SYNC))==-1) {
		return -1;
	}

	memset(&options, 0, sizeof options);
	if (tcgetattr(dev_fd, &options) != 0) {
		close(dev_fd);
		return -1;
	}

	cfsetispeed(&options,B9600);
	cfsetospeed(&options,B9600);
	/* Finaly!!!!  Works on Linux & Solaris  */
	options.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG|IEXTEN);
	options.c_oflag &= ~(OPOST);
	options.c_iflag &= ~(INLCR|ICRNL|IXON|IXOFF|IXANY|IMAXBEL);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cc[VMIN]  = 0;	// read doesn't block
	options.c_cc[VTIME] = 50;	// 5 seconds read timeout

	if (tcsetattr(dev_fd,TCSANOW, &options) != 0) {
		close(dev_fd);
		return -1;
	}

	return dev_fd;
}


int open_telescope_tcp(char *host, int port) {
	struct sockaddr_in srv_info;
	struct hostent *he;
	int sock;
	struct timeval timeout;      

	timeout.tv_sec = 5;
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
