/*
 * Package source
 * Install type
 * Video driver
 * Partition editor
 * Mount points
 * Boot loader
 * Advanced boot options
 * Root password
 * Create user
 * Network settings
 * Timezone
 */
#include "textsetup.h"
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
	
	TextSetup app;
	return app.run();

}
