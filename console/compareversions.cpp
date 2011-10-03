#include <mpkgsupport/string_operations.h>

int main(int argc, char **argv) {
	if (argc!=5) {
		fprintf(stderr, "Compares versions and builds of two packages.\nUsage: %s VER_1 BUILD_1 VER_2 BUILD_2\nResults:\n\t-1: V1 < V2\n\t0: V1 = V2\n\t1: V1 > V2\n", argv[0]);
		return 2;
	}
	int result = compareVersions(argv[1], argv[2], argv[3], argv[4]);
	printf("%d\n", result);
	return 0;
}
