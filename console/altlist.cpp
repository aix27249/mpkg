#include <mpkg/libmpkg.h>

int main(int, char **) {
	mpkg core;
	SQLRecord sqlSearch;
	PACKAGE_LIST pkgList;
	core.get_packagelist(sqlSearch, &pkgList);
	vector<string> alt = pkgList.getAlternativeList(true);
	for (size_t i=0; i<alt.size(); ++i) {
		cout << alt[i] << endl;
	}
	return 0;
}
