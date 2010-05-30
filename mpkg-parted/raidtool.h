#ifndef RAIDTOOL_H_
#define RAIDTOOL_H_
#include <string>
#include <vector>
using namespace std;
struct RaidArray {
	string md_dev;
	string level;
	string size;
	string extra_description;
	vector<string> devices;
};

int runRaidTool();
int raidCreateMenu();
//int raidManageMenu();
int raidStopMenu();
int raidMainMenu();
int raidAssembleMenu();

vector<RaidArray> getActiveRaidArrays();
int createRaid(string md_dev, string level, int count, vector<string> partitions);
int assembleRaid(string md_dev, vector<string> partitions);
int stopRaidArray(string md_dev);

void initRAID();
#endif
