#include <mpkg/libmpkg.h>

int print_usage(int ret = 0) {
	fprintf(stderr, "mpkg-setopt: set specific option from mpkg config file\n");
	fprintf(stderr, "Usage:\nmpkg-setopt OPTION_NAME OPTION_VALUE\n");
	return ret;
}

int main(int argc, char **argv) {
	if (argc!=3) return print_usage(1);
	// Requires root, so check it and use sudo if required
	if (getuid() != 0 ) {
		string arg_string;
		for (int i=0; i<argc; i++) {
			arg_string += (string) argv[i] + " ";
		}
		return system("sudo " + arg_string);
	}

	mConfig.setValue(argv[1], argv[2]);
	return 0;
}
