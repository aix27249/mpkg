#ifndef MECHANICS_H__
#define MECHANICS_H__
#include <mpkg/libmpkg.h>
#include "custompkgset.h"

class TextSetupMechanics {
	public:
		TextSetupMechanics();
		~TextSetupMechanics();

		vector<CustomPkgSet> getCustomPkgSetList(const string &pkgsource, string *result_pkgsource, string *volname, string *rep_location);

		bool checkNvidiaLoad();

};


#endif
