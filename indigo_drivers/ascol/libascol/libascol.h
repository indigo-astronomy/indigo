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
#define write_telescope(devfd, buf) (write(devfd, buf, strlen(buf)))
#define write_telescope_s(devfd, buf, size) (write(devfd, buf, size))

int ascol_no_param_cmd(int devfd, char *cmd_name);
int ascol_int_param_cmd(int devfd, char *cmd_name, int param);
int ascol_double_param_cmd(int devfd, char *cmd_name, double param, int precision);


int ascol_GLLG(int devfd, char *password);

/* Telescope Commands */

#define ascol_TEON(devfd, on) (ascol_int_param_cmd(devfd, "TEON", on))
#define ascol_TETR(devfd, on) (ascol_int_param_cmd(devfd, "TETR", on))
#define ascol_TEHC(devfd, on) (ascol_int_param_cmd(devfd, "TEHC", on))
#define ascol_TEDC(devfd, on) (ascol_int_param_cmd(devfd, "TEDC", on))

#define ascol_TGRA(devfd, on) (ascol_int_param_cmd(devfd, "TGRA", on))
#define ascol_TGRR(devfd, on) (ascol_int_param_cmd(devfd, "TGRR", on))

#define ascol_TGHA(devfd, on) (ascol_int_param_cmd(devfd, "TGHA", on))
#define ascol_TGHR(devfd, on) (ascol_int_param_cmd(devfd, "TGHR", on))

#define ascol_TSCS(devfd, model) (ascol_int_param_cmd(devfd, "TSCS", model))

#define ascol_TSCA(devfd, on) (ascol_int_param_cmd(devfd, "TSCA", on))
#define ascol_TSCP(devfd, on) (ascol_int_param_cmd(devfd, "TSCP", on))
#define ascol_TSCR(devfd, on) (ascol_int_param_cmd(devfd, "TSCR", on))
#define ascol_TSGM(devfd, on) (ascol_int_param_cmd(devfd, "TSGM", on))

#define ascol_TSS1(devfd, speed) (ascol_double_param_cmd(devfd, "TSS1", speed, 2))
#define ascol_TSS2(devfd, speed) (ascol_double_param_cmd(devfd, "TSS2", speed, 2))
#define ascol_TSS3(devfd, speed) (ascol_double_param_cmd(devfd, "TSS3", speed, 2))

int ascol_TRRD(int devfd, double *ra, double *de, char *east);
int ascol_TRHD(int devfd, double *ha, double *de);

/* Focuser Commands */

#define ascol_FOST(devfd) (ascol_no_param_cmd(devfd, "FOST"))
#define ascol_FOGR(devfd) (ascol_no_param_cmd(devfd, "FOGR"))
#define ascol_FOGA(devfd) (ascol_no_param_cmd(devfd, "FOGA"))
#define ascol_FOSR(devfd, position) (ascol_double_param_cmd(devfd, "FOSR", position, 2))
#define ascol_FOSA(devfd, position) (ascol_double_param_cmd(devfd, "FOSA", position, 2))

/* Dome Commands */

#define ascol_DOON(devfd, on) (ascol_int_param_cmd(devfd, "DOON", on))
#define ascol_DOSO(devfd, on) (ascol_int_param_cmd(devfd, "DOSO", on))
#define ascol_DOST(devfd) (ascol_no_param_cmd(devfd, "DOST"))
#define ascol_DOGR(devfd) (ascol_no_param_cmd(devfd, "DOGR"))
#define ascol_DOGA(devfd) (ascol_no_param_cmd(devfd, "DOGA"))
#define ascol_DOAM(devfd) (ascol_no_param_cmd(devfd, "DOAM"))
#define ascol_DOSR(devfd, position) (ascol_double_param_cmd(devfd, "DOSR", position, 2))
#define ascol_DOSA(devfd, position) (ascol_double_param_cmd(devfd, "DOSA", position, 2))

/* Flap commands */

#define ascol_FTOC(devfd, on) (ascol_int_param_cmd(devfd, "FTOC", on))
#define ascol_FCOC(devfd, on) (ascol_int_param_cmd(devfd, "FCOC", on))

/* Oil Commands */

#define ascol_OION(devfd, on) (ascol_int_param_cmd(devfd, "OION", on))


#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __LIBASCOL_H */
