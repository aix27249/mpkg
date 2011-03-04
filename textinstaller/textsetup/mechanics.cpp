#include "mechanics.h"
#include "mediachecker.h"
TextSetupMechanics::TextSetupMechanics() {
}

TextSetupMechanics::~TextSetupMechanics() {
}

vector<CustomPkgSet> TextSetupMechanics::getCustomPkgSetList(const string &pkgsource, string *result_pkgsource, string *volname, string *rep_location) {
	MediaChecker mediaChecker;

	ncInterface.uninit(); // Disable ncurses, since we do expect lots of debugging output here
	return mediaChecker.getCustomPkgSetList(pkgsource, setlocale(LC_ALL, NULL), result_pkgsource, volname, rep_location);
}


bool TextSetupMechanics::checkNvidiaLoad() {
	int hasNvidia = -1;

	if (hasNvidia==-1) {
		string tmp_hw = get_tmp_file();
		system("lspci > " + tmp_hw);
		vector<string> hw = ReadFileStrings(tmp_hw);
		for(size_t i=0; i<hw.size(); ++i) {
			if (hw[i].find("VGA compatible controller")!=std::string::npos && hw[i].find("nVidia")!=std::string::npos) hasNvidia = i;
		}
		if (hasNvidia == -1) hasNvidia = 0;
	}
	// For debugging reasons, check for /tmp/nvidia_force file
	if (FileExists("/tmp/nvidia_force")) hasNvidia = 1;

	return hasNvidia > 0;
}

