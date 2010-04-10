#include "raidtool.h"
#include "parted_tools.h"

int main(int argc, char **argv) {
	dialogMode = true;
	cdromList = getCdromList();
	runRaidTool();
}
