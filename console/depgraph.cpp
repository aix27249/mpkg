#include <mpkg/libmpkg.h>

int main(int argc, char **argv) {
	if (argc<3 || strcmp(argv[0], "--help")==0) {
		printf("Usage: %s REPOSITORY_URL OUTPUT_FILE\n", argv[0]);
		return 1;
	}
	vector<string> g;
	PACKAGE_LIST pkglist;
	Repository rep;
	string reppath = argv[1];
	rep.get_index(reppath, &pkglist);
	g.push_back("digraph G {");
	for (size_t i=0; i<pkglist.size(); ++i) {
		for (size_t d=0; d<pkglist[i].get_dependencies().size(); ++d) {
			g.push_back("\t" + pkglist[i].get_name() + " $$ " + pkglist[i].get_dependencies().at(d).get_package_name() + ";");
			strReplace(&g[g.size()-1], "-", "_");
			strReplace(&g[g.size()-1], "$$", "->");
			strReplace(&g[g.size()-1], "+", "plus");

		}
	}
	g.push_back("}");
	WriteFileStrings(argv[2], g);
	return 0;
}
