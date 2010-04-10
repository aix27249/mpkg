#include "composite_setup.h"
#include <stdio.h>
#include <cstdio>
#include <unistd.h>
int print_usage(char *argv) {
	fprintf(stderr, "composite_setup: enable/disable compositing and DRI support in X11\nLicensed under GPL, (c) RPU NET, http://www.mopslinux.org\nUsage: %s enable\tenable compositing\n\t%s disable\tdisable compositing\n", argv, argv);
	return 1;
}
int main(int argc, char **argv) {
	if (argc != 1) return print_usage(argv[0]);
	if (string(argv[1])=="enable") enableComposite("/root/xorg.conf");
	if (string(argv[1])=="disable") enableComposite("/root/xorg.conf", true);
	return 0;
}
