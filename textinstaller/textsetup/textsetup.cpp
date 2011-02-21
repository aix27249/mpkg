#include "textsetup.h"

TextSetup::TextSetup() {
}

TextSetup::~TextSetup() {
}

int TextSetup::run() {
	int ret;
	for (int i=0; i<9; ++i) {
		switch(i) {
			case 0: ret = setPackageSource(); break;
			case 1: ret = setInstallType(); break;
			case 2: ret = setPartitionEditor(); break;
			case 3: ret = setMountPoints(); break;
			case 4: ret = setBootLoader(); break;
			case 5: ret = setRootPassword(); break;
			case 6: ret = setCreateUser(); break;
			case 7: ret = setNetworkSettings(); break;
			case 8: ret = setTimezone(); break;
			default: return -1;
		}
		if (ret!=0) i=i-2;
		if (i<0) return -1;
	}
	return 0;
}

int TextSetup::setPackageSource() {
	return 0;
}

int TextSetup::setInstallType() {
	setNvidiaDriver();
	return 0;
}

int TextSetup::setNvidiaDriver() {
	return 0;
}

int TextSetup::setPartitionEditor() {
	return 0;
}

int TextSetup::setMountPoints() {
	return 0;
}

int TextSetup::setBootLoader() {
	return 0;
}

int TextSetup::setRootPassword() {
	return 0;
}

int TextSetup::setCreateUser() {
	return 0;
}

int TextSetup::setNetworkSettings() {
	return 0;
}

int TextSetup::setTimezone() {
	return 0;
}

