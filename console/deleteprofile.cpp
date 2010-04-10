#include <mpkg/libmpkg.h>
int print_usage() {
	fprintf(stderr, _("MPKG Package System: delete repository profile\n"));
	fprintf(stderr, _("Usage: mpkg-deleteprofile PROFILE_NAME\n"));
	return 1;
}

int main(int argc, char **argv) {
	if (argc<2) return print_usage();
	if (!FileExists("/etc/mpkg/profiles/" + string(argv[1]))) {
		fprintf(stderr, _("Profile %s doesn't exist\n"), argv[1]);
		return 2;
	}
	if (unlink(string("/etc/mpkg/profiles/" + string(argv[1])).c_str())) {
		mError(_("Failed to delete profile ") + string(argv[1]) + _(", may be you are not root?"));
		return 1;
	}
	return 0;
}
