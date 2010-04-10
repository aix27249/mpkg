#include "composite_setup.h"
#include <mpkg/libmpkg.h>
void enableComposite(string filename, bool disable) {
	int card = CARD_GENERIC;
	string a;
	if (disable) a = "#";
	vector<string> conf = ReadFileStrings(filename);
	int sh=0;
	for (unsigned int i=0; i<conf.size(); ++i) {
		if (conf[i].find("Section \"Extensions\"")!=std::string::npos || i==conf.size()-1) {
			sh=0;
			if (i!=conf.size()-1) {
				while (conf[i+sh].find("EndSection")==std::string::npos) sh++;
				conf.erase(conf.begin()+i, conf.begin()+i+sh+1);
			}
			VI(i, a, "EndSection");
			VI(i, a, "Option \"Composite\" \"Enable\"");
			VI(i, a, "Option \"RENDER\" \"Enable\"");
			VI(i, a, "Section \"Extensions\"");
			break;
		}
	}
	sh = 0;
	for (unsigned int i=0; i<conf.size(); ++i) {
		if (conf[i].find("Section \"DRI\"")!=std::string::npos || i==conf.size()-1) {
			sh=0;
			if (i!=conf.size()-1) {
				while (conf[i+sh].find("EndSection")==std::string::npos) sh++;
				conf.erase(conf.begin()+i, conf.begin()+i+sh+1);
			}
			VI(i, a, "EndSection");
			VI(i, a, "Mode 0666");
			VI(i, a, "Group 0");
			VI(i, a, "Section \"DRI\"");
			break;
		}
	}


	for (unsigned int i=0; i<conf.size(); ++i) {
		// Detect driver and identifier
		string driver, identifier;
		if (conf[i].find("Section \"Device\"")!=std::string::npos || i==conf.size()-1) {
			for (unsigned int k=i; k<conf.size() && conf[k].find("EndSection") == std::string::npos; ++k) {
				if (conf[k].find("Driver")!=std::string::npos) {
					driver = cutSpaces(conf[k].substr(conf[k].find("Driver") + strlen("Driver")));
					if (driver.find("\"")==0 && driver.rfind("\"")==driver.size()-1) {
						driver = driver.substr(1, driver.size()-2);
					}
				}
				if (conf[k].find("Identifier")!=std::string::npos) {
					identifier = cutSpaces(conf[k].substr(conf[k].find("Identifier") + strlen("Identifier")));
					if (identifier.find("\"")==0 && identifier.rfind("\"")==identifier.size()-1) identifier = identifier.substr(1, identifier.size()-2);
				}
			}
			card = CARD_GENERIC;
			if (driver == "intel") card=CARD_INTEL;
			if (driver == "fglrx") card=CARD_ATI;
			if (driver == "nvidia") card=CARD_NVIDIA;
		
			sh=0;
			if (i!=conf.size()-1) {
				while (conf[i+sh].find("EndSection")==std::string::npos) sh++;
				conf.erase(conf.begin()+i, conf.begin()+i+sh+1);
			}
			VI(i, a, "EndSection");
			switch(card) {
				case CARD_INTEL:
					VI(i, a, "Option \"DRI\" \"True\"");
					VI(i, a, "Option \"AccelMethod\" \"xaa\"");
					VI(i, a, "Option \"NoAccel\" \"False\"");
					VI(i, a, "Option \"MigrationHeuristic\" \"greedy\"");
					VI(i, a, "Option \"ExaNoComposite\" \"false\"");
					VI(i, a, "Option \"XAANoOffscreenPixmaps\" \"true\"");
					VI(i, a, "Option \"EXANoUploadToScreen\" \"true\"");
					break;
				case CARD_NVIDIA:
					VI(i, a, "Option \"NoLogo\" \"True\"");
					VI(i, a, "Option \"TripleBuffer\" \"True\"");
					VI(i, a, "Option \"NvAGP\" \"3\"");
					VI(i, a, "Option \"RenderAccel\" \"True\"");
					VI(i, a, "Option \"HWcursor\" \"On\"");
					VI(i, a, "Option \"AllowGLXWithComposite\" \"True\"");
					VI(i, a, "Option \"AddARGBGLXVisuals\" \"True\"");
					VI(i, a, "Option \"DPMS\" \"True\"");
				case CARD_ATI:
					VI(i, a, "Option \"XAANoOffscreenPixmaps\" \"true\"");
					VI(i, a, "Option \"AddARGBGLXVisuals\" \"True\"");
					break;

			}
			VI(i, a, "Identifier \"" + identifier + "\"");
			VI(i, a, "Driver \"" + driver + "\"");
			VI(i, a, "Section \"Device\"");
			break;
		}
	}
	WriteFileStrings(filename, conf);
}

void enableSynaptics(string filename) {
}
