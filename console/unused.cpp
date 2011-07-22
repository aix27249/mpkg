#include <mpkg/libmpkg.h>

int main(int argc, char **argv) {
	mpkg core;
	PACKAGE_LIST packages;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_installed", 1);
	core.get_packagelist(sqlSearch, &packages);
	string tag;
       	if (argc>1) tag	= argv[1];


	vector<PACKAGE *> list;
	bool used;
	for (size_t z=0; z<packages.size(); ++z) {
		used = false;
		for (size_t i=0; !used && i<packages.size(); ++i) {
			for (size_t t=0; !used && t<packages[i].get_dependencies().size(); ++t) {
				if (packages[i].get_dependencies().at(t).get_package_name()==packages[z].get_corename()) {
					used = true;
				}
			}
		}
		if (!used) list.push_back(packages.get_package_ptr(z));
	}


	vector<PACKAGE *> selected_list;
	// Now, we have list of unused packages in list variable.
	for (size_t i=0; i<list.size(); ++i) {
		if (tag.empty() || list[i]->isTaggedBy(tag)) selected_list.push_back(list[i]);
	}


	// Output results
	fprintf(stderr, _("Found %d unused packages:\n"), (int) selected_list.size());
	for (size_t i=0; i<selected_list.size(); ++i) {
		cout << selected_list[i]->get_name() << " (" << humanizeSize(selected_list[i]->get_installed_size()) << ")" << endl;
	}
	return 0;
	
}

