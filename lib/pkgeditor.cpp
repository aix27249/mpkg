#include "libmpkg.h"
#include "metaframe.h"
MetaPackage *pkg;
void showMainMenu() {
}

int main(int argc, char **argv) {
	dialogMode = true;
	if (argc!=2) {
		fprintf(stderr, _("MPKG Package Editor (ncurses-based)\nUsage: %s FILENAME\n"), argv[0]);
		return 1;
	}
	pkg = new MetaPackage(argv[1]);
	if (!pkg->data) {
		mError("Failed to load package " + string(argv[1]));
		return 2;
	}
	showMainMenu();
	return 0;
}
