#ifndef TEXTSETUP_H__
#define TEXTSETUP_H__

#include <iniparser.h>
#include <mpkg/libmpkg.h>

class TextSetup {
	public: 
		TextSetup();
		~TextSetup();

		mpkg *core;
		int setPackageSource();
		int setInstallType();
		int setNvidiaDriver();
		int setPartitionEditor();
		int setMountPoints();
		int setBootLoader();
		int setRootPassword();
		int setCreateUser();
		int setNetworkSettings();
		int setTimezone();

		int run();

};

#endif
