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

	double dd;
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
