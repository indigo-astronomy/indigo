/**************************************************************
	ASCOL telescope control library

	(C)2018 by Rumen G.Bogdanovski
***************************************************************/
#if !defined(__LIBASCOL_H)
#define __LIBASCOL_H

#include<string.h>
#include<unistd.h>
#include<stdint.h>

#define DEFAULT_PORT        (2001)
#define ASCOL_MSG_LEN        (100)

#define ASCOL_OFF              (0)
#define ASCOL_ON               (1)

/* Function Return Codes */
#define ASCOL_OK               (0)
#define ASCOL_READ_ERROR       (1)
#define ASCOL_WRITE_ERROR      (2)
#define ASCOL_COMMAND_ERROR    (3)
#define ASCOL_RESPONCE_ERROR   (4)
#define ASCOL_PARAM_ERROR      (5)

/* Oil States */
#define OIL_STATE_OFF          (0)
#define OIL_STATE_START1       (1)
#define OIL_STATE_START2       (2)
#define OIL_STATE_START3       (3)
#define OIL_STATE_ON           (4)
#define OIL_STATE_OFF_DELAY    (5)

/* Telescope States */
#define TE_STATE_INIT          (0)
#define TE_STATE_OFF           (1)
#define TE_STATE_OFF_WAIT      (2)
#define TE_STATE_STOP          (3)
#define TE_STATE_TRACK         (4)
#define TE_STATE_OFF_REQ       (5)
#define TE_STATE_SS_CLU1       (6)
#define TE_STATE_SS_SLEW       (7)
#define TE_STATE_SS_CLU2       (8)
#define TE_STATE_SS_CLU3       (9)
#define TE_STATE_ST_DECC1     (10)
#define TE_STATE_ST_CLU1      (11)
#define TE_STATE_ST_SLEW      (12)
#define TE_STATE_ST_DECC2     (13)
#define TE_STATE_ST_CLU2      (14)
#define TE_STATE_ST_DECC3     (15)
#define TE_STATE_ST_CLU3      (16)
#define TE_STATE_SS_DECC3     (17)
#define TE_STATE_SS_DECC2     (18)

/* Focus States */
#define FOCUS_STATE_OFF        (0)
#define FOCUS_STATE_STOP       (1)
#define FOCUS_STATE_PLUS       (2)
#define FOCUS_STATE_MINUS      (3)
#define FOCUS_STATE_SLEW       (4)

/* Dome States */
#define DOME_STATE_OFF         (0)
#define DOME_STATE_STOP        (1)
#define DOME_STATE_PLUS        (2)
#define DOME_STATE_MINUS       (3)
#define DOME_STATE_SLEW_PLUS   (4)
#define DOME_STATE_SLEW_MINUS  (5)
#define DOME_STATE_AUTO_STOP   (6)
#define DOME_STATE_AUTO_PLUS   (7)
#define DOME_STATE_AUTO_MINUS  (8)

/* Slit and Flap States */
#define SF_STATE_UNDEF         (0)
#define SF_STATE_OPENING       (1)
#define SF_STATE_CLOSING       (2)
#define SF_STATE_OPEN          (3)
#define SF_STATE_CLOSE         (4)

/* ALARM_BITS */
/* Bank 0 */
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
/* Bank 1 */
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
/* Bank 2 */
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
/* Bank 3 */
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
/* Bank 4 */
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
/* 10 - 15 UNUSED */
#define ALARM_MAX  ALARM_4_BIT_9

/* Often used Alarms */
#define ALARM_BRIDGE      ALARM_2_BIT_1
#define ALARM_HUMIDITY    ALARM_1_BIT_3

/* Bitwise checks */
#define CHECK_BIT(bitmap, bit)      (((bitmap) >> (bit)) & 1)
#define CHECK_ALARM(glst, alarm)    CHECK_BIT(glst.alarm_bits[(int)(alarm/16)], (alarm%16))

/* Data structures */
#define ASCOL_DESCRIBE       (-1)   /* ascol_OIMV() and ascol_GLME() return descriptions only */

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

#define ASCOL_GLST_N_LINUX   (22)
#define ASCOL_GLST_N_MACOS   (16)
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
#define IS_REFRACTION_CORR(glst)    CHECK_BIT(glst.state_bits, 6)
#define IS_ERR_MODEL_CORR(glst)     CHECK_BIT(glst.state_bits, 7)
#define IS_GUIDE_MODE_ON(glst)      CHECK_BIT(glst.state_bits, 8)
#define IS_USER1_BIT_I_ON(glst)     CHECK_BIT(glst.state_bits, 14)
#define IS_USER1_BIT_II_ON(glst)    CHECK_BIT(glst.state_bits, 15)

/* Check alarms and get descripion */
int ascol_check_alarm(ascol_glst_t glst, int alarm, char **descr, int *state);

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
/* Generic functions */
int ascol_0_param_cmd(int devfd, char *cmd_name);
int ascol_1_int_param_cmd(int devfd, char *cmd_name, int param);
int ascol_1_double_param_cmd(int devfd, char *cmd_name, double param, int precision);
int ascol_2_double_param_cmd(int devfd, char *cmd_name, double param1, int precision1, double param2, int precision2);
int ascol_2_double_1_int_param_cmd(int devfd, char *cmd_name, double param1, int precision1, double param2, int precision2, int west);
int ascol_1_double_return_cmd(int devfd, char *cmd_name, double *val);
int ascol_2_double_return_cmd(int devfd, char *cmd_name, double *val1, double *val2);
int ascol_3_ra_de_e_return_cmd(int devfd, char *cmd_name, double *ra, double *de, char *west);


/* Global Commands */

/* GLobal LoGin */
int ascol_GLLG(int devfd, char *password);

/* GLobal MEteorological values */
int ascol_GLME(int devfd, ascol_glme_t *glme);

/* GLobal STate */
int ascol_GLST(int devfd, ascol_glst_t *glst);

/* GLobal read UTc */
int ascol_GLUT(int devfd, double *ut);


/* Telescope Commands */

/* TElescope ON or off */
#define ascol_TEON(devfd, on) (ascol_1_int_param_cmd(devfd, "TEON", on))

/* TElescope TRack */
#define ascol_TETR(devfd, on) (ascol_1_int_param_cmd(devfd, "TETR", on))

/* TElescope Hour axis Calibration */
#define ascol_TEHC(devfd, on) (ascol_1_int_param_cmd(devfd, "TEHC", on))

/* TElescope Declination axis Calibration */
#define ascol_TEDC(devfd, on) (ascol_1_int_param_cmd(devfd, "TEDC", on))

/* Telescope Set new Right ascension and declination Absolute */
#define ascol_TSRA(devfd, ra, de, west) (ascol_2_double_1_int_param_cmd(devfd, "TSRA", ra, 5, de, 5, west))

/* Telescope Read new Right ascension and declination Absolute */
#define ascol_TRRA(devfd, ra, de, west) (ascol_3_ra_de_e_return_cmd(devfd, "TRRA", ra, de, west))

/* Telescope Go to new Right ascension and declination Absolute */
#define ascol_TGRA(devfd, on) (ascol_1_int_param_cmd(devfd, "TGRA", on))

/* Telescope Set new Right ascension and declination Relative */
#define ascol_TSRR(devfd, r_ra, r_de) (ascol_2_double_param_cmd(devfd, "TSRR", r_ra, 2, r_de, 2))

/* Telescope Read new Right ascension and declination Relative */
#define ascol_TRRR(devfd, ra_rel, de_rel) (ascol_2_double_return_cmd(devfd, "TRRR", ra_rel, de_rel))

/* Telescope Go to new Right ascension and declination Relative */
#define ascol_TGRR(devfd, on) (ascol_1_int_param_cmd(devfd, "TGRR", on))

/* Telescope Set new Hour and declination axis Absolute */
#define ascol_TSHA(devfd, ha, de) (ascol_2_double_param_cmd(devfd, "TSHA", ha, 4, de, 4))

/* Telescope Read new Hour and declination axis Absolute */
#define ascol_TRHA(devfd, ha, de) (ascol_2_double_return_cmd(devfd, "TRHA", ra, de))

/* Telescope Go to new Hour and declination axis Absolute */
#define ascol_TGHA(devfd, on) (ascol_1_int_param_cmd(devfd, "TGHA", on))

/* Telescope Set new Hour and declination axis Relative */
#define ascol_TSHR(devfd, r_ha, r_de) (ascol_2_double_param_cmd(devfd, "TSHR", r_ha, 2, r_de, 2))

/* Telescope Read new Hour and declination axis Relative */
#define ascol_TRHR(devfd, ha_rel, de_rel) (ascol_2_double_return_cmd(devfd, "TRHR", ha_rel, de_rel))

/* Telescope Go to new Hour and declination axis Relative */
#define ascol_TGHR(devfd, on) (ascol_1_int_param_cmd(devfd, "TGHR", on))

/* Telescope Set Correction model Set */
#define ascol_TSCS(devfd, model) (ascol_1_int_param_cmd(devfd, "TSCS", model))

/* Telescope Set Correction of the Aberration */
#define ascol_TSCA(devfd, on) (ascol_1_int_param_cmd(devfd, "TSCA", on))

/* Telescope Set Correction of the Precession and nutation */
#define ascol_TSCP(devfd, on) (ascol_1_int_param_cmd(devfd, "TSCP", on))

/* Telescope Set Correction of the Refraction */
#define ascol_TSCR(devfd, on) (ascol_1_int_param_cmd(devfd, "TSCR", on))

/* Telescope Set Correction of the telescope Model */
#define ascol_TSCM(devfd, on) (ascol_1_int_param_cmd(devfd, "TSCM", on))

/* Telescope Set Guiding Mode */
#define ascol_TSGM(devfd, on) (ascol_1_int_param_cmd(devfd, "TSGM", on))

/* Telescope Set Guiding Value */
#define ascol_TSGV(devfd, ra_gv, de_gv) (ascol_2_double_param_cmd(devfd, "TSGV", ra_gv, 1, de_gv, 1))

/* Telescope Read Guiding Value */
#define ascol_TRGV(devfd, ra_gv, de_gv) (ascol_2_double_return_cmd(devfd, "TRGV", ra_gv, de_gv))

/* TElescope declination CEntering */
#define ascol_TECE(devfd, on) (ascol_1_int_param_cmd(devfd, "TECE", on))

/* Telescope Set User Speed */
#define ascol_TSUS(devfd, ra_us, de_us) (ascol_2_double_param_cmd(devfd, "TSUS", ra_us, 4, de_us, 4))

/* Telescope Read User Speed */
#define ascol_TRUS(devfd, ra_us, de_us) (ascol_2_double_return_cmd(devfd, "TRUS", ra_us, de_us))

/* Telescope Set Speed 1 */
#define ascol_TSS1(devfd, speed) (ascol_1_double_param_cmd(devfd, "TSS1", speed, 2))

/* Telescope Read Speed 1 */
#define ascol_TRS1(devfd, speed) (ascol_1_double_return_cmd(devfd, "TRS1", speed))

/* Telescope Set Speed 2 */
#define ascol_TSS2(devfd, speed) (ascol_1_double_param_cmd(devfd, "TSS2", speed, 2))

/* Telescope Read Speed 2 */
#define ascol_TRS2(devfd, speed) (ascol_1_double_return_cmd(devfd, "TRS2", speed))

/* Telescope Set Speed 3 */
#define ascol_TSS3(devfd, speed) (ascol_1_double_param_cmd(devfd, "TSS3", speed, 2))

/* Telescope Read Speed 3 */
#define ascol_TRS3(devfd, speed) (ascol_1_double_return_cmd(devfd, "TRS3", speed))

/* Telescope Read Right ascension and Declination */
#define ascol_TRRD(devfd, ra, de, west) (ascol_3_ra_de_e_return_cmd(devfd, "TRRD", ra, de, west))

/* Telescope Read Hour and Declination Axis */
#define ascol_TRHD(devfd, ha, de) (ascol_2_double_return_cmd(devfd, "TRHD", ha, de))

/* Telescope Set Guiding Corrections */
#define ascol_TSGC(devfd, ra_gc, de_gc) (ascol_2_double_param_cmd(devfd, "TSGC", ra_gc, 1, de_gc, 1))


/* Focuser Commands */

/* FOcus STop */
#define ascol_FOST(devfd) (ascol_0_param_cmd(devfd, "FOST"))

/* FOcus Go Relative */
#define ascol_FOGR(devfd) (ascol_0_param_cmd(devfd, "FOGR"))

/* FOcus Go Absolute */
#define ascol_FOGA(devfd) (ascol_0_param_cmd(devfd, "FOGA"))

/* FOcus Set Relative position */
#define ascol_FOSR(devfd, pos) (ascol_1_double_param_cmd(devfd, "FOSR", pos, 2))

/* FOcus Set Absolute position */
#define ascol_FOSA(devfd, pos) (ascol_1_double_param_cmd(devfd, "FOSA", pos, 2))

/* FOcus POsition */
#define ascol_FOPO(devfd, pos) (ascol_1_double_return_cmd(devfd, "FOPO", pos))


/* Dome Commands */

/* DOme ON or off */
#define ascol_DOON(devfd, on) (ascol_1_int_param_cmd(devfd, "DOON", on))

/* DOme Slit Open */
#define ascol_DOSO(devfd, on) (ascol_1_int_param_cmd(devfd, "DOSO", on))

/* DOme POsition */
#define ascol_DOPO(devfd, pos) (ascol_1_double_return_cmd(devfd, "DOPO", pos))

/* DOme STop */
#define ascol_DOST(devfd) (ascol_0_param_cmd(devfd, "DOST"))

/* DOme Go Relative */
#define ascol_DOGR(devfd) (ascol_0_param_cmd(devfd, "DOGR"))

/* DOme Go Absolute */
#define ascol_DOGA(devfd) (ascol_0_param_cmd(devfd, "DOGA"))

/* DOme AutoMaded mode */
#define ascol_DOAM(devfd) (ascol_0_param_cmd(devfd, "DOAM"))

/* DOme Set Relative position */
#define ascol_DOSR(devfd, pos) (ascol_1_double_param_cmd(devfd, "DOSR", pos, 2))

/* DOme Set Absolute position */
#define ascol_DOSA(devfd, pos) (ascol_1_double_param_cmd(devfd, "DOSA", pos, 2))


/* Flap commands */

/* Flap Tube Open or Close */
#define ascol_FTOC(devfd, on) (ascol_1_int_param_cmd(devfd, "FTOC", on))

/* Flaap Coude Open or Close */
#define ascol_FCOC(devfd, on) (ascol_1_int_param_cmd(devfd, "FCOC", on))


/* Oil Commands */

/* OIl ON or off */
#define ascol_OION(devfd, on) (ascol_1_int_param_cmd(devfd, "OION", on))

/* OIl Measurement Values */
int ascol_OIMV(int devfd, ascol_oimv_t *oimv);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

extern int ascol_debug;

#endif /* __LIBASCOL_H */
