#include <nmea/nmea.h>

#include <string.h>
#include <stdio.h>

#ifdef NMEA_WIN
#   include <io.h>
#endif

void trace(const char *str, int str_size)
{
//    printf("Trace: ");
//    write(1, str, str_size);
//    printf("\n");
}
void error(const char *str, int str_size)
{
//    printf("Error: ");
//    write(1, str, str_size);
//    printf("\n");
}

int main(int argc, char **argv)
{
    nmeaINFO info;
    nmeaPARSER parser;
    FILE *file;
    char buff[2048];
    int size, it = 0;
    nmeaPOS dpos;

	if (argc > 1)
		file = fopen(argv[1], "rb");
	else
		file = fopen("gpslog.txt", "rb");

    if(!file)
        return -1;

    nmea_property()->trace_func = &trace;
    nmea_property()->error_func = &error;

    nmea_zero_INFO(&info);
    nmea_parser_init(&parser);

    /*
    while(1)
    {
    */

    while(!feof(file))
    {
        size = (int)fread(&buff[0], 1, 100, file);

        nmea_parse(&parser, &buff[0], size, &info);

        nmea_info2pos(&info, &dpos);

        printf(
            "%03d, Lat: %f, Lon: %f, Sig: %d, Fix: %d\n",
            it++, dpos.lat, dpos.lon, info.sig, info.fix
            );
    }

    fseek(file, 0, SEEK_SET);

    /*
    }
    */

    nmea_parser_destroy(&parser);
    fclose(file);

    return 0;
}
