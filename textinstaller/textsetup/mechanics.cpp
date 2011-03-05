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

void TextSetupMechanics::updatePartitionLists() {
	ncInterface.showInfoBox(_("Updating lists of available drives and partitions, please wait"));
	// Let's go setuid. At least, let's try.
	uid_t uid = getuid();
	if (uid) {
		perror("Failed to obtain root privileges to read disk partition table");
		ncInterface.showMsgBox(_("This program should have suid bit, or be run from root. Otherwise, it could not get drive information.\nThis is fatal, and we have to exit."));
		exit(1);
		return;
	}
	drives = getDevList();
	partitions = getPartitionList();
	lvm_groups = getLVM_VGList();

	// Since fslabel in libparted means completely other thing, let's get this from blkid
	for (size_t i=0; i<partitions.size(); ++i) {
		partitions[i].fslabel = getLABEL(partitions[i].devname).c_str();
	}
}

string TextSetupMechanics::getLABEL(const string& dev) {
	string tmp_file = get_tmp_file();
	string data;
	int try_count = 0;
	while (try_count<2 && data.empty()) {
		system("blkid -s LABEL " + dev + " > " + tmp_file);
		data = ReadFile(tmp_file);
		try_count++;
	}
	if (data.empty()) return "";
	size_t a = data.find_first_of("\"");
	if (a==std::string::npos || a==data.size()-1) return "";
	data.erase(data.begin(), data.begin()+a+1);
	a = data.find_first_of("\"");
	if (a==std::string::npos || a==0) return "";
	return data.substr(0, a);
}
