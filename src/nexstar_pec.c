/**************************************************************
	Celestron NexStar compatible telescope control library

	(C)2013-2016 by Rumen G.Bogdanovski
***************************************************************/
#include <math.h>
#include <nexstar.h>
#include <nexstar_pec.h>


int pec_index_found(int dev) {
	char res[2];

	REQUIRE_VER(VER_AUX);
	REQUIRE_VENDOR(VNDR_CELESTRON);

	if (tc_pass_through_cmd(dev, 1, _TC_AXIS_RA_AZM, 0x18, 0, 0, 0, 1, res) < 0) {
		return RC_FAILED;
	}

	if (res[0] == 0) return 0;
	else return 1;
}


int pec_seek_index(int dev) {
	char res;

	REQUIRE_VER(VER_AUX);
	REQUIRE_VENDOR(VNDR_CELESTRON);

	if (tc_pass_through_cmd(dev, 1, _TC_AXIS_RA_AZM, 0x19, 0, 0, 0, 0, &res) < 0) {
		return RC_FAILED;
	}

	return RC_OK;
}


int pec_record(int dev, char action) {
	char res, cmd_id;

	REQUIRE_VER(VER_AUX);
	REQUIRE_VENDOR(VNDR_CELESTRON);

	if (action == PEC_START) {
		cmd_id = 0x0c;  /* Start PEC record */
	} else if (action == PEC_STOP) {
		cmd_id = 0x16;  /* Stop PEC record */
	} else {
		return RC_PARAMS;
	}

	if (tc_pass_through_cmd(dev, 1, _TC_AXIS_RA_AZM, cmd_id, 0, 0, 0, 0, &res) < 0) {
		return RC_FAILED;
	}

	return RC_OK;
}


int pec_record_complete(int dev) {
	char res[2];

	REQUIRE_VER(VER_AUX);
	REQUIRE_VENDOR(VNDR_CELESTRON);

	if (tc_pass_through_cmd(dev, 1, _TC_AXIS_RA_AZM, 0x15, 0, 0, 0, 1, res) < 0) {
		return RC_FAILED;
	}

	if (res[0] == 0) return 0;
	else return 1;
}


int pec_playback(int dev, char action) {
	char res;

	REQUIRE_VER(VER_AUX);
	REQUIRE_VENDOR(VNDR_CELESTRON);

	if ((action != PEC_START) && (action != PEC_STOP)) {
		return RC_PARAMS;
	}

	if (tc_pass_through_cmd(dev, 2, _TC_AXIS_RA_AZM, 0x0d, action, 0, 0, 0, &res) < 0) {
		return RC_FAILED;
	}

	return RC_OK;
}


int pec_get_playback_index(int dev) {
	char res[2];

	REQUIRE_VER(VER_AUX);
	REQUIRE_VENDOR(VNDR_CELESTRON);

	if (tc_pass_through_cmd(dev, 1, _TC_AXIS_RA_AZM, 0x0e, 0, 0, 0, 1, res) < 0) {
		return RC_FAILED;
	}

	return (unsigned char) res[0];
}


int pec_get_data_len(int dev) {
	char res[2];

	REQUIRE_VER(VER_AUX);
	REQUIRE_VENDOR(VNDR_CELESTRON);

	if (tc_pass_through_cmd(dev, 2, _TC_AXIS_RA_AZM, 0x30, 0x3f, 0, 0, 1, res) < 0) {
		return RC_FAILED;
	}

	return (unsigned char) res[0];
}


int _pec_set_data(int dev, unsigned char index, char data) {
	char res;

	REQUIRE_VER(VER_AUX);
	REQUIRE_VENDOR(VNDR_CELESTRON);

	if (tc_pass_through_cmd(dev, 4, _TC_AXIS_RA_AZM, 0x31, 0x40+index, data, 0, 0, &res) < 0) {
		return RC_FAILED;
	}

	return RC_OK;
}


/* on success the returned value should be threated as a signed char */
int _pec_get_data(int dev, unsigned char index) {
	char res[2];

	REQUIRE_VER(VER_AUX);
	REQUIRE_VENDOR(VNDR_CELESTRON);

	if (tc_pass_through_cmd(dev, 2, _TC_AXIS_RA_AZM, 0x30, 0x40+index, 0, 0, 1, res) < 0) {
		return RC_FAILED;
	}

	return (unsigned char) res[0];
}


int pec_set_data(int dev, float *data, int len) {
	int pec_len, i, rdiff;
	float diff, current = 0;

	pec_len = pec_get_data_len(dev);
	if (pec_len < 0) {
		return RC_FAILED;
	}

	if (pec_len != len) {
		return RC_PARAMS;
	}

	for( i=0; i<pec_len; i++) {
		diff = data[i] - current;
		/* I have no idea why the values are different for positive and negative numbers
		   I thought the coefficient should be 0.0772 arcsec/unit as the resolution of
		   24bit number for 360 degrees. I tried to mach as best as I could the values
		   returned by Celestron's PECTool and I came up with this numbers... */
		if (diff < 0) {
			rdiff = (int)roundl(data[i] / 0.0845) - (int)roundl(current / 0.0845);
		} else {
			rdiff = (int)roundl(data[i] / 0.0774) - (int)roundl(current / 0.0774);
		}

		if ((rdiff > 127) || (rdiff < -127)) {
			return RC_DATA;
		}

		if (_pec_set_data(dev, i, (char)rdiff) < 0) {
			return RC_FAILED;
		}
		current = data[i];
	}

	if (pec_record(dev, PEC_STOP) < 0) {
		return RC_FAILED;
	}
	return RC_OK;
}


int pec_get_data(int dev, float *data, const int max_len) {
	int pec_len, i, diff;
	float current = 0;
	char rdata;

	pec_len = pec_get_data_len(dev);
	if (pec_len < 0) {
		return RC_FAILED;
	}

	if (pec_len > max_len) {
		return RC_PARAMS;
	}

	for (i = 0; i < pec_len; i++) {
		diff = _pec_get_data(dev, i);
		if (diff < 0) {
			return RC_FAILED;
		}
		/* we need thedata as a signed char */
		rdata = (char)diff;

		/* I have no idea why the values are different for positive and negative numbers.
		   Pease se the note in pec_set_data()! */
		if (rdata > 0) {
			current += rdata * 0.0774;
		} else {
			current += rdata * 0.0845;
		}
		data[i] = current;
	}
	return pec_len;
}
