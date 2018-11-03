/**************************************************************
	ASCOL telescope control library

	(C)2018 by Rumen G.Bogdanovski
***************************************************************/
#if !defined(__LIBASCOL_H)
#define __LIBASCOL_H

#define DEFAULT_PORT 2001

#include<string.h>
#include<unistd.h>
#include<stdint.h>

#define ASCOL_LEN            (80)

#define ASCOL_OFF             (0)
#define ASCOL_ON              (1)

#define ASCOL_OK              (0)
#define ASCOL_READ_ERROR      (1)
#define ASCOL_WRITE_ERROR     (2)
#define ASCOL_COMMAND_ERROR   (3)
#define ASCOL_RESPONCE_ERROR  (4)
#define ASCOL_PARAM_ERROR     (5)

#define CHECK_BIT(bitmap, bit) (((bitmap) >> (bit)) & 1)

/* ALARM_BITS */
#define ALARM_0_BIT_0	(0)
#define ALARM_0_BIT_1	(1)
#define ALARM_0_BIT_2	(2)
#define ALARM_0_BIT_3	(3)
#define ALARM_0_BIT_4	(4)
#define ALARM_0_BIT_5	(5)
#define ALARM_0_BIT_6	(6)
#define ALARM_0_BIT_7	(7)
#define ALARM_0_BIT_8	(8)
#define ALARM_0_BIT_9	(9)
#define ALARM_0_BIT_10	(10)
#define ALARM_0_BIT_11	(11)
#define ALARM_0_BIT_12	(12)
#define ALARM_0_BIT_13	(13)
#define ALARM_0_BIT_14	(14)
#define ALARM_0_BIT_15	(15)

#define ALARM_1_BIT_0	(16)
#define ALARM_1_BIT_1	(17)
#define ALARM_1_BIT_2	(18)
#define ALARM_1_BIT_3	(19)
/* 4 UNUSED */
#define ALARM_1_BIT_5	(21)
#define ALARM_1_BIT_6	(22)
#define ALARM_1_BIT_7	(23)
#define ALARM_1_BIT_8	(24)
#define ALARM_1_BIT_9	(25)
#define ALARM_1_BIT_10	(26)
#define ALARM_1_BIT_11	(27)
#define ALARM_1_BIT_12	(28)
#define ALARM_1_BIT_13	(29)
#define ALARM_1_BIT_14	(30)
#define ALARM_1_BIT_15	(31)

#define ALARM_2_BIT_0	(32)
#define ALARM_2_BIT_1	(33)
#define ALARM_2_BIT_2	(34)
#define ALARM_2_BIT_3	(35)
#define ALARM_2_BIT_4	(36)
#define ALARM_2_BIT_5	(37)
#define ALARM_2_BIT_6	(38)
#define ALARM_2_BIT_7	(39)
#define ALARM_2_BIT_8	(40)
#define ALARM_2_BIT_9	(41)
#define ALARM_2_BIT_10	(42)
#define ALARM_2_BIT_11	(43)
#define ALARM_2_BIT_12	(44)
/* 13, 14 UNUSED */
#define ALARM_2_BIT_15	(47)

#define ALARM_3_BIT_0	(48)
#define ALARM_3_BIT_1	(49)
#define ALARM_3_BIT_2	(50)
#define ALARM_3_BIT_3	(51)
#define ALARM_3_BIT_4	(52)
#define ALARM_3_BIT_5	(53)
#define ALARM_3_BIT_6	(54)
#define ALARM_3_BIT_7	(55)
#define ALARM_3_BIT_8	(56)
#define ALARM_3_BIT_9	(57)
#define ALARM_3_BIT_10	(58)
#define ALARM_3_BIT_11	(59)
#define ALARM_3_BIT_12	(60)
/* 13, 14, 15 UNUSED */

#define ALARM_4_BIT_0	(64)
#define ALARM_4_BIT_1	(65)
#define ALARM_4_BIT_2	(66)
#define ALARM_4_BIT_3	(67)
#define ALARM_4_BIT_4	(68)
#define ALARM_4_BIT_5	(69)
#define ALARM_4_BIT_6	(70)
#define ALARM_4_BIT_7	(71)
#define ALARM_4_BIT_8	(72)
#define ALARM_4_BIT_9	(73)
#define ALARM_4_BIT_10	(74)
/* 11 - 15 UNUSED */


#define ASCOL_OIMV_N         (17)
typedef struct {
	double value[ASCOL_OIMV_N];
	char **description;
	char **unit;
} ascol_oimv_t;

#define ASCOL_GLME_N         (7)
typedef struct {
	double value[ASCOL_GLME_N];
	char **description;
	char **unit;
} ascol_glme_t;

#define ASCOL_GLST_N         (22)
typedef struct {
	uint16_t oil_state;
	uint16_t telescope_state;
	uint16_t ra_axis_state;
	uint16_t de_axis_state;
	uint16_t focus_state;
	// 6 -> unused
	uint16_t dome_state;
	uint16_t slit_state;
	uint16_t flap_tube_state;
	uint16_t flap_coude_state;
	// 11 -> unused
	// 12 -> unused
	// 13 -> unused
	// 14 -> unused
	uint16_t selected_model_index;
	uint16_t state_bits;
	uint16_t alarm_bits[5];
	// 22 -> unused
} ascol_glst_t;

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

/* Convert ASCOL HHMMSS.SS and DDMMSS.SS format in decimal degrees */

int ascol_dms2dd(double *dd, const char *dms);
int ascol_hms2dd(double *dd, const char *hms);

/* Check state bits */

#define IS_RA_CALIBRATED(glst)      CHECK_BIT(glst.state_bits, 0)
#define IS_DA_CALIBRATED(glst)      CHECK_BIT(glst.state_bits, 1)
#define IS_ABEARRATION_CORR(glst)   CHECK_BIT(glst.state_bits, 4)
#define IS_PRECESSION_CORR(glst)    CHECK_BIT(glst.state_bits, 5)
#define IS_NUTATION_CORR(glst)      CHECK_BIT(glst.state_bits, 6)
#define IS_ERR_MODEL_CORR(glst)     CHECK_BIT(glst.state_bits, 7)
#define IS_GUIDE_MODE_ON(glst)      CHECK_BIT(glst.state_bits, 8)
#define IS_USER1_BIT_I_ON(glst)     CHECK_BIT(glst.state_bits, 14)
#define IS_USER1_BIT_II_ON(glst)    CHECK_BIT(glst.state_bits, 15)

/* Check alarms */

#define ALARM_BRIDGE      ALARM_2_BIT_1
#define ALARM_HUMIDITY    ALARM_1_BIT_3
#define CHECK_ALARM(glst, alarm)    CHECK_BIT(glst.alarm_bits[(int)(alarm/16)], (alarm%16))

/* Utility functions */

int ascol_parse_devname(char *device, char *host, int *port);
int ascol_open(char *host, int port);
int ascol_read(int devfd, char *reply, int len);
#define ascol_write(devfd, buf) (write(devfd, buf, strlen(buf)))
#define ascol_write_s(devfd, buf, size) (write(devfd, buf, size))
#define ascol_close(devfd) (close(devfd))

/* State desctiption functions */

int ascol_get_oil_state(ascol_glst_t glst, char **long_descr, char **short_descr);
int ascol_get_telescope_state(ascol_glst_t glst, char **long_descr, char **short_descr);
int ascol_get_ra_axis_state(ascol_glst_t glst, char **long_descr, char **short_descr);
int ascol_get_de_axis_state(ascol_glst_t glst, char **long_descr, char **short_descr);
int ascol_get_focus_state(ascol_glst_t glst, char **long_descr, char **short_descr);
int ascol_get_dome_state(ascol_glst_t glst, char **long_descr, char **short_descr);

int ascol_get_slit_flap_state(uint16_t state, char **long_descr, char **short_descr);
#define ascol_get_slit_state(glst, long_descr, short_descr) (ascol_get_slit_flap_state(glst.slit_state, long_descr, short_descr))
#define ascol_get_flap_tube_state(glst, long_descr, short_descr) (ascol_get_slit_flap_state(glst.flap_tube_state, long_descr, short_descr))
#define ascol_get_flap_coude_state(glst, long_descr, short_descr) (ascol_get_slit_flap_state(glst.flap_coude_state, long_descr, short_descr))

/* COMMANDS TO ASCOL CONTROLER */

int ascol_0_param_cmd(int devfd, char *cmd_name);
int ascol_1_int_param_cmd(int devfd, char *cmd_name, int param);
int ascol_1_double_param_cmd(int devfd, char *cmd_name, double param, int precision);
int ascol_2_double_param_cmd(int devfd, char *cmd_name, double param1, int precision1, double param2, int precision2);
int ascol_2_double_1_int_param_cmd(int devfd, char *cmd_name, double param1, int precision1, double param2, int precision2, int east);

int ascol_1_double_return_cmd(int devfd, char *cmd_name, double *val);
int ascol_2_double_return_cmd(int devfd, char *cmd_name, double *val1, double *val2);

/* Global commands */

int ascol_GLLG(int devfd, char *password);
int ascol_GLME(int devfd, ascol_glme_t *glme);
int ascol_GLST(int devfd, ascol_glst_t *glst);

/* Telescope Commands */

#define ascol_TEON(devfd, on) (ascol_1_int_param_cmd(devfd, "TEON", on))
#define ascol_TETR(devfd, on) (ascol_1_int_param_cmd(devfd, "TETR", on))
#define ascol_TEHC(devfd, on) (ascol_1_int_param_cmd(devfd, "TEHC", on))
#define ascol_TEDC(devfd, on) (ascol_1_int_param_cmd(devfd, "TEDC", on))

#define ascol_TSRA(devfd, ra, de, east) (ascol_2_double_1_int_param_cmd(devfd, "TSRA", ra, 5, de, 5, east))
#define ascol_TGRA(devfd, on) (ascol_1_int_param_cmd(devfd, "TGRA", on))
#define ascol_TSRR(devfd, r_ra, r_de) (ascol_2_double_param_cmd(devfd, "TSRR", r_ra, 2, r_de, 2))
#define ascol_TGRR(devfd, on) (ascol_1_int_param_cmd(devfd, "TGRR", on))

#define ascol_TSHA(devfd, ha, de) (ascol_2_double_param_cmd(devfd, "TSHA", ha, 4, de, 4))
#define ascol_TGHA(devfd, on) (ascol_1_int_param_cmd(devfd, "TGHA", on))
#define ascol_TSHR(devfd, r_ha, r_de) (ascol_2_double_param_cmd(devfd, "TSHR", r_ha, 2, r_de, 2))
#define ascol_TGHR(devfd, on) (ascol_1_int_param_cmd(devfd, "TGHR", on))

#define ascol_TSCS(devfd, model) (ascol_1_int_param_cmd(devfd, "TSCS", model))

#define ascol_TSCA(devfd, on) (ascol_1_int_param_cmd(devfd, "TSCA", on))
#define ascol_TSCP(devfd, on) (ascol_1_int_param_cmd(devfd, "TSCP", on))
#define ascol_TSCR(devfd, on) (ascol_1_int_param_cmd(devfd, "TSCR", on))
#define ascol_TSCM(devfd, on) (ascol_1_int_param_cmd(devfd, "TSCM", on))
#define ascol_TSGM(devfd, on) (ascol_1_int_param_cmd(devfd, "TSGM", on))

#define ascol_TSS1(devfd, speed) (ascol_1_double_param_cmd(devfd, "TSS1", speed, 2))
#define ascol_TRS1(devfd, speed) (ascol_1_double_return_cmd(devfd, "TRS1", speed))
#define ascol_TSS2(devfd, speed) (ascol_1_double_param_cmd(devfd, "TSS2", speed, 2))
#define ascol_TRS2(devfd, speed) (ascol_1_double_return_cmd(devfd, "TRS2", speed))
#define ascol_TSS3(devfd, speed) (ascol_1_double_param_cmd(devfd, "TSS3", speed, 2))
#define ascol_TRS3(devfd, speed) (ascol_1_double_return_cmd(devfd, "TRS3", speed))

int ascol_TRRD(int devfd, double *ra, double *de, char *east);
#define ascol_TRHD(devfd, ha, de) (ascol_2_double_return_cmd(devfd, "TRHD", ha, de))

#define ascol_TSGV(devfd, ra_gv, de_gv) (ascol_2_double_param_cmd(devfd, "TSGV", ra_gv, 1, de_gv, 1))
#define ascol_TRGV(devfd, ra_gv, de_gv) (ascol_2_double_return_cmd(devfd, "TRGV", ra_gv, de_gv))

#define ascol_TSUS(devfd, ra_us, de_us) (ascol_2_double_param_cmd(devfd, "TSUS", ra_us, 4, de_us, 4))
#define ascol_TRUS(devfd, ra_us, de_us) (ascol_2_double_return_cmd(devfd, "TRUS", ra_us, de_us))

#define ascol_TSGC(devfd, ra_gc, de_gc) (ascol_2_double_param_cmd(devfd, "TSGC", ra_gc, 1, de_gc, 1))
#define ascol_TECE(devfd, on) (ascol_1_int_param_cmd(devfd, "TECE", on))

/* Focuser Commands */

#define ascol_FOST(devfd) (ascol_0_param_cmd(devfd, "FOST"))
#define ascol_FOGR(devfd) (ascol_0_param_cmd(devfd, "FOGR"))
#define ascol_FOGA(devfd) (ascol_0_param_cmd(devfd, "FOGA"))
#define ascol_FOSR(devfd, pos) (ascol_1_double_param_cmd(devfd, "FOSR", pos, 2))
#define ascol_FOSA(devfd, pos) (ascol_1_double_param_cmd(devfd, "FOSA", pos, 2))
#define ascol_FOPO(devfd, pos) (ascol_1_double_return_cmd(devfd, "FOPO", pos))

/* Dome Commands */

#define ascol_DOON(devfd, on) (ascol_1_int_param_cmd(devfd, "DOON", on))
#define ascol_DOSO(devfd, on) (ascol_1_int_param_cmd(devfd, "DOSO", on))
#define ascol_DOPO(devfd, pos) (ascol_1_double_return_cmd(devfd, "DOPO", pos))
#define ascol_DOST(devfd) (ascol_0_param_cmd(devfd, "DOST"))
#define ascol_DOGR(devfd) (ascol_0_param_cmd(devfd, "DOGR"))
#define ascol_DOGA(devfd) (ascol_0_param_cmd(devfd, "DOGA"))
#define ascol_DOAM(devfd) (ascol_0_param_cmd(devfd, "DOAM"))
#define ascol_DOSR(devfd, pos) (ascol_1_double_param_cmd(devfd, "DOSR", pos, 2))
#define ascol_DOSA(devfd, pos) (ascol_1_double_param_cmd(devfd, "DOSA", pos, 2))

/* Flap commands */

#define ascol_FTOC(devfd, on) (ascol_1_int_param_cmd(devfd, "FTOC", on))
#define ascol_FCOC(devfd, on) (ascol_1_int_param_cmd(devfd, "FCOC", on))

/* Oil Commands */

#define ascol_OION(devfd, on) (ascol_1_int_param_cmd(devfd, "OION", on))
int ascol_OIMV(int devfd, ascol_oimv_t *oimv);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

extern int ascol_debug;

#endif /* __LIBASCOL_H */
