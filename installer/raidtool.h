#ifndef RAIDTOOL_SETUP_H_
#define RAIDTOOL_SETUP_H_
#include <mpkg-parted/mpkg-parted.h>
#include <mpkg-parted/raidtool.h>
#include <mpkg/libmpkg.h>
int runRaidTool();
int raidCreateMenu();
int raidStopMenu();
int raidMainMenu();
int raidAssembleMenu();
#endif
