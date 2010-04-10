#include <mpkg/libmpkg.h>

int main(int argc, char **argv) {

	PACKAGE_LIST i_availablePackages;
	mpkg core;
	SQLRecord sqlSearch;
	string arg;
	bool shift=false;
	if (argc>1 && string(argv[1])=="-i") {
		shift=true;
	}
	if (argc>1) {
		if (shift) {
			if (argc>2) arg = string(argv[2]);
		}
		else {
			arg = string(argv[1]);
		}
		if (!arg.empty()) {
			sqlSearch.addField("package_name", arg+"%");
			sqlSearch.setEqMode(EQ_CUSTOMLIKE);
		}
	}
	core.get_packagelist(sqlSearch, &i_availablePackages, true);
	for (size_t i=0; i<i_availablePackages.size(); ++i) {
		if (!shift || i_availablePackages[i].installed()) cout << i_availablePackages[i].get_name() << endl;
	}
	return 0;
}

