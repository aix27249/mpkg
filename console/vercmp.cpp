#include <mpkgsupport/mpkgsupport.h>

int print_usage(const char *prog_name, int ret_value) {
	FILE *fd = stdout;
	if (ret_value!=0) fd = stderr;
	fprintf(fd, "%s: compares versions using mpkg logic\n", prog_name);
	fprintf(fd, "Usage: %s <version1> <version2>\n", prog_name);
	return ret_value;

}

int main(int argc, char **argv) {
	if (argc==2 && (strcmp(argv[1], "--help")==0 || strcmp(argv[1], "-h")==0)) return print_usage(argv[0], 0);
	if (argc!=3) {
		return print_usage(argv[0], -1);
	}
	int result = strverscmp2(argv[1], argv[2]);
	printf("%d\n", result);
	return 0;
}
