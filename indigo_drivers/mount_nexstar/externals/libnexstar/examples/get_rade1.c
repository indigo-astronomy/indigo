#include <stdio.h>
#include <nexstar.h>

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
	printf("Telescope coordinates are:\n");
	printf("RA = %f, DE = %f\n", ra, de);
	close_telescope(dev);
}
