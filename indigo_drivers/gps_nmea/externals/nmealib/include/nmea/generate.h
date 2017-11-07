/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: generate.h 4 2007-08-27 13:11:03Z xtimor $
 *
 */

#ifndef __NMEA_GENERATE_H__
#define __NMEA_GENERATE_H__

#include "sentence.h"

#ifdef  __cplusplus
extern "C" {
#endif

int     nmea_generate(
        char *buff, int buff_sz,    /* buffer */
        const nmeaINFO *info,       /* source info */
        int generate_mask           /* mask of sentence`s (e.g. GPGGA | GPGSA) */
        );

int     nmea_gen_GGA(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaGGA *pack);
int     nmea_gen_GSA(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaGSA *pack);
int     nmea_gen_GSV(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaGSV *pack);
int     nmea_gen_RMC(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaRMC *pack);
int     nmea_gen_VTG(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaVTG *pack);

void    nmea_info2GGA(const nmeaINFO *info, nmeaGGA *pack);
void    nmea_info2GSA(const nmeaINFO *info, nmeaGSA *pack);
void    nmea_info2RMC(const nmeaINFO *info, nmeaRMC *pack);
void    nmea_info2VTG(const nmeaINFO *info, nmeaVTG *pack);

int     nmea_gsv_npack(int sat_count);
void    nmea_info2GSV(const nmeaINFO *info, nmeaGSV *pack, int pack_idx);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_GENERATE_H__ */
