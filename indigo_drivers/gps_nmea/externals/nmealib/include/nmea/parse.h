/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: parse.h 4 2007-08-27 13:11:03Z xtimor $
 *
 */

#ifndef __NMEA_PARSE_H__
#define __NMEA_PARSE_H__

#include "sentence.h"

#ifdef  __cplusplus
extern "C" {
#endif

int nmea_pack_talker(const char *buff, int buff_sz);
int nmea_pack_type(const char *buff, int buff_sz);
int nmea_find_tail(const char *buff, int buff_sz, int *res_crc);

int nmea_parse_GGA(const char *buff, int buff_sz, nmeaGGA *pack);
int nmea_parse_GSA(const char *buff, int buff_sz, nmeaGSA *pack);
int nmea_parse_GSV(const char *buff, int buff_sz, nmeaGSV *pack);
int nmea_parse_RMC(const char *buff, int buff_sz, nmeaRMC *pack);
int nmea_parse_VTG(const char *buff, int buff_sz, nmeaVTG *pack);
int nmea_parse_ZDA(const char *buff, int buff_sz, nmeaZDA *pack);
int nmea_parse_GLL(const char *buff, int buff_sz, nmeaGLL *pack);

void nmea_GGA2info(nmeaGGA *pack, nmeaINFO *info);
void nmea_GSA2info(nmeaGSA *pack, nmeaINFO *info);
void nmea_GSV2info(nmeaGSV *pack, nmeaINFO *info);
void nmea_RMC2info(nmeaRMC *pack, nmeaINFO *info);
void nmea_VTG2info(nmeaVTG *pack, nmeaINFO *info);
void nmea_ZDA2info(nmeaZDA *pack, nmeaINFO *info);
void nmea_GLL2info(nmeaGLL *pack, nmeaINFO *info);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_PARSE_H__ */
