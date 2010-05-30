#ifndef MPKG_LVMTOOL_H_
#define MPKG_LVMTOOL_H_
#include <vector>
#include <string>
#include "mpkg-parted.h"
using namespace std;

class LVM_VG {
	public:
		LVM_VG();
		~LVM_VG();

		string name;
		string size;
		string freesize;
		vector<pEntry> getLVList();
};

void initLVM();
vector<LVM_VG> getLVM_VGList();
#endif
