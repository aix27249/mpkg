#include <mpkg/libmpkg.h>

#define MODE_HELP 0
#define MODE_SHOW 1
#define MODE_SET 2

int main(int argc, char **argv) {
	int mode = MODE_HELP;
	if (argc==2 && string(argv[1])=="--show") mode = MODE_SHOW;
	if (argc>3 && string(argv[1])=="--set") mode = MODE_SET;

	string mName = mConfig.getValue("maintainer_name");
	string mEmail = mConfig.getValue("maintainer_email");

	if (getuid() && FileExists(getenv("HOME") + string("/.mpkg-maintainer"))) {
		vector<string> mdata = ReadFileStrings(getenv("HOME") + string("/.mpkg-maintainer"));
		if (mdata.size()>0) mName=cutSpaces(mdata[0]);
		if (mdata.size()>1) mEmail=cutSpaces(mdata[1]);
	}

	switch (mode) {
		case MODE_HELP:
			fprintf(stderr, _("%s: shows or sets maintainer name and email\nSyntax:\n"), argv[0]);
			fprintf(stderr, _("%s --help              show this help\n"), argv[0]);
			fprintf(stderr, _("%s --show              show maintainer name and email\n"), argv[0]);
			fprintf(stderr, _("%s --set NAME EMAIL    set name and email appropriately\n"), argv[0]);
			return 1;
		case MODE_SET:
			mName = argv[2];
			mEmail = argv[3];
			if (getuid()) {
				printf("Writing user maintainer\n");
				system("echo " + mName + " > ~/.mpkg-maintainer");
				system("echo " + mEmail + " >> ~/.mpkg-maintainer");
			}
			else {
				mConfig.setValue("maintainer_name", mName);
				mConfig.setValue("maintainer_email", mEmail);
			}
			// Doesn't break here, let's show data

		case MODE_SHOW:
			printf("MaintainerName: %s\nMaintainerEmail: %s\n", mName.c_str(), mEmail.c_str());
			return 0;

	}
	return 0;

}
