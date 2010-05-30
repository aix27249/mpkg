#include "raidtool.h"

int main(int argc, char **argv) {
	dialogMode = true;
	cdromList = getCdromList();
	runRaidTool();
}
