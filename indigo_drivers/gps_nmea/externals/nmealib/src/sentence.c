/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: sentence.c 17 2008-03-11 11:56:11Z xtimor $
 *
 */

#include "nmea/sentence.h"

#include <string.h>

void nmea_zero_GGA(nmeaGGA *pack)
{
    memset(pack, 0, sizeof(nmeaGGA));
    nmea_time_now(&pack->utc);
    pack->ns = 'N';
    pack->ew = 'E';
    pack->elv_units = 'M';
    pack->diff_units = 'M';
}

void nmea_zero_GSA(nmeaGSA *pack)
{
    memset(pack, 0, sizeof(nmeaGSA));
    pack->fix_mode = 'A';
    pack->fix_type = NMEA_FIX_BAD;
}

void nmea_zero_GSV(nmeaGSV *pack)
{
    memset(pack, 0, sizeof(nmeaGSV));
}

void nmea_zero_RMC(nmeaRMC *pack)
{
    memset(pack, 0, sizeof(nmeaRMC));
    nmea_time_now(&pack->utc);
    pack->status = 'V';
    pack->ns = 'N';
    pack->ew = 'E';
    pack->declin_ew = 'E';
}

void nmea_zero_VTG(nmeaVTG *pack)
{
    memset(pack, 0, sizeof(nmeaVTG));
    pack->dir_t = 'T';
    pack->dec_m = 'M';
    pack->spn_n = 'N';
    pack->spk_k = 'K';
}

void nmea_zero_ZDA(nmeaZDA *pack)
{
    memset(pack, 0, sizeof(nmeaZDA));
    nmea_time_now(&pack->utc);
}

void nmea_zero_GLL(nmeaGLL *pack)
{
    memset(pack, 0, sizeof(nmeaGLL));
    nmea_time_now(&pack->utc);
    pack->status = 'V';
    pack->ns = 'N';
}
