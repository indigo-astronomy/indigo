#include <stdio.h>
#include <string.h>
#include <nexstar.h>
#include <deg2str.h>

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("usage: %s SERIAL_PORT\n", argv[0]);
		printf("SERIAL_PORT can be /dev/ttyUSB0, /dev/ttyS0, /dev/cu.usbserial etc.\n");
		return 0;
	}
	
	int dev = open_telescope(argv[1]);
	if (dev < 0) {
	       	printf("Can not open device: %s\n", argv[1]);
		return 1;
	}

	/* check if the telescope is alligned */
	int aligned = tc_check_align(dev);
	if (aligned < 0) {
		printf("Communication error.\n");
		close_telescope(dev);
		return 1;
	} 
	if (!aligned) {
		printf("Telescope is not aligned. Please align it!\n");
		close_telescope(dev);
		return 1;
	}
	printf("Telescope is aligned.\n");

	/* Get Right Ascension and Declination from the telescope */
	double ra, de;
	if (tc_get_rade_p(dev, &ra, &de)) {
		printf("Communication error.\n");
		close_telescope(dev);
		return 1;
	}
	
	char ra_str[15]; 
	char de_str[15];
	/* to convert ra in to hours we have to 
	divide it by 15 then convert it to string. */
	strncpy(ra_str, dh2a(ra/15), 15);

	/* convert de to string */
	strncpy(de_str, dd2a(de,1), 15);
	printf("Telescope coordinates are:\n");
	printf("RA = %s, DE = %s\n", ra_str, de_str);

	close_telescope(dev);
}
