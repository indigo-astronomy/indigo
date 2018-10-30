#include<stdio.h>
#include<libascol.h>

int main() {
	int fd = open_telescope("localhost",2000);
	printf("OPEN: %d\n", fd);

	char cmd[80] = "TRRD\n";
	int res = write_telescope(fd, cmd);
	printf("WROTE: %d -> %s", res, cmd);

	char res_str[80] = {0};
	res = read_telescope(fd, res_str, 80);

	printf("READ: %d <- >%s<\n", res, res_str);
}
