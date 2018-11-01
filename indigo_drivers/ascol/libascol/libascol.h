/**************************************************************
	ASCOL telescope control library

	(C)2018 by Rumen G.Bogdanovski
***************************************************************/
#if !defined(__LIBASCOL_H)
#define __LIBASCOL_H

#define DEFAULT_PORT 2001

#include<string.h>
#include<unistd.h>

#define ASCOL_OFF             (0)
#define ASCOL_ON              (1)

#define ASCOL_OK              (0)
#define ASCOL_READ_ERROR      (1)
#define ASCOL_WRITE_ERROR     (2)
#define ASCOL_COMMAND_ERROR   (3)
#define ASCOL_RESPONCE_ERROR  (4)

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

int dms2dd(double *dd, const char *dms);
int hms2dd(double *dd, const char *hms);

int parse_devname(char *device, char *host, int *port);
int open_telescope(char *host, int port);
int close_telescope(int devfd);
int read_telescope(int devfd, char *reply, int len);
#define write_telescope(dev_fd, buf) (write(dev_fd, buf, strlen(buf)))
#define write_telescope_s(dev_fd, buf, size) (write(dev_fd, buf, size))
int ascol_int_param_cmd(int fd, char *cmd, int param);
int ascol_double_param_cmd(int fd, char *cmd, double param, int precision);


int ascol_GLLG(int fd, char *password);

/* Telescope Commands */

#define ascol_TEON(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TEON", on))
#define ascol_TETR(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TETR", on))
#define ascol_TEHC(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TEHC", on))
#define ascol_TEDC(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TEDC", on))

#define ascol_TGRA(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TGRA", on))
#define ascol_TGRR(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TGRR", on))

#define ascol_TGHA(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TGHA", on))
#define ascol_TGHR(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TGHR", on))

#define ascol_TSCS(dev_fd, model) (ascol_int_param_cmd(dev_fd, "TSCS", model))

#define ascol_TSCA(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TSCA", on))
#define ascol_TSCP(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TSCP", on))
#define ascol_TSCR(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TSCR", on))
#define ascol_TSGM(dev_fd, on) (ascol_int_param_cmd(dev_fd, "TSGM", on))

#define ascol_TSS1(dev_fd, speed) (ascol_double_param_cmd(dev_fd, "TSS1", speed, 2))
#define ascol_TSS2(dev_fd, speed) (ascol_double_param_cmd(dev_fd, "TSS2", speed, 2))
#define ascol_TSS3(dev_fd, speed) (ascol_double_param_cmd(dev_fd, "TSS3", speed, 2))

int ascol_TRRD(int fd, double *ra, double *de, char *east);
int ascol_TRHD(int fd, double *ha, double *de);

/* Focuser Commands */


/* Dome Commands */

#define ascol_DOON(dev_fd, on) (ascol_int_param_cmd(dev_fd, "DOON", on))
#define ascol_DOSO(dev_fd, on) (ascol_int_param_cmd(dev_fd, "DOSO", on))


/* Flap commands */

#define ascol_FTOC(dev_fd, on) (ascol_int_param_cmd(dev_fd, "FTOC", on))
#define ascol_FCOC(dev_fd, on) (ascol_int_param_cmd(dev_fd, "FCOC", on))

/* Oil Commands */

#define ascol_OION(dev_fd, on) (ascol_int_param_cmd(dev_fd, "OION", on))


#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __LIBASCOL_H */
