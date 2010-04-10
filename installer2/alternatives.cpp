#include <mpkgsupport/mpkgsupport.h>
#include <nwidgets/ncurses_if.h>
#include "default_paths.h"
void loadAltDescriptions(vector<MenuItem> *altMenu) {
	for (size_t i=0; i<altMenu->size(); ++i) {
		if (altMenu->at(i).tag=="bfs") altMenu->at(i).value=_("BFS kernel");
		else if (altMenu->at(i).tag=="cleartype") altMenu->at(i).value=_("Cleartype-patched fonts");
	}
}


int main() {
	// Here is a problem: we already need to know list of all possible alternative flags.
	// In old installer, there was a package list already available, but here we cannot do that.
	// So, we cannot know the list, and we need to know it. 
	// What the hell we can do here?
	// First of all, if we specify a flag that is not in packages anymore - it isn't a problem.
	// Secondary, in theory we can get some special file from repo. But this requires support from repository.
	// As last resort, we can try to fetch package list twice, and parse it. In case of DVD, there will be cache, 
	//  in case of network - it will be no serious affect.
	//
	//  For now, we will use hardcoded list, because the amount of these packages are still very low.
	dialogMode = true;
	CursesInterface ncInterface;
	vector<MenuItem> menuItems;
	menuItems.push_back(MenuItem("bfs", ""));
	menuItems.push_back(MenuItem("cleartype", ""));
	loadAltDescriptions(&menuItems);
	if (ncInterface.showExMenu(_("There are some package alternatives. Please, mark ones which you want to use."), menuItems)==-1) return 1;
	vector<string> appliedAlternatives;
	string __applAlt;
	for (size_t i=0; i<menuItems.size(); ++i) {
		if (menuItems[i].flag) {
			appliedAlternatives.push_back(menuItems[i].tag);
		}
	}
	WriteFileStrings(SETUPCONFIG_ALTERNATIVES, appliedAlternatives);
	return 0;
	
}
