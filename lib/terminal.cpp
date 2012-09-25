#include "terminal.h"
//#include "string_operations.h"
#include "debug.h"
#include "version.h"
#include "config.h"
int prevRowLength=0;
int getTerminalWidth() {
	struct winsize tios;
	ioctl(0, TIOCGWINSZ, &tios);
	return tios.ws_col;
}
void clearRow(int rowLength)
{
	if (dialogMode || htmlMode) return;
	int oldRowLength = prevRowLength;
	if (rowLength>=oldRowLength) return;
	string buff;
	for (int i=0; i<oldRowLength; i++) {
		buff+=" ";
	}
	say("\r%s", buff.c_str());
	fflush(stdout);
}
void truncateDataStr(string& data) {
	int sz = getTerminalWidth();
	if ((unsigned int) sz>utf8strlen(data)) return;
	strReplace(&data, CL_BLACK, "");
	strReplace(&data, CL_RED, "");
	strReplace(&data, CL_GREEN, "");
	strReplace(&data, CL_YELLOW, "");
	strReplace(&data, CL_BLUE, "");
	strReplace(&data, CL_WHITE, "");
	strReplace(&data, CL_5, "");
	strReplace(&data, CL_6, "");
	strReplace(&data, CL_7, "");
	strReplace(&data, CL_8, "");
/*	string p1, p2;
	p1 = data.substr(0, (sz/2-2));
	p2 = data.substr(data.length()-(sz/2)+2);
	while (utf8strlen(p1)<(sz/2)-3) {
		p1 += data[p1.size()];
	}	
	while (utf8strlen(p2)<(sz/2)-3) {
		p2 = data[data.size()-p2.size()-1] + p2;
	}	*/
	data = data.substr(0, (sz/2)-2) + "..." + data.substr(data.size()-(sz/2)+2);
	//data = p1 + "..." + p2;

}
void msay (string data, int mode, FILE *output)
{
	currentStatus = data;
	//if (!dialogMode && !htmlMode && errorManagerMode==EMODE_CONSOLE) truncateDataStr(data); // disabled due to segfaults in Qt mode (and, maybe, in others too)
	if (!dialogMode && !htmlMode) {
		switch (mode) {
			case SAYMODE_INLINE:
				clearRow(utf8strlen(data));
				fprintf(output, "\r%s", data.c_str());
				msayState = true;
				break;
			case SAYMODE_INLINE_START:
				msayState = true;
				fprintf(output, "\r%s", data.c_str());
				break;
			case SAYMODE_INLINE_END:
				clearRow();
				fprintf(output, "\r%s\n", data.c_str());
				msayState = false;
				break;
			case SAYMODE_NEWLINE:
				fprintf(output, "%s\n", data.c_str());
				msayState = false;
				break;
		}		
		fflush(stdout);
	}
	prevRowLength = utf8strlen(data);
}
