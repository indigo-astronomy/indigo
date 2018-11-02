#include<stdio.h>
#include<libascol.h>

int main() {
	ascol_debug = 1;
	int fd = ascol_open("localhost",2000);
	printf("OPEN: %d\n", fd);

	char cmd[80] = "TRRD\n";
	char response[80];
	int res = ascol_write(fd, cmd);
	printf("WROTE: %d -> %s", res, cmd);

	char res_str[80] = {0};
	res = ascol_read(fd, res_str, 80);
	printf("READ: %d <- >%s<\n", res, res_str);

	double ra, ha, de;
	char east;



	printf("\n===== ascol_GLLG() =====\n");
	res = ascol_GLLG(fd, "234");
	printf("GLLG = %d\n", res);
	res = ascol_GLLG(fd, "123");
	printf("GLLG = %d\n", res);
	res = ascol_GLLG(fd, "");
	printf("GLLG = %d\n", res);

	ascol_TETR(fd, ASCOL_ON);

	ascol_oimv_t oimv;
	printf("\n===== ascol_OIMV() =====\n");
	res = ascol_OIMV(fd, &oimv);
	printf("OIMV = %2d\n", res);
	for (int i=0; i < ASCOL_OIMV_N; i++) {
		printf("OIMV[%d] = %lf %s (%s)\n", i, oimv.value[i], oimv.unit[i], oimv.description[i]);
	}

	ascol_glst_t glst;
	printf("\n===== ascol_glst() =====\n");
	res = ascol_GLST(fd, &glst);
	printf("GLST = %2d\n", res);
	printf( "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
		(glst.oil_state), (glst.telescope_state), (glst.ra_axis_state), (glst.de_axis_state), (glst.focus_state),
		(glst.dome_state), (glst.slit_state), (glst.flap_tube_state), (glst.flap_coude_state), (glst.selected_model_index),
		(glst.state_bits), (glst.alarm_bits[0]), (glst.alarm_bits[1]), (glst.alarm_bits[2]), (glst.alarm_bits[3]),
		(glst.alarm_bits[4])
	);

	printf("HA callibrated: %d\n", IS_RA_CALIBRATED(glst));
	printf("DA callibrated: %d\n", IS_DA_CALIBRATED(glst));
	printf("Aberration Cor: %d\n", IS_ABEARRATION_CORR(glst));

	char *oil_l, *oil_s;
	ascol_get_oil_state(glst, &oil_l, &oil_s);
	printf("oil state: %s - %s\n", oil_s, oil_l);

	char *te_l, *te_s;
	ascol_get_telescope_state(glst, &te_l, &te_s);
	printf("telescope state: %s - %s\n", te_s, te_l);

	ascol_glme_t glme;
	printf("\n===== ascol_GLME() =====\n");
	res = ascol_GLME(fd, &glme);
	printf("GLME = %2d\n", res);
	for (int i=0; i < ASCOL_GLME_N; i++) {
		printf("GLME[%d] = %lf %s (%s)\n", i, glme.value[i], glme.unit[i], glme.description[i]);
	}

	printf("\n===== ascol_TSRA() =====\n");
	res = ascol_TSRA(fd, 15.5, -10.111111, 1);
	printf("TSRA = %d\n", res);

	ascol_TGRA(fd, ASCOL_ON);
	sleep(30);

	printf("\n===== ascol_TRRD() =====\n");
	res = ascol_TRRD(fd, &ra, &de, &east);
	printf("TRRD = %d <- %lf %lf %d\n", res, ra, de, east);

	printf("\n===== ascol_TRHD() =====\n");
	res = ascol_TRHD(fd, &ha, &de);
	printf("TRHD = %d <- %lf %lf\n", res, ha, de);

	printf("\n===== ascol_TEON() =====\n");
	res = ascol_TEON(fd, ASCOL_ON);
	printf("TEON = %d\n", res);
	res = ascol_TEON(fd, ASCOL_OFF);
	printf("TEON = %d\n", res);
	res = ascol_TEON(fd, 3);
	printf("TEON = %d\n", res);
	res = ascol_TEON(fd, ASCOL_ON);
	printf("TEON = %d\n", res);

	printf("\n===== ascol_TSCS() =====\n");
	res = ascol_TETR(fd, ASCOL_OFF);
	printf("TETR = %d\n", res);

	res = ascol_TSCS(fd, 3);
	printf("TSCS = %d\n", res);

	res = ascol_TETR(fd, ASCOL_ON);
	printf("TETR = %d\n", res);

	printf("\n===== ascol_TSS1() =====\n");
	res = ascol_TSS1(fd, 150.2134);
	printf("TSS1 = %d\n", res);
	res = ascol_TSS1(fd, 15.2134);
	printf("TSS1 = %d\n", res);

	printf("\n===== ascol_DOON() =====\n");
	res = ascol_DOON(fd, ASCOL_ON);
	printf("DOON = %d\n", res);

	printf("\n===== ascol_DOSA() =====\n");
	res = ascol_DOSA(fd, 150.2134);
	printf("DOSA = %d\n", res);
	res = ascol_DOSA(fd, 15.2134);
	printf("DOSA = %d\n", res);
	res = ascol_DOSA(fd, -3515.2154);
	printf("DOSA = %d\n", res);

	printf("\n===== ascol_DOGA() =====\n");
	res = ascol_DOGA(fd);
	printf("DOGA = %d\n", res);

	//printf("\n===== ascol_DOST() =====\n");
	//res = ascol_DOST(fd);
	//printf("DOST = %d\n", res);
	//sleep(2);

	double pos;
	printf("\n===== ascol_DOPO() =====\n");
	res = ascol_DOPO(fd, &pos);
	printf("DOST = %d <- %lf\n", res, pos);

	double ra_us, de_us;
	printf("\n===== ascol_TSUS() =====\n");
	res = ascol_TSUS(fd, 1.22222, 2.3333);
	printf("TSUS = %d\n", res);

	printf("\n===== ascol_TRUS() =====\n");
	res = ascol_TRUS(fd, &ra_us, &de_us);
	printf("TRUS = %d <- %lf %lf\n", res, ra_us, de_us);

	double ra_gv, de_gv;
	printf("\n===== ascol_TSGV() =====\n");
	res = ascol_TSGV(fd, 1.22222, 2.3333);
	printf("TSGV = %d\n", res);

	printf("\n===== ascol_TRGV() =====\n");
	res = ascol_TRGV(fd, &ra_gv, &de_gv);
	printf("TRGV = %d <- %lf %lf\n", res, ra_gv, de_gv);

	printf("\n===== ascol_TECE() =====\n");
	res = ascol_TECE(fd, ASCOL_ON);
	printf("TECE = %d\n", res);
	res = ascol_TECE(fd, ASCOL_OFF);
	printf("TECE = %d\n", res);
	res = ascol_TECE(fd, 2);
	printf("TECE = %d\n", res);

	double dd;
	printf("\n===== ascol_dms2dd() =====\n");
	strcpy(response, "+1030003.6");
	res = ascol_dms2dd(&dd, response);
	printf("ascol_dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+103003.6");
	res = ascol_dms2dd(&dd, response);
	printf("ascol_dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+1003");
	res = ascol_dms2dd(&dd, response);
	printf("ascol_dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+103");
	res = ascol_dms2dd(&dd, response);
	printf("ascol_dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "103003");
	res = ascol_dms2dd(&dd, response);
	printf("ascol_dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "103003.6");
	res = ascol_dms2dd(&dd, response);
	printf("ascol_dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "-103036.");
	res = ascol_dms2dd(&dd, response);
	printf("ascol_dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);

	printf("\n===== ascol_hms2dd() =====\n");
	strcpy(response, "+1030003.6");
	res = ascol_hms2dd(&dd, response);
	printf("ascol_hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+103003.6");
	res = ascol_hms2dd(&dd, response);
	printf("ascol_hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+1003");
	res = ascol_hms2dd(&dd, response);
	printf("ascol_hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+103");
	res = ascol_hms2dd(&dd, response);
	printf("ascol_hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "103003");
	res = ascol_hms2dd(&dd, response);
	printf("ascol_hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "103003.6");
	res = ascol_hms2dd(&dd, response);
	printf("ascol_hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "240000.01");
	res = ascol_hms2dd(&dd, response);
	printf("ascol_hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "235959.99999");
	res = ascol_hms2dd(&dd, response);
	printf("ascol_hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);


}
