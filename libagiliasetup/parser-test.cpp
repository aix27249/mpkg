#include "agiliasetup.h"

int main(int argc, char **argv) {
	map<string, string> settings;
	map<string, map<string, string> > partitions;
	vector<string> repositories;
	loadSettings(argv[1], settings, repositories, partitions);
	saveSettings(string(argv[1]) + ".new", settings, repositories, partitions);
	return 0;


}
