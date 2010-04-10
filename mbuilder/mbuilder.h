#ifndef _MBUILDER_H__
#define _MBUILDER_H__
#include <mpkg/libmpkg.h>

class MBuilder {
	public: 
		MBuilder(){
			if (getenv("EDITOR")) editor = getenv("EDITOR");
			if (editor.empty() && getenv("VISUAL")) editor = getenv("VISUAL");
			if (editor.empty()) editor = "mcedit";
		}
		~MBuilder(){}
		void mainMenu();
		void generalDataMenu();
		MetaSrcPackage *metapkg; // FIXME: not implemented, should be derived from MetaPackage within metaframe
	private:
		void dependenciesMenu();
		void descriptionsMenu();
		void tagsMenu();
		void maintainerMenu();
		void buildOptionsMenu();
		void build();
		void saveMenu();
		void buildSystemChooseMenu();
		void keysMenu();
		void patchesMenu();
		void prebuildMenu();
		void postbuildMenu();
		void advancedBuildOptionsMenu();
		void addKeyMenu();
		void editKeyMenu(int);
		void deleteKeysMenu();
		void deleteTagsMenu();

		string dir;

		string editor;
};

#endif
