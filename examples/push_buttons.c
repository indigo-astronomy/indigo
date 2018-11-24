#include <stdio.h>
#include <nexstar.h>

int simulate_push_buttons(int dev, int seconds) {
	int result;
	/* save the current trackig mode */
	int tracking_mode = tc_get_tracking_mode(dev);
	if (tracking_mode == RC_UNSUPPORTED) {
		printf("Get tracking mode in unnsupported on this mount assuming: EQ NORTH\n");
		tracking_mode = TC_TRACK_EQ_NORTH;
	} else if (tracking_mode < 0) return tracking_mode;

	/* Celestron advises to disable tracking before issuing slew
	commands at rates above 2 and reenable it after finishing */
	result = tc_set_tracking_mode(dev, TC_TRACK_OFF);
	if (result != 0) return result;

	/* start slewing at rate  7.5"/s */
	result = tc_slew_fixed(dev, TC_AXIS_DE, TC_DIR_POSITIVE, 9);
	if (result != 0) return result;

	result = tc_slew_fixed(dev, TC_AXIS_RA, TC_DIR_POSITIVE, 9);
        if (result != 0) return result;


	/* sleep several seconds */
	sleep(seconds);

	/* stop slewing (call slew with rate = 0) */
	result = tc_slew_fixed(dev, TC_AXIS_DE, TC_DIR_POSITIVE, 0);
	if (result != 0) return result;

	result = tc_slew_fixed(dev, TC_AXIS_RA, TC_DIR_POSITIVE, 0);
	if (result != 0) return result;
	
	/* reenable previous tracking mode */
	result = tc_set_tracking_mode(dev, tracking_mode);
        if (result != 0) return result;

	return 0;	
}

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

	enforce_protocol_version(dev,VER_AUTO);

	/* simulate pushing the hand control buttons for 5 seconds */
	int result = simulate_push_buttons(dev, 5);
	if (result < 0) {
		printf("Something went wrong! Error code: %d\n", result);
		close_telescope(dev);
		return 1;
	} 

	close_telescope(dev);
}
