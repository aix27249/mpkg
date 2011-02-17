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


	ncInterface.setStrings();
	dialogMode = true;
	
	TextSetup app;
	return app.run();

}
