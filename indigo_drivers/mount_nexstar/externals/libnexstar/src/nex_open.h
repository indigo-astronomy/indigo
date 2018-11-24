/**************************************************************
	Celestron NexStar compatible telescope control library
	
	(C)2013-2016 by Rumen G.Bogdanovski
***************************************************************/
#if !defined(__NEX_OPEN_H)
#define __NEX_OPEN_H

#define DEFAULT_PORT 9999

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

int parse_devname(char *device, char *host, int *port);
int open_telescope_rs(char *dev_file);
int open_telescope_tcp(char *host, int port);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __NEX_OPEN_H */
