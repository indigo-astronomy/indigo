/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: generate.c 17 2008-03-11 11:56:11Z xtimor $
 *
 */

#include "nmea/tok.h"
#include "nmea/sentence.h"
#include "nmea/generate.h"
#include "nmea/units.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

int nmea_gen_GGA(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaGGA *pack)
{
    return nmea_printf(buff, buff_sz,
        "$%2sGGA,%02d%02d%02d.%02d,%07.4f,%C,%07.4f,%C,%1d,%02d,%03.1f,%03.1f,%C,%03.1f,%C,%03.1f,%04d",
        nmeaTalkers[talker],
        pack->utc.hour, pack->utc.min, pack->utc.sec, pack->utc.hsec,
        pack->lat, pack->ns, pack->lon, pack->ew,
        pack->sig, pack->satinuse, pack->HDOP, pack->elv, pack->elv_units,
        pack->diff, pack->diff_units, pack->dgps_age, pack->dgps_sid);
}

int nmea_gen_GSA(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaGSA *pack)
{
    return nmea_printf(buff, buff_sz,
        "$%2sGSA,%C,%1d,%02d,%02d,%02d,%02d,%02d,%02d,%02d,%02d,%02d,%02d,%02d,%02d,%03.1f,%03.1f,%03.1f",
        nmeaTalkers[talker],
        pack->fix_mode, pack->fix_type,
        pack->sat_prn[0], pack->sat_prn[1], pack->sat_prn[2], pack->sat_prn[3], pack->sat_prn[4], pack->sat_prn[5],
        pack->sat_prn[6], pack->sat_prn[7], pack->sat_prn[8], pack->sat_prn[9], pack->sat_prn[10], pack->sat_prn[11],
        pack->PDOP, pack->HDOP, pack->VDOP);
}

int nmea_gen_GSV(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaGSV *pack)
{
    return nmea_printf(buff, buff_sz,
        "$%2sGSV,%1d,%1d,%02d,"
        "%02d,%02d,%03d,%02d,"
        "%02d,%02d,%03d,%02d,"
        "%02d,%02d,%03d,%02d,"
        "%02d,%02d,%03d,%02d",
        nmeaTalkers[talker],
        pack->pack_count, pack->pack_index + 1, pack->sat_count,
        pack->sat_data[0].id, pack->sat_data[0].elv, pack->sat_data[0].azimuth, pack->sat_data[0].sig,
        pack->sat_data[1].id, pack->sat_data[1].elv, pack->sat_data[1].azimuth, pack->sat_data[1].sig,
        pack->sat_data[2].id, pack->sat_data[2].elv, pack->sat_data[2].azimuth, pack->sat_data[2].sig,
        pack->sat_data[3].id, pack->sat_data[3].elv, pack->sat_data[3].azimuth, pack->sat_data[3].sig);
}

int nmea_gen_RMC(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaRMC *pack)
{
    return nmea_printf(buff, buff_sz,
        "$%2sRMC,%02d%02d%02d.%02d,%C,%07.4f,%C,%07.4f,%C,%03.1f,%03.1f,%02d%02d%02d,%03.1f,%C,%C",
        nmeaTalkers[talker],
        pack->utc.hour, pack->utc.min, pack->utc.sec, pack->utc.hsec,
        pack->status, pack->lat, pack->ns, pack->lon, pack->ew,
        pack->speed, pack->direction,
        pack->utc.day, pack->utc.mon + 1, pack->utc.year - 100,
        pack->declination, pack->declin_ew, pack->mode);
}

int nmea_gen_VTG(char *buff, int buff_sz, nmeaPACKTALKER talker, nmeaVTG *pack)
{
    return nmea_printf(buff, buff_sz,
        "$%2sVTG,%.1f,%C,%.1f,%C,%.1f,%C,%.1f,%C",
        nmeaTalkers[talker],
        pack->dir, pack->dir_t,
        pack->dec, pack->dec_m,
        pack->spn, pack->spn_n,
        pack->spk, pack->spk_k);
}

void nmea_info2GGA(const nmeaINFO *info, nmeaGGA *pack)
{
    nmea_zero_GGA(pack);

    pack->utc = info->utc;
    pack->lat = fabs(info->lat);
    pack->ns = ((info->lat > 0)?'N':'S');
    pack->lon = fabs(info->lon);
    pack->ew = ((info->lon > 0)?'E':'W');
    pack->sig = info->sig;
    pack->satinuse = info->satinfo.inuse;
    pack->HDOP = info->HDOP;
    pack->elv = info->elv;
}

void nmea_info2GSA(const nmeaINFO *info, nmeaGSA *pack)
{
    int it;

    nmea_zero_GSA(pack);

    pack->fix_type = info->fix;
    pack->PDOP = info->PDOP;
    pack->HDOP = info->HDOP;
    pack->VDOP = info->VDOP;

    for(it = 0; it < NMEA_MAXSAT; ++it)
    {
        pack->sat_prn[it] =
            ((info->satinfo.sat[it].in_use)?info->satinfo.sat[it].id:0);
    }
}

int nmea_gsv_npack(int sat_count)
{
    int pack_count = (int)ceil(((double)sat_count) / NMEA_SATINPACK);

    if(0 == pack_count)
        pack_count = 1;

    return pack_count;
}

void nmea_info2GSV(const nmeaINFO *info, nmeaGSV *pack, int pack_idx)
{
    int sit, pit;

    nmea_zero_GSV(pack);

    pack->sat_count = (info->satinfo.inview <= NMEA_MAXSAT)?info->satinfo.inview:NMEA_MAXSAT;
    pack->pack_count = nmea_gsv_npack(pack->sat_count);

    if(pack->pack_count == 0)
        pack->pack_count = 1;

    if(pack_idx >= pack->pack_count)
        pack->pack_index = pack_idx % pack->pack_count;
    else
        pack->pack_index = pack_idx;

    for(pit = 0, sit = pack->pack_index * NMEA_SATINPACK; pit < NMEA_SATINPACK; ++pit, ++sit)
        pack->sat_data[pit] = info->satinfo.sat[sit];
}

void nmea_info2RMC(const nmeaINFO *info, nmeaRMC *pack)
{
    nmea_zero_RMC(pack);

    pack->utc = info->utc;
    pack->status = ((info->sig > 0)?'A':'V');
    pack->lat = fabs(info->lat);
    pack->ns = ((info->lat > 0)?'N':'S');
    pack->lon = fabs(info->lon);
    pack->ew = ((info->lon > 0)?'E':'W');
    pack->speed = info->speed / NMEA_TUD_KNOTS;
    pack->direction = info->direction;
    pack->declination = info->declination;
    pack->declin_ew = 'E';
    pack->mode = ((info->sig > 0)?'A':'N');
}

void nmea_info2VTG(const nmeaINFO *info, nmeaVTG *pack)
{
    nmea_zero_VTG(pack);

    pack->dir = info->direction;
    pack->dec = info->declination;
    pack->spn = info->speed / NMEA_TUD_KNOTS;
    pack->spk = info->speed;
}

int nmea_generate(
    char *buff, int buff_sz,
    const nmeaINFO *info,
    int generate_mask
    )
{
    int gen_count = 0, gsv_it, gsv_count;
    int pack_mask = generate_mask;
    int talker = TK_GP;

    nmeaGGA gga;
    nmeaGSA gsa;
    nmeaGSV gsv;
    nmeaRMC rmc;
    nmeaVTG vtg;

    if(!buff)
        return 0;

    while(pack_mask)
    {
        if(pack_mask & TP_GGA)
        {
            nmea_info2GGA(info, &gga);
            gen_count += nmea_gen_GGA(buff + gen_count, buff_sz - gen_count, talker, &gga);
            pack_mask &= ~TP_GGA;
        }
        else if(pack_mask & TP_GSA)
        {
            nmea_info2GSA(info, &gsa);
            gen_count += nmea_gen_GSA(buff + gen_count, buff_sz - gen_count, talker, &gsa);
            pack_mask &= ~TP_GSA;
        }
        else if(pack_mask & TP_GSV)
        {
            gsv_count = nmea_gsv_npack(info->satinfo.inview);
            for(gsv_it = 0; gsv_it < gsv_count && buff_sz - gen_count > 0; ++gsv_it)
            {
                nmea_info2GSV(info, &gsv, gsv_it);
                gen_count += nmea_gen_GSV(buff + gen_count, buff_sz - gen_count, talker, &gsv);
            }
            pack_mask &= ~TP_GSV;
        }
        else if(pack_mask & TP_RMC)
        {
            nmea_info2RMC(info, &rmc);
            gen_count += nmea_gen_RMC(buff + gen_count, buff_sz - gen_count, talker, &rmc);
            pack_mask &= ~TP_RMC;
        }
        else if(pack_mask & TP_VTG)
        {
            nmea_info2VTG(info, &vtg);
            gen_count += nmea_gen_VTG(buff + gen_count, buff_sz - gen_count, talker, &vtg);
            pack_mask &= ~TP_VTG;
        }
        else
            break;

        if(buff_sz - gen_count <= 0)
            break;
    }

    return gen_count;
}
