#include "lvmtool.h"
#include <mpkgsupport/mpkgsupport.h>
// Initializes LVM volumes
void initLVM() {
	system("vgscan --mknodes --ignorelockingfailure >/dev/null");
	system("vgchange -ay --ignorelockingfailure >/dev/null");
}


LVM_VG::LVM_VG() {
}

LVM_VG::~LVM_VG() {
}

vector<pEntry> LVM_VG::getLVList() {
	string tmpfile = get_tmp_file();
	system("lvdisplay -C --units=m " + name + " > " + tmpfile);
	vector<string> data = ReadFileStrings(tmpfile);
	unlink(tmpfile.c_str());
	// Let's parse it
	size_t pos;
	string tmpstring;
	vector<pEntry> ret;
	pEntry new_pEntry;
	for (size_t i=1; i<data.size(); ++i) {
		tmpstring = cutSpaces(data[i]);
		
		// LV name
		pos = tmpstring.find_first_of(" \t");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output
		new_pEntry.devname = "/dev/" + name + "/" + tmpstring.substr(0, pos);

		// VG name (skipping)
		tmpstring = cutSpaces(tmpstring.substr(pos));
		pos = tmpstring.find_first_of(" \t");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output
		
		// Attributes (skipping)
		tmpstring = cutSpaces(tmpstring.substr(pos));
		pos = tmpstring.find_first_of(" \t");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output

		// LV size
		tmpstring = cutSpaces(tmpstring.substr(pos));
		pos = tmpstring.find_first_of(".m");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output
		new_pEntry.size = tmpstring.substr(0, pos);

		ret.push_back(new_pEntry);
	}
	return ret;
	
}

vector<LVM_VG> getLVM_VGList() {
	string tmpfile = get_tmp_file();
	system("vgdisplay -C --units=m > " + tmpfile);
	vector<string> data = ReadFileStrings(tmpfile);
	unlink(tmpfile.c_str());
	// Let's parse it
	size_t pos;
	string tmpstring;
	vector<LVM_VG> ret;
	LVM_VG newVG;
	for (size_t i=1; i<data.size(); ++i) {
		tmpstring = cutSpaces(data[i]);
		
		// VG name
		pos = tmpstring.find_first_of(" \t");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output
		newVG.name = tmpstring.substr(0, pos);

		
		// #PV (skip)
		tmpstring = cutSpaces(tmpstring.substr(pos));
		pos = tmpstring.find_first_of(" \t");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output
		
		// #LV (skip)
		tmpstring = cutSpaces(tmpstring.substr(pos));
		pos = tmpstring.find_first_of(" \t");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output

		// #SN (skip)
		tmpstring = cutSpaces(tmpstring.substr(pos));
		pos = tmpstring.find_first_of(" \t");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output

		// Attributes (skip)
		tmpstring = cutSpaces(tmpstring.substr(pos));
		pos = tmpstring.find_first_of(" \t");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output

		// VG size
		tmpstring = cutSpaces(tmpstring.substr(pos));
		pos = tmpstring.find_first_of(".m");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output
		newVG.size = tmpstring.substr(0, pos);
		
		pos = tmpstring.find_first_of(" \t");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output

		// VG Free
		tmpstring = cutSpaces(tmpstring.substr(pos));
		pos = tmpstring.find_first_of(".m");
		if (pos==std::string::npos) continue; // in case of corrupted or suddenly changed lvdisplay output
		newVG.freesize = tmpstring.substr(0, pos);
	
		ret.push_back(newVG);
	}
	return ret;


}
