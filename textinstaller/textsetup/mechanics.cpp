#include "mechanics.h"
#include "mediachecker.h"
TextSetupMechanics::TextSetupMechanics() {
}

TextSetupMechanics::~TextSetupMechanics() {
}

vector<CustomPkgSet> TextSetupMechanics::getCustomPkgSetList(const string &pkgsource) {
	MediaChecker mediaChecker;

	ncInterface.uninit(); // Disable ncurses, since we do expect lots of debugging output here
	return mediaChecker.getCustomPkgSetList(pkgsource, setlocale(LC_ALL, NULL));
}
