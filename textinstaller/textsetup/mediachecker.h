#ifndef MEDIACHECKER_H__
#define MEDIACHECKER_H__

#include <mpkg/libmpkg.h>
#include "custompkgset.h"

class MediaChecker {
	public:
		MediaChecker();
		~MediaChecker();

		vector<CustomPkgSet> getCustomPkgSetList(const string &_pkgsource, char *locale, string *result_pkgsource, string *r_volname, string *r_rep_location);

	private:
		// Media detection and pkglist retriveal
		string detectDVDDevice(string isofile="");
		string dvdDevice;
		string volname, rep_location;
		string pkgsource;
		bool mountDVD(string device="", string mountOptions="", bool iso=false);
		bool umountDVD();
		vector<CustomPkgSet> customPkgSetList;

		void calculatePkgSetSize(CustomPkgSet &set);
		void getCustomSetupVariants(const vector<string>& rep_list);

		CustomPkgSet getCustomPkgSet(const string& name);
		string locale;
};


#endif
