#include <stdio.h>
#include <string.h>
#include <indigocat/indigocat_solar_system.h>
#include <math.h>
#include <time.h>

#define DELTA_UTC_UT1 (0.0340990 / 86400.0) /* For 2025-04-19 */
#define UT2JD(t) ((t) / 86400.0 + 2440587.5 + DELTA_UTC_UT1)
#define JD_NOW UT2JD(time(NULL))


char* indigo_dtos(double value, char *format) { // circular use of 4 static buffers!
	double d = fabs(value);
	double m = 60.0 * (d - floor(d));
	double s = 60.0 * (m - floor(m));

	static char string_1[128], string_2[128], string_3[128], string_4[128], buf[128];
	static char *string = string_4;
	if (string == string_1)
		string = string_2;
	else if (string == string_2)
		string = string_3;
	else if (string == string_3)
		string = string_4;
	else if (string == string_4)
		string = string_1;
	if (format == NULL)
		snprintf(buf, 128, "%d:%02d:%05.2f", (int)d, (int)m, (int)(s*100.0)/100.0);
	else if (format[strlen(format) - 1] == 'd')
		snprintf(buf, 128, format, (int)d, (int)m, (int)s);
	else
		snprintf(buf, 128, format, (int)d, (int)m, s);
	if (value < 0) {
		if (buf[0] == '+') {
			buf[0] = '-';
			snprintf(string, 128, "%s", buf);
		} else {
			snprintf(string, 128, "-%s", buf);
		}
	} else {
		snprintf(string, 128, "%s", buf);
	}
	return string;
}


void print_planet(char *name, equatorial_coords_s *equ) {
	printf("|%12s | RA %13s | Dec %13s |\n", name, indigo_dtos(equ->ra/15, NULL), indigo_dtos(equ->dec, NULL));
}


int main (int argc, char * argv[]) {
	equatorial_coords_s equ;
	double JD = 2459747.410601;
	//JD = JD_NOW;
	printf("| JD %f\n", JD);
	printf("|-----------------------\n");

	indigocat_mercury_equatorial_coords(JD, &equ);
	print_planet("Mercury", &equ);

	indigocat_venus_equatorial_coords(JD, &equ);
	print_planet("Venus", &equ);

	indigocat_mars_equatorial_coords(JD, &equ);
	print_planet("Mars", &equ);

	indigocat_jupiter_equatorial_coords(JD, &equ);
	print_planet("Jupiter", &equ);

	indigocat_saturn_equatorial_coords(JD, &equ);
	print_planet("Saturn", &equ);

	indigocat_uranus_equatorial_coords(JD, &equ);
	print_planet("Uranus", &equ);

	indigocat_neptune_equatorial_coords(JD, &equ);
	print_planet("Neptune", &equ);

	indigocat_pluto_equatorial_coords(JD, &equ);
	print_planet("Pluto", &equ);

	indigocat_moon_equatorial_coords(JD, &equ);
	print_planet("Moon", &equ);

	indigocat_sun_equatorial_coords(JD, &equ);
	print_planet("Sun", &equ);

	return 0;
}
