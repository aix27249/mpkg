#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
using namespace std;
int getTerminalWidth();
void clearRow(int rowLength = -1);
#define SAYMODE_INLINE 0
#define SAYMODE_INLINE_END 1
#define SAYMODE_INLINE_START 2
#define SAYMODE_NEWLINE 3

#define	PMODE_AUTO 0
#define	PMODE_NONE 1

void msay(string data, int mode=SAYMODE_INLINE);

class ProgressDisplay {
	public:
		ProgressDisplay(int currentId=0, int totalCount = 0, string action = "PROCESSING");
		~ProgressDisplay();
		void setCurrentID(int current);
		void setTotalCount(int total);
		void setCurrentAction(string action);
		void setCurrentObjectName(string name);
		void setCurrentObjectDetails(string details);
		void say(string currentStatus);
		void setHeader(string header); // Graphic & Dialog mode only, in CLI mode this function has no effect
		void setProgressTotal(int total);
		void setProgressNow(int now);
		void showPercentMode(int pMode=PMODE_AUTO);
};
