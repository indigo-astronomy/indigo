/**************************************************************
	ASCOL telescope control library

	(C)2018 by Rumen G.Bogdanovski
***************************************************************/
#if !defined(__LIBASCOL_H)
#define __LIBASCOL_H

#define DEFAULT_PORT 2001

#include<string.h>
#include<unistd.h>

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

int parse_devname(char *device, char *host, int *port);
int open_telescope(char *host, int port);
int close_telescope(int devfd);
int read_telescope(int devfd, char *reply, int len);
#define write_telescope(dev_fd, buf) (write(dev_fd, buf, strlen(buf)))
#define write_telescope_s(dev_fd, buf, size) (write(dev_fd, buf, size))

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __LIBASCOL_H */
