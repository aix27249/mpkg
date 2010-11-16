#include <mpkg/libmpkg.h>
#include <mpkg/depreactor.h>
#include <queue>
/*vector<PACKAGE *> getChilds(PACKAGE *ptr, map<PACKAGE *, PACKAGE *> path) {
	map<PACKAGE *, PACKAGE *>::iterator it;
	vector<PACKAGE *> ret;
	for (it=path.begin(); it<path.end(); it++) {
		if ((*it).second!=ptr) continue;
		ret.push_back((*it).first);
	}
	return ret;
}*/

void a_star (PACKAGE *start, PACKAGE *goal, map<PACKAGE *, PACKAGE *> dpath) {
	vector<PACKAGE *> closed;
	queue<PACKAGE *> q;
	q.push(start);
	bool queueExtend;
	printf("START: %s [%p]\nGOAL: %s [%p]\n", start->get_name().c_str(), start, goal->get_name().c_str(), goal);
	PACKAGE *p, *next, *prevRoot;
	vector<PACKAGE *> path;
	while (!q.empty()) {
		p = q.front();
		printf("%s [%p]\n", p->get_name().c_str(), p);
		map<PACKAGE *, PACKAGE *>::iterator it;
		queueExtend = false;
		for (it=dpath.begin(); it!=dpath.end(); it++) {
			if ((*it).second == p) {
				queueExtend = true;
				next = (*it).first;
				q.push(next);
				path.push_back(next);
			}
		}
		if (p==goal) {
			printf("GOAL in %d steps!\n", path.size());
			for (size_t i=0; i<path.size(); ++i) {
				cout << i << ": " << path[i]->get_name() << endl;
				q.pop();
			}
			return;
		}
		if (!queueExtend) {
			prevRoot = p;
		}

		q.pop();
	}

}

int main(int argc, char **argv) {
	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");
	string mode = argv[1];
	string pkg1 = argv[2];
	string pkg2 = argv[3];

	PACKAGE_LIST packages;
	SQLRecord sqlSearch;

	_cmdOptions["sql_readonly"]="yes";
	mpkg *core = new mpkg;
	core->get_packagelist(sqlSearch, &packages);
	delete core;

	string corename1, corename2, data;
	PACKAGE *ptr1 = NULL, *ptr2 = NULL;
	// First, check if such package exists
	for (size_t i=0; i<packages.size(); ++i) {
		if (mode=="remove" && !packages[i].installed()) continue;
		if (mode=="install" && packages[i].installed()) continue;
		if (packages[i].get_name()==pkg1) {
			corename1 = packages[i].get_corename();
			ptr1 = packages.get_package_ptr(i);
		}
		else if (packages[i].get_name()==pkg2) {
			corename2 = packages[i].get_corename();
			ptr2 = packages.get_package_ptr(i);
		}
		if (!corename1.empty() && !corename2.empty()) break;
	}
	printf("Searching dependency path from %s to %s\n", corename1.c_str(), corename2.c_str());
	if (corename1.empty()) {
		mError(_("Package ") + pkg1 + _(" not installed"));
		return 0;
	}
	if (corename2.empty()) {
		mError(_("Package ") + pkg2 + _(" not installed"));
		return 0;
	}

	vector<DEPENDENCY> *dep;

	DepReactor reactor;
	reactor.setPackageDB(packages);
	vector<PACKAGE *> iQueue, rQueue;
	if (mode=="install") iQueue.push_back(ptr1);
	else if (mode=="remove") rQueue.push_back(ptr1);
	reactor.setInstallQueue(iQueue);
	reactor.setRemoveQueue(rQueue);
	reactor.process();
	vector<PACKAGE *> reactorResults;
	map<PACKAGE *, PACKAGE *> depPath;
        if (mode=="install") {
		reactorResults = reactor.getFinalInstallList();
		depPath = reactor.getDepInstallPaths();
	}
	if (mode=="remove") {
		reactorResults = reactor.getFinalRemoveList();
		depPath = reactor.getDepRemovePaths();
	}
	printf("Res: %d, path: %d\n", reactorResults.size(), depPath.size());
	corename1.clear();
	corename2.clear();
	for (size_t i=0; i<reactorResults.size(); ++i) {
		if (reactorResults[i]->get_name()==pkg1) {
			corename1 = packages[i].get_corename();
			ptr1 = reactorResults[i];
		}
		else if (reactorResults[i]->get_name()==pkg2) {
			corename2 = reactorResults[i]->get_corename();
			ptr2 = reactorResults[i];
		}
		if (!corename1.empty() && !corename2.empty()) break;
	}


	a_star(ptr1, ptr2, depPath);

	vector<PACKAGE *> pathList;

	cout << endl;
	cout << "----------------------------" << endl;
	for (size_t i=0; i<pathList.size(); ++i) {
		cout << pathList[i]->get_name();
		if (i>0) cout << " -> ";
	}
	cout << endl;


}
