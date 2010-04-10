#include <mpkg/libmpkg.h>
#include "mbuilder.h"
void MBuilder::buildSystemChooseMenu() {
	vector<MenuItem> menu;
	menu.push_back(MenuItem("autotools", _("Generic build system, used in most programs"), \
				_("Main configuration script or file: configure\n\
				   Recommended flags:\n --prefix=/usr\n --libdir=/usr/lib$LIBSUFFIX\n --mandir=/usr/man\n --sysconfdir=/etc\n --localstatedir=/var")));
	menu.push_back(MenuItem("scons", _("SCons - python based build system. Used in some projects"), \
				_("Main configuration script or file: SConstruct\n\
				   Recommended flags:\n --prefix=/usr\n --libdir=/usr/lib$LIBSUFFIX\n")));
	menu.push_back(MenuItem("cmake", _("Modern and very popular build system, used in all KDE programs and many others"), \
				_("Main configuration script or file: CMakeLists.txt\n\
				   Recommended flags:\n -DCMAKE_INSTALL_PREFIX=/usr\n -DCMAKE_LIBDIRSUFFIX=$LIBSUFFIX")));
	menu.push_back(MenuItem("custom", _("Custom set of commands. AVOID IT! Use script instead"), \
				_("This option is deprecated and should not be used. It was left here for compatibility reasons")));
	menu.push_back(MenuItem("script", _("Custom script. Use this if no predefined build systems fits your requirements"), \
				_("There are number of global variables available in script. For example, $SRC, $PKG, $DATADIR, $LIBSUFFIX. See online documentation.")));
	menu.push_back(MenuItem("qmake4", _("Qt4 build system"), \
				_("Main configuration script or file: *.pro\n\
				   Recommended flags:\n PREFIX=/usr")));
	menu.push_back(MenuItem("python", _("Python build system. Used in most python programs"), \
				_("Main configuration script or file: setup.py\n\
				   Recommended flags:\n --prefix=/usr")));
	menu.push_back(MenuItem("make", _("Simple Makefile"), \
				_("Main configuration script or file: Makefile\n\
				   Recommended flags:\n PREFIX=/usr\nNote that flags can widely vary in this case")));
	menu.push_back(MenuItem("perl", _("Perl build system"), \
				_("Main configuration script or file: Makefile.pl\n\
				   Recommended flags:\n --prefix=/usr\n --libdir=/usr/lib$LIBSUFFIX\n --mandir=/usr/man\n --sysconfdir=/etc\n --localstatedir=/var")));
	menu.push_back(MenuItem("waf", _("Waf build system"), \
				_("Main configuration script or file: waf\n\
				   Recommended flags:\n --prefix=/usr\n --libdir=/usr/lib$LIBSUFFIX\n --mandir=/usr/man\n --sysconfdir=/etc\n --localstatedir=/var")));
	string r = ncInterface.showMenu2(_("Select build system:"), menu);
	if (r.empty()) return;
	metapkg->data->buildsystem = r;

}

void MBuilder::editKeyMenu(int i) {
	string k, v;
	ncInterface.setSubtitle(_("Edit configure key"));
	k = ncInterface.showInputBox(_("Enter key name:"), metapkg->data->configure_keys[i]);
	if (k.empty()) return;
	v = metapkg->data->configure_values[i];
	
	if (ncInterface.showInputBox(_("Enter key value:"), &v)) {
		metapkg->data->configure_keys[i] = k;
		metapkg->data->configure_values[i] = v;
	}
}

void MBuilder::deleteKeysMenu() {
	vector<MenuItem> menu;
	string tmp;
	for (size_t i=0; i<metapkg->data->configure_keys.size() && i<metapkg->data->configure_values.size(); ++i) {
		menu.push_back(MenuItem(metapkg->data->configure_keys[i], metapkg->data->configure_values[i], "", false));
	}
	ncInterface.setSubtitle(_("Delete configure keys"));
	ncInterface.showExMenu(_("Mark keys which you want to be deleted:"), menu);
	vector<string> k = metapkg->data->configure_keys, v = metapkg->data->configure_values;
	metapkg->data->configure_keys.clear();
	metapkg->data->configure_values.clear();
	for (size_t i=0; i<menu.size(); ++i) {
		if (!menu[i].flag) {
			metapkg->data->configure_keys.push_back(k[i]);
			metapkg->data->configure_values.push_back(v[i]);
		}
	}
}

void MBuilder::addKeyMenu() {
	string k, v;
	ncInterface.setSubtitle(_("Add configure key"));
	k = ncInterface.showInputBox(_("Enter key name:"));
	if (k.empty()) return;
	
	if (ncInterface.showInputBox(_("Enter key value:"), &v)) {
		metapkg->data->configure_keys.push_back(k);
		metapkg->data->configure_values.push_back(v);
	}
}

void MBuilder::keysMenu() {
	vector<MenuItem> menu;
	string tmp;
	int r;
	while (true) {
		menu.clear();

		for (size_t i=0; i<metapkg->data->configure_keys.size() && i<metapkg->data->configure_values.size(); ++i) {
			menu.push_back(MenuItem(metapkg->data->configure_keys[i], metapkg->data->configure_values[i]));
		}
		menu.push_back(MenuItem("", ""));
		menu.push_back(MenuItem(_("ADD"), _("Add key")));
		menu.push_back(MenuItem(_("DELETE"), _("Delete some keys"), _("Opens menu where you can select which keys you want to remove")));
		menu.push_back(MenuItem(_("Back"), _("Go back")));

		ncInterface.setSubtitle(_("Configure keys"));
		r = ncInterface.showMenu("Select what do you want. Select key to edit it", menu);
		if (r==-1 || r==(int) menu.size()-1) return;
		if (r<(int) menu.size()-4) editKeyMenu(r);
		if (r==(int) menu.size()-3) addKeyMenu();
		if (r==(int) menu.size()-2) deleteKeysMenu();
	}
}

void MBuilder::patchesMenu() {
	ncInterface.showMsgBox("NOT IMPLEMENTED YET");
}

void MBuilder::prebuildMenu() {
	string text = metapkg->sp->readPrebuildScript();
	string tmp = get_tmp_file();
	WriteFile(tmp + ".sh", text);
	ncInterface.uninit();
	system(editor + " " + tmp + ".sh");
	metapkg->sp->setPrebuildScript(ReadFile(tmp + ".sh"));
	unlink(tmp.c_str());
	unlink(string(tmp+".sh").c_str());
}

void MBuilder::postbuildMenu() {
	string text = metapkg->sp->readBuildScript();
	string tmp = get_tmp_file();
	WriteFile(tmp + ".sh", text);
	ncInterface.uninit();
	system(editor + " " + tmp + ".sh");
	metapkg->sp->setBuildScript(ReadFile(tmp + ".sh"));
	unlink(tmp.c_str());
	unlink(string(tmp+".sh").c_str());
}

void MBuilder::advancedBuildOptionsMenu() {
	ncInterface.showMsgBox("NOT IMPLEMENTED YET");
}

void MBuilder::buildOptionsMenu() {
	vector<MenuItem> menu;
	string r;
	string tmp;
	while (r!=_("Back")) {
		menu.clear();
		menu.push_back(MenuItem(_("URL"), metapkg->data->url));
		menu.push_back(MenuItem(_("Build system"), metapkg->data->buildsystem));
		menu.push_back(MenuItem(_("Keys"), _("Edit configure keys")));
		menu.push_back(MenuItem(_("Patches"), _("Edit patch list")));
		menu.push_back(MenuItem(_("Prebuild"), _("Edit pre-build script")));
		menu.push_back(MenuItem(_("Postbuild"), _("Edit post-build script")));
		menu.push_back(MenuItem(_("Advanced"), _("Edit advanced build options")));
		menu.push_back(MenuItem(_("Back"), _("Go back to main menu")));
		r = ncInterface.showMenu2(_("Edit build options"), menu);
		if (r.empty()) return;
		if (r==_("URL")) {
			tmp = metapkg->data->url;
			if (ncInterface.showInputBox(_("Enter source URL:"), &tmp)) metapkg->data->url = tmp;
		}
		if (r==_("Build system")) buildSystemChooseMenu();
		if (r==_("Keys")) keysMenu();
		if (r==_("Patches")) patchesMenu();
		if (r==_("Prebuild")) prebuildMenu();
		if (r==_("Postbuild")) postbuildMenu();
		if (r==_("Advanced")) advancedBuildOptionsMenu();
	}
}
void MBuilder::saveMenu() {
	ncInterface.showInfoBox(_("Saving package..."));
	ncInterface.uninit();
	if (metapkg->pkgFilename.empty()) {

		ncInterface.setSubtitle(_("Select directory where do you want to save package:"));
		if (dir.empty()) dir = ncInterface.ncGetOpenDir();
		ncInterface.uninit();
		if (dir.empty()) return;
		else if (metapkg->savePackage(dir)) {
			ncInterface.showMsgBox(_("Package successfully saved"));
		}
		return;
	}
	if (metapkg->savePackage()) ncInterface.showMsgBox(_("Package successfully saved"));
	else ncInterface.showMsgBox(_("Failed to save package"));
}

void MBuilder::descriptionsMenu() {
	vector<MenuItem> menu;
	menu.push_back(MenuItem(_("Summary"), _("Edit short description")));
	menu.push_back(MenuItem(_("Full"), _("Edit full description")));
	menu.push_back(MenuItem(_("Back"), _("Back to general data menu")));
	int r=0;
	string tmp;
	while (r!=2) {
		r = ncInterface.showMenu(_("Summary: ") + metapkg->data->pkg.get_short_description() + "\n\n" + metapkg->data->pkg.get_description(), menu);
		switch (r) {
			case 0:
				tmp = metapkg->data->pkg.get_short_description();
				if (ncInterface.showInputBox(_("Edit package summary:"), &tmp)) metapkg->data->pkg.set_short_description(tmp);
				break;
			case 1:
				tmp = get_tmp_file();
				WriteFile(tmp, metapkg->data->pkg.get_description());
				ncInterface.uninit();
				system(editor + " " + tmp);
				metapkg->data->pkg.set_description(ReadFile(tmp));
				unlink(tmp.c_str());
				break;
			case 2:
				return;
		}
	}
}
void MBuilder::deleteTagsMenu() {
	vector<MenuItem> menu;
	vector<string> tags;
	string tmp;
	for (size_t i=0; i<metapkg->data->pkg.get_tags().size(); ++i) {
		menu.push_back(MenuItem(IntToStr(i), metapkg->data->pkg.get_tags().at(i), "", false));
	}
	ncInterface.setSubtitle(_("Delete tags"));
	ncInterface.showExMenu(_("Mark keys which you want to be deleted:"), menu);

	for (size_t i=0; i<menu.size(); ++i) {
		if (!menu[i].flag) {
			tags.push_back(metapkg->data->pkg.get_tags().at(i));
		}
	}
	metapkg->data->pkg.set_tags(tags);
}
void MBuilder::tagsMenu() {
	vector<MenuItem> menu;
	vector<string> tags;
	string tmp;
	int r;
	while (true) {
		menu.clear();
		tags = metapkg->data->pkg.get_tags();

		for (size_t i=0; i<tags.size(); ++i) {
			menu.push_back(MenuItem(IntToStr(i), tags[i]));
		}
		menu.push_back(MenuItem("", ""));
		menu.push_back(MenuItem(_("ADD"), _("Add tag")));
		menu.push_back(MenuItem(_("DELETE"), _("Delete some tags"), _("Opens menu where you can select which tags you want to remove")));
		menu.push_back(MenuItem(_("Back"), _("Go back")));

		ncInterface.setSubtitle(_("Tags"));
		r = ncInterface.showMenu("Select what do you want. Select tag to edit it", menu);
		if (r==-1 || r==(int) menu.size()-1) return;
		if (r<(int) menu.size()-4) {
			tmp = tags[r];
			if (ncInterface.showInputBox(_("Edit tag:"), &tmp)) {
				tags[r]=tmp;
			}
		}
		if (r==(int) menu.size()-3) {
			tmp = ncInterface.showInputBox(_("Add new tag:"));
			if (!tmp.empty()) tags.push_back(tmp);
		}
		if (r==(int) menu.size()-2) deleteTagsMenu();
		metapkg->data->pkg.set_tags(tags);
	}

	
}

void MBuilder::maintainerMenu() {
	vector<MenuItem> menu;
	int r=0;
	string tmp;
	while (r!=-1) {
		menu.clear();
		menu.push_back(MenuItem(_("Name:"), metapkg->data->pkg.get_packager()));
		menu.push_back(MenuItem(_("E-Mail:"), metapkg->data->pkg.get_packager_email()));
		menu.push_back(MenuItem(_("Back"), _("Back to data menu")));
		r = ncInterface.showMenu(_("Maintainer data"), menu);
		switch (r) {
			case 0:
				tmp = metapkg->data->pkg.get_packager();
				if (ncInterface.showInputBox(_("Enter maintainer name:"), &tmp)) metapkg->data->pkg.set_packager(tmp);
				break;
			case 1:
				tmp = metapkg->data->pkg.get_packager_email();
				if (ncInterface.showInputBox(_("Enter maintainer email:"), &tmp)) metapkg->data->pkg.set_packager_email(tmp);
				break;
			case 2:
				return;
		}
	}
}

void MBuilder::dependenciesMenu() {
	ncInterface.showMsgBox("NOT IMPLEMENTED YET");

}

void MBuilder::build() {
	ncInterface.showInfoBox(_("Saving package to build..."));
	ncInterface.showInfoBox(_("Saving package..."));
	ncInterface.uninit();
	string expectedSpkgName = metapkg->pkgFilename;
	if (metapkg->pkgFilename.empty()) {
		ncInterface.setSubtitle(_("Select directory where do you want to save package:"));
		if (dir.empty()) dir = ncInterface.ncGetOpenDir();
		ncInterface.uninit();
		if (dir.empty()) return;
		else if (!metapkg->savePackage(dir)) {
			ncInterface.showMsgBox(_("Failed to save package"));
			return;
		}
		expectedSpkgName = dir + "/" + metapkg->data->pkg.get_name() + "-" + metapkg->data->pkg.get_fullversion() + ".spkg";
	}
	else {
		if (!metapkg->savePackage()) {
			ncInterface.showMsgBox(_("Failed to save package"));
			return;
		}
	}

	system("mpkg build " + expectedSpkgName + " && echo 'Press any key to close' && read");
}


void MBuilder::generalDataMenu() {
	vector<MenuItem> menu;
	string header = "General data menu";
	string text = "";

	string r, tmp;
	while (r!=_("Back")) {
		menu.clear();
		menu.push_back(MenuItem(_("Name:"), metapkg->data->pkg.get_name()));
		menu.push_back(MenuItem(_("Version:"), metapkg->data->pkg.get_version()));
		menu.push_back(MenuItem(_("Arch:"), metapkg->data->pkg.get_arch()));
		menu.push_back(MenuItem(_("Build:"), metapkg->data->pkg.get_build()));
		menu.push_back(MenuItem(_("Descriptions"), _("Edit descriptions")));
		menu.push_back(MenuItem(_("Tags"), _("Edit tags")));
		menu.push_back(MenuItem(_("Maintaner"), _("Edit maintainer data")));
		menu.push_back(MenuItem(_("Dependencies"), _("Edit dependencies")));
		menu.push_back(MenuItem(_("Back"), _("Go to main menu")));
		ncInterface.setSubtitle(header);
		r = ncInterface.showMenu2("", menu);
		if (r==_("Name:")) {
			tmp=metapkg->data->pkg.get_name();
		       	if (ncInterface.showInputBox(_("Enter package name:"), &tmp)) metapkg->data->pkg.set_name(tmp);
		}
		if (r==_("Version:")) {
		       tmp = metapkg->data->pkg.get_version();
	       		if (ncInterface.showInputBox(_("Enter package version:"), &tmp)) metapkg->data->pkg.set_version(tmp);
		}
		if (r==_("Arch:")) {
		       tmp = metapkg->data->pkg.get_arch();
		       if (ncInterface.showInputBox(_("Enter package arch:"), &tmp)) metapkg->data->pkg.set_arch(tmp);
		}
		if (r==_("Build:")) {
			tmp = metapkg->data->pkg.get_build();
			if (ncInterface.showInputBox(_("Enter package build:"), &tmp)) metapkg->data->pkg.set_build(tmp);
		}
		if (r==_("Descriptions")) descriptionsMenu();
		if (r==_("Tags")) tagsMenu();
		if (r==_("Maintainer")) maintainerMenu();
		if (r==_("Dependencies")) dependenciesMenu();
		if (r.empty()) return;
	}
}

void MBuilder::mainMenu() {
	vector<MenuItem> menu;
	string r;
	string header = "Main menu";
	string text = "";
	while (r!=_("Quit")) {
		menu.clear();
		menu.push_back(MenuItem(_("General data"), _("Edit name, version, arch and build")));
		menu.push_back(MenuItem(_("Build options"), _("Edit building options")));
		menu.push_back(MenuItem(_("Save"), _("Save package")));
		menu.push_back(MenuItem(_("Build"), _("Build package")));
		menu.push_back(MenuItem(_("Quit"), _("Just quit")));
		ncInterface.setSubtitle(header);
		r = ncInterface.showMenu2(text, menu);
		if (r.empty()) return;
		if (r==_("General data")) generalDataMenu();
		if (r==_("Build options")) buildOptionsMenu();
		if (r==_("Save")) saveMenu();
		if (r==_("Build")) build();
	}
}

int main(int argc, char **argv) {
	dialogMode = true;
	// There are number of modes:
	// 1. SPKG editor
	// 2. Package editor (or, maybe left it for mpkg-setmeta?)
	// 3. SPKG creation wizard
	// 
	// So, let's implement editor first!
	MBuilder mbuilder;
	ncInterface.setTitle(_("MBuilder: an spkg editor"));
	string filename;
	if (argc>1) filename = argv[1];
	// TODO: create empty tree if no filename provided
	mbuilder.metapkg = new MetaSrcPackage(filename);
	// Main menu
	
	mbuilder.mainMenu();
	return 0;
	
}
