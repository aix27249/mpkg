#include "eventhandler.h"
TextEventHandler *eventHandler;
void updateProgressData(ItemState a) {
	eventHandler->setDetailsTextCall(a.name + ": " + a.currentAction);
	if (a.totalProgress>=0 && a.totalProgress<=100) eventHandler->setProgressCall(a.totalProgress);
}

void parseConfig(map<string, string> *_strSettings, vector<TagPair> *_users, vector<PartConfig> *_partConfigs, vector<string> *_additional_repositories) {
	// TODO: implement parser
}

int main(int argc, char **argv) {
	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");

	// For mpkg, note that we copy config to temp directory	
	CONFIG_FILE="/tmp/mpkg.xml";
	mConfig.configName=CONFIG_FILE;
	unlink("/tmp/packages.db");
	unlink("/tmp/mpkg.xml");
	if (!FileExists("/usr/share/setup/packages.db")) {
		mError("Oops, no database template in /usr/share/setup/packages.db!");
		return 1;
	}
	if (!FileExists("/usr/share/setup/mpkg-setup.xml")) {
		mError("Oops, no config template in /usr/share/setup/mpkg-setup.xml!");
		return 1;
	}
	system("cp /usr/share/setup/packages.db /tmp/packages.db");
	system("cp /usr/share/setup/mpkg-setup.xml /tmp/mpkg.xml");


	ncInterface.setStrings();
	dialogMode = true;
	
	map<string, string> strSettings;
	vector<PartConfig> partConfigs;
	vector<TagPair> users;
	vector<string> additional_repositories;

	parseConfig(&strSettings, &users, &partConfigs, &additional_repositories); // TO BE IMPLEMENTED!
	eventHandler = new TextEventHandler;
	AgiliaSetup agiliaSetup;
	agiliaSetup.registerStatusNotifier(eventHandler);
	agiliaSetup.run(strSettings, users, partConfigs, additional_repositories, &updateProgressData);
	ncInterface.showMsgBox(_("Installation finished!"));
	delete eventHandler;
	
}
