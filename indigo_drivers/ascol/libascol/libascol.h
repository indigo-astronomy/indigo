/**************************************************************
	ASCOL telescope control library

	(C)2018 by Rumen G.Bogdanovski
***************************************************************/
#if !defined(__LIBASCOL_H)
#define __LIBASCOL_H

#define DEFAULT_PORT 2001

#include<string.h>
#include<unistd.h>

#define ASCOL_OK              (0)
#define ASCOL_READ_ERROR      (1)
#define ASCOL_WRITE_ERROR     (2)
#define ASCOL_COMMAND_ERROR   (3)
#define ASCOL_RESPONCE_ERROR  (4)

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

int parse_devname(char *device, char *host, int *port);
int open_telescope(char *host, int port);
int close_telescope(int devfd);
int read_telescope(int devfd, char *reply, int len);
#define write_telescope(dev_fd, buf) (write(dev_fd, buf, strlen(buf)))
#define write_telescope_s(dev_fd, buf, size) (write(dev_fd, buf, size))

int dms2dd(double *dd, const char *dms);
int hms2dd(double *dd, const char *hms);

int ascol_GLLG(int fd, char *password);
int ascol_TRRD(int fd, double *ra, double *de, char *east);
int ascol_TRHD(int fd, double *ha, double *de);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __LIBASCOL_H */
