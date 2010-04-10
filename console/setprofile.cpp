#include <mpkg/libmpkg.h>
int print_usage() {
	fprintf(stderr, _("MPKG Package System: set repository profile\n"));
	fprintf(stderr, _("Usage: mpkg-setprofile PROFILE_NAME\n"));
	return 1;
}
int main(int argc, char **argv) {
	if (argc<2) return print_usage();
	if (!FileExists("/etc/mpkg/profiles/" + string(argv[1]))) {
		fprintf(stderr, _("Selected profile %s doesn't exist\n"), argv[1]);
		return 1;
	}
	mpkg *core = new mpkg;
	vector<string> repListTmp = ReadFileStrings("/etc/mpkg/profiles/" + string(argv[1]));
	vector<string> repList;
	// Validating input
	for (unsigned int i=0; i<repListTmp.size(); ++i) {
		if (validateRepStr(repListTmp[i])) {
			repList.push_back(repListTmp[i]);
			printf("%s\n", repListTmp[i].c_str());
		}
	}
	repListTmp.clear();
	core->set_repositorylist(repList, repListTmp);
	delete core;
	return 0;
}
