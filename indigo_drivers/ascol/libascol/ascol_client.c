#include<stdio.h>
#include<libascol.h>

int main() {
	int fd = open_telescope("localhost",2000);
	printf("OPEN: %d\n", fd);

	char cmd[80] = "TRRD\n";
	char response[80];
	int res = write_telescope(fd, cmd);
	printf("WROTE: %d -> %s", res, cmd);

	char res_str[80] = {0};
	res = read_telescope(fd, res_str, 80);
	printf("READ: %d <- >%s<\n", res, res_str);

	double ra, ha, de;
	char east;

	printf("\n===== ascol_TRRD() =====\n");
	res = ascol_TRRD(fd, &ra, &de, &east);
	printf("TRRD = %d <- %lf %lf %d\n", res, ra, de, east);

	printf("\n===== ascol_GLLG() =====\n");
	res = ascol_GLLG(fd, "234");
	printf("GLLG = %d\n", res);
	res = ascol_GLLG(fd, "123");
	printf("GLLG = %d\n", res);
	res = ascol_GLLG(fd, "");
	printf("GLLG = %d\n", res);


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

	printf("\n===== ascol_TSS1() =====\n");
	res = ascol_TSS1(fd, 150.2134);
	printf("TSS1 = %d\n", res);
	res = ascol_TSS1(fd, 15.2134);
	printf("TSS1 = %d\n", res);

	printf("\n===== ascol_DOSA() =====\n");
	res = ascol_DOSA(fd, 150.2134);
	printf("DOSA = %d\n", res);
	res = ascol_DOSA(fd, 15.2134);
	printf("DOSA = %d\n", res);
	res = ascol_DOSA(fd, -3515.2154);
	printf("DOSA = %d\n", res);

	double dd;
	printf("\n===== dms2dd() =====\n");
	strcpy(response, "+1030003.6");
	res = dms2dd(&dd, response);
	printf("dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+103003.6");
	res = dms2dd(&dd, response);
	printf("dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+1003");
	res = dms2dd(&dd, response);
	printf("dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+103");
	res = dms2dd(&dd, response);
	printf("dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "103003");
	res = dms2dd(&dd, response);
	printf("dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "103003.6");
	res = dms2dd(&dd, response);
	printf("dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "-103036.");
	res = dms2dd(&dd, response);
	printf("dms2dd() = %2d: %s -> %.9lf\n", res, response, dd);

	printf("\n===== hms2dd() =====\n");
	strcpy(response, "+1030003.6");
	res = hms2dd(&dd, response);
	printf("hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+103003.6");
	res = hms2dd(&dd, response);
	printf("hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+1003");
	res = hms2dd(&dd, response);
	printf("hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "+103");
	res = hms2dd(&dd, response);
	printf("hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "103003");
	res = hms2dd(&dd, response);
	printf("hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "103003.6");
	res = hms2dd(&dd, response);
	printf("hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "240000.01");
	res = hms2dd(&dd, response);
	printf("hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);
	strcpy(response, "235959.99999");
	res = hms2dd(&dd, response);
	printf("hms2dd() = %2d: %s -> %.9lf\n", res, response, dd);


}
