//#include "libmpkg.h"
#include <mpkgsupport/mpkgsupport.h>
#include "ncurses_if.h"
#define ENTER 10
#define ESCAPE 27
#define TAB 9
#define BACKSPACE 127
#define BACKSPACE2 263
#define INSERT 331
#define DELETE 330
#define BGFILL _BGF.c_str()
#define SPACE      32
#define PAGE_UP    339
#define PAGE_DOWN  338

bool dialogMode = false;

#include <time.h>
static void fillwin2(WINDOW *win, string ch)
{
	int y, x;
	int y1, x1;
	getmaxyx(win, y1, x1);
	for (y = 0; y < y1; y++) {
		wmove(win, y, 0);
		for (x = 0; x < x1; x++)
			waddstr(win, ch.c_str());
	}
}

CursesInterface::CursesInterface(bool _initLocale)
{
	lastUsedInterface = LASTUI_NONE;
	// Zerofying windows
	mainWindow = 0;
	titleWindow = 0;
	subTitleWindow = 0;
	boxWindow = 0;
	textWindow = 0;
	boxShadowWindow = 0;
	button1 = 0;
	button2 = 0;
	progressWindow = 0;
	progressLineWindow = 0;
	currentButton = 0;
	init_complete = false;
	items=NULL;
	initLocale=_initLocale;
	okStr = _("OK");
	cancelStr = _("CANCEL");
	_BGF = "\342\226\222";

}
void CursesInterface::init()
{
	if (init_complete) return;
	// NCURSES INIT
	
	// Setting locale	
	if (initLocale) setlocale(LC_ALL, "");
	if (setlocale(LC_ALL, NULL) && setlocale(LC_ALL, NULL)==string("C")) _BGF=" ";
	// Screen init
	WINDOW * scr = NULL;
       	scr = initscr();
	if (scr == NULL) {
		//mError(string("NCurses initialization failed.")); // FIXME
		exit(1);
	}

	if (has_colors()) start_color();
	
	// Color map
	init_pair(1, COLOR_WHITE, COLOR_GREEN); // Выделенная кнопка
    	init_pair(2, COLOR_WHITE, COLOR_WHITE);// Невыделенная кнопка
    	init_pair(3, COLOR_RED, COLOR_WHITE);
    	init_pair(4, COLOR_CYAN, COLOR_BLACK);
    	init_pair(5, COLOR_WHITE, COLOR_BLACK);
    	init_pair(6, COLOR_MAGENTA, COLOR_GREEN);
    	init_pair(7, COLOR_BLUE, COLOR_BLACK);
    	init_pair(8, COLOR_WHITE, COLOR_BLACK);
	init_pair(9, COLOR_WHITE, COLOR_BLUE); // хзшто, но кажыцо ввод
	init_pair(10,COLOR_BLACK,COLOR_WHITE); // Фон окон
	init_pair(11,COLOR_WHITE,COLOR_GREEN); // Зеленая поебта - заголовки, прогрессбар, и тд
	init_pair(12,COLOR_CYAN,COLOR_RED);
	init_pair(13,COLOR_WHITE,COLOR_BLUE);
	init_pair(14,COLOR_WHITE,COLOR_GREEN);

	curs_set(0);
	noecho();
	keypad(stdscr, true);
	init_complete = true;
	//mLog("init ok");

}
void CursesInterface::setStrings() {
	okStr = _("OK");
	cancelStr = _("CANCEL");
}
void CursesInterface::uninit() {
	if (!init_complete || !dialogMode) return;
	//if (items) delete items;
	endwin();
//	delscreen();
	init_complete = false;
}
CursesInterface::~CursesInterface()
{
	if (!dialogMode || !init_complete) return;
	uninit();

}
void CursesInterface::drawTitles()
{
	if (!dialogMode) return;
	init();
	// А теперь - мегафича: ЧАСЫ!
	
	time_t time_now = time(NULL);
	if (setupMode) time_now += 86400;
	struct tm *time_struct = NULL;
	time_struct = localtime(&time_now);
	string hr = IntToStr(time_struct->tm_hour); 
	if (hr.length() < 2) hr = "0" + hr;
	string mn = IntToStr(time_struct->tm_min);
	if (mn.length() < 2) mn = "0" + mn;

	string time_str = hr + ":" + mn;
	string date_str = getWeekDay(time_struct->tm_wday) + ", "+IntToStr(time_struct->tm_mday) + " " + getMonthName(time_struct->tm_mon) + " "+IntToStr(1900 + time_struct->tm_year);
	time_str = date_str + ", " + time_str;
//	string time_str = ctime(&time_now);
	//int shit = time_str.find_first_of("\n\r\0");
	//time_str = time_str.substr(0, shit);
	fillwin2(titleWindow, " ");
	wmove(titleWindow, 0, 0);
	string title_tmp = "  " + title;
	while (utf8strlen(title_tmp)<COLS-2-utf8strlen(time_str)) title_tmp += " ";
       	title_tmp += time_str;
	waddstr(titleWindow, title_tmp.c_str());
	
	fillwin2(subTitleWindow, " ");
	wmove(subTitleWindow, 0,2);
	waddstr(subTitleWindow, subtitle.c_str());
}
void CursesInterface::setTitle(string text) {
	title = text;
}
void CursesInterface::setSubtitle(string text) {
	subtitle = text;
}
/*void createWindow(WINDOW * winPtr, int size_x, int size_y, int pos_x, int pos_y) {
	if (!dialogMode) return;
	if (winPtr) delwin(winPtr);
	winPtr = subwin(stdscr, size_x, size_y, pos_x, pos_y);
}*/

void CursesInterface::showInfoBox(string text)
{
	if (!dialogMode) return;
	init();
	lastUsedInterface = LASTUI_INFOBOX;
	// Sizes constants
	unsigned int box_width = COLS/2;
	unsigned int box_height = 2 + adjustStringWide2(text, box_width-4).size();	
	if (box_height < 6) box_height = 6;
	if ((int) box_height > LINES-3) box_height = LINES-3;
	unsigned int box_start_l = (int) (LINES - box_height)/2+1;
	if (box_start_l < 2) box_start_l = 2;
	unsigned int box_start_c = (unsigned int) (COLS - box_width)/2;
	// Title window
	titleWindow = subwin(stdscr, 1, COLS, 0, 0);
	wattron(titleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(titleWindow, " ");
	
	// Main background
	mainWindow = subwin(stdscr, LINES-1, COLS, 1, 0);
	wattron(mainWindow, COLOR_PAIR(10));
	fillwin2(mainWindow, BGFILL);

	// Dimensions of window:
	// Subtitle
	subTitleWindow = subwin(stdscr, 1, box_width, box_start_l - 1, box_start_c);
	wattron(subTitleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(subTitleWindow, " ");

	// Shadow
	/*boxShadowWindow = subwin(stdscr, box_height+1, (int) COLS/2, (int) (LINES/2) - (int) (box_height/2), (int) (COLS/4) +1);
	wattron(boxShadowWindow, COLOR_PAIR(8));
	fillwin2(boxShadowWindow, " ");*/

	// BoxWindow
	boxWindow = subwin(stdscr, box_height, box_width, box_start_l, box_start_c);
	wattron(boxWindow, COLOR_PAIR(10));
	fillwin2(boxWindow, " ");
	box(boxWindow, ACS_VLINE, ACS_HLINE);

	// Text
	textWindow = subwin(stdscr, box_height - 2, box_width-4, box_start_l+1, box_start_c+2);
	wbkgd(textWindow, COLOR_PAIR(10));
	wmove(textWindow, 0,0);
	for (unsigned int i=0; i<adjustStringWide2(text, box_width-4).size() && i<box_height-2; i++) {
		wmove(textWindow, i, 0);
		wprintw(textWindow, adjustStringWide2(text, box_width-4)[i].c_str());
	}

	// Drawing titles
	drawTitles();
	// Refreshing
	wrefresh(mainWindow);
	wrefresh(titleWindow);
	wrefresh(subTitleWindow);
	//wrefresh(boxShadowWindow);
	wrefresh(boxWindow);
	wrefresh(textWindow);
//	getch();
}
void CursesInterface::showMsgBox(string text)
{
	if (!dialogMode) return;
	init();
	
	lastUsedInterface = LASTUI_MSGBOX;	
	

	// Sizes constants
	unsigned int box_width = COLS/2;
	unsigned int box_height = 4 + adjustStringWide2(text, box_width-4).size();	
	if (box_height < 6) box_height = 6;
	if ((int) box_height > LINES-3) box_height = LINES-3;
	unsigned int box_start_l = (int) (LINES - box_height)/2+1;
	if (box_start_l < 2) box_start_l = 2;
	unsigned int box_start_c = (unsigned int) (COLS - box_width)/2;
	// Title window
	titleWindow = subwin(stdscr, 1, COLS, 0, 0);
	wattron(titleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(titleWindow, " ");
	
	// Main background
	mainWindow = subwin(stdscr, LINES-1, COLS, 1, 0);
	wattron(mainWindow, COLOR_PAIR(10));
	fillwin2(mainWindow, BGFILL);

	// Dimensions of window:
	// Subtitle
	subTitleWindow = subwin(stdscr, 1, box_width, box_start_l - 1, box_start_c);
	wattron(subTitleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(subTitleWindow, " ");

	// Shadow
	/*boxShadowWindow = subwin(stdscr, box_height+1, (int) COLS/2, (int) (LINES/2) - (int) (box_height/2), (int) (COLS/4) +1);
	wattron(boxShadowWindow, COLOR_PAIR(8));
	fillwin2(boxShadowWindow, " ");*/

	// BoxWindow
	boxWindow = subwin(stdscr, box_height, box_width, box_start_l, box_start_c);
	wattron(boxWindow, COLOR_PAIR(10));
	fillwin2(boxWindow, " ");
	box(boxWindow, ACS_VLINE, ACS_HLINE);

	// Text
	textWindow = subwin(stdscr, box_height - 4, box_width-4, box_start_l+1, box_start_c+2);
	wbkgd(textWindow, COLOR_PAIR(10));
	wmove(textWindow, 0,0);
	for (unsigned int i=0; i<adjustStringWide2(text, box_width-4).size() && i<box_height-4; i++) {
		wmove(textWindow, i, 0);
		wprintw(textWindow, adjustStringWide2(text, box_width-4)[i].c_str());
	}

	// Button
	button1 = subwin(stdscr, 1, 14, box_start_l+box_height-2, box_start_c + box_width/2 - 7);
	wbkgd(button1, A_BOLD | COLOR_PAIR(1));
	wmove(button1, 0, (14-utf8strlen("OK"))/2);
	waddstr(button1, "OK");

	// Drawing titles
	drawTitles();
	// Refreshing
	wrefresh(mainWindow);
	wrefresh(titleWindow);
	wrefresh(subTitleWindow);
	//wrefresh(boxShadowWindow);
	wrefresh(boxWindow);
	wrefresh(textWindow);
	wrefresh(button1);
	int kbd_input=0;
	while (kbd_input!=ENTER && kbd_input != ESCAPE) kbd_input = getch();
}

bool CursesInterface::showYesNo(string text, string yes_button, string no_button) {
	if (!dialogMode) {
		return false;
	}
	init();
	lastUsedInterface = LASTUI_YESNO;
	currentButton = 0;

	unsigned int box_width = utf8strlen(subtitle)+4;
	for (unsigned int i=0; i<adjustStringWide2(text, COLS/2).size(); i++) {
		if (box_width<utf8strlen(adjustStringWide2(text, COLS/2)[i])) box_width = utf8strlen(adjustStringWide2(text, COLS/2)[i]);
	}
	if ((int) box_width<COLS/2) box_width = COLS/2;

	int box_height = 4 + adjustStringWide2(text, box_width-4).size(); // 2 for borders, 1 for space line, 1 for buttons
	int kbd_input;
	int box_start_l = (int) (LINES/2) - (int) (box_height/2);
	int box_start_c = COLS/2 - (int) box_width/2;

	// Title window
	titleWindow = subwin(stdscr, 1, COLS, 0, 0);
	wattron(titleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(titleWindow, " ");
	
	// Main background
	mainWindow = subwin(stdscr, LINES-1, COLS, 1, 0);
	wattron(mainWindow, COLOR_PAIR(10));
	fillwin2(mainWindow, BGFILL);

	// Subtitle
	subTitleWindow = subwin(stdscr, 1, box_width, box_start_l - 1, box_start_c);
	wattron(subTitleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(subTitleWindow, " ");

	// Shadow
	/*boxShadowWindow = subwin(stdscr, box_height+1, (int) COLS/2, (int) (LINES/2) - (int) (box_height/2), (int) (COLS/4) +1);
	wattron(boxShadowWindow, COLOR_PAIR(8));
	fillwin2(boxShadowWindow, " ");*/

	// BoxWindow
	boxWindow = subwin(stdscr, box_height, box_width, box_start_l, box_start_c );
	wattron(boxWindow, COLOR_PAIR(10));
	fillwin2(boxWindow, " ");
	box(boxWindow, ACS_VLINE, ACS_HLINE);


	// Text
	textWindow = subwin(stdscr, box_height-4, box_width-4, box_start_l+1, box_start_c+2);
	//wattron(textWindow, COLOR_PAIR(8));
	wbkgd(textWindow, COLOR_PAIR(10));
	wmove(textWindow, 0,0);
	for (unsigned int i=0; i<adjustStringWide2(text, box_width-4).size() && i < (unsigned int) box_height - 4; i++) {
		wmove(textWindow, i,0);
		wprintw(textWindow, adjustStringWide2(text, box_width-4)[i].c_str());
	}
	
	// Buttons
	button1 = subwin(stdscr, 1, 14, box_start_l+box_height-2, box_start_c + (box_width)/2 - 16);
	button2 = subwin(stdscr, 1, 14, box_start_l+box_height-2, box_start_c + (box_width)/2 + 2);
	wmove(button1, 0, (14-utf8strlen(yes_button))/2);
	waddstr(button1, yes_button.c_str());
	wmove(button2, 0, (14-utf8strlen(no_button))/2);
	waddstr(button2, no_button.c_str());
	
	// Drawing titles
	drawTitles();
	
refreshWindows:
	if (currentButton == 0) {
		wbkgd(button1, A_BOLD | COLOR_PAIR (1));
		wbkgd(button2, A_BOLD | COLOR_PAIR (2));
	}
	else {
		wbkgd(button2, A_BOLD | COLOR_PAIR (1));
		wbkgd(button1, A_BOLD | COLOR_PAIR (2));

	}
	// Refreshing
	wrefresh(mainWindow);
	wrefresh(titleWindow);
	wrefresh(subTitleWindow);
	//wrefresh(boxShadowWindow);
	wrefresh(boxWindow);
	wrefresh(textWindow);
	wrefresh(button1);
	wrefresh(button2);
	
	kbd_input = getch();
	touchwin(stdscr);
	if (kbd_input == TAB || kbd_input == KEY_BTAB || kbd_input == 261 || kbd_input == 260) {
		if (currentButton == 0) {
			currentButton = 1;
			goto refreshWindows;
		} else {
			currentButton = 0;
			goto refreshWindows;
		}
	}
	if (kbd_input==10) {
		if (currentButton == 0) return true;
		else return false;
	}
	if (kbd_input == ESCAPE) {
		return false;
	}
	goto refreshWindows;
}
void CursesInterface::increaseProgress() {
	setProgress(currentProgress + 1);
}
void CursesInterface::setProgress(int progress) {
	if (!dialogMode) return;
	init();
	if (lastUsedInterface != LASTUI_PROGRESS) {
		showProgressBar(headText, windowText, progress, progressMax);
		return;
	}
	int box_height = 9;
	int box_width = COLS/2;
	if (box_width < 60) box_width = 60;
	if (box_width > COLS-4) box_width = COLS-4;
	int box_start_l = (int) (LINES-box_height)/2;
	int box_start_c = (int) (COLS-box_width)/2;


	currentProgress = progress;

//-----------
	progressLineWindow = subwin(stdscr, 1, box_width - 8, box_start_l + 5, box_start_c+4);
	wbkgd(progressLineWindow, COLOR_PAIR(10));

	if (progressMax == 0) progressMax = 1;
	fillwin2(progressLineWindow, " ");
	wmove(progressLineWindow, 0,0);
	int physProgressSize = box_width - 8;
	int physProgress = (int) ((double) progress * ((double) physProgressSize / (double) progressMax));
	wattron(progressLineWindow, A_BOLD | COLOR_PAIR(11));
	for (int i=0; i<physProgress; i++) {
		waddstr(progressLineWindow, " ");
	}
	wattron(progressLineWindow, COLOR_PAIR(10));
	wrefresh(progressLineWindow);
	drawTitles();
	wrefresh(titleWindow);
	wrefresh(subTitleWindow);


}
void CursesInterface::setProgressMax(int progress_max) {
	if (!dialogMode) return;
	init();
	progressMax = progress_max;
	showProgressBar(headText, windowText, currentProgress, progressMax);
}
void CursesInterface::setProgressHeadText(string text) {
	if (!dialogMode) return;
	init();
	headText = text;
	showProgressBar(headText, windowText, currentProgress, progressMax);
}

void CursesInterface::setProgressText(string text) {
	if (!dialogMode) return;
	init();
	if (lastUsedInterface != LASTUI_PROGRESS) {
		showProgressBar(headText, text, currentProgress, progressMax);
		return;
	}
	windowText = text;
	int box_height = 9;
	int box_width = COLS/2;
	if (box_width < 60) box_width = 60;
	if (box_width > COLS-4) box_width = COLS-4;
	int box_start_l = (int) (LINES-box_height)/2;
	int box_start_c = (int) (COLS-box_width)/2;



	// Text
	textWindow = subwin(stdscr, 3, box_width-4, box_start_l+1, box_start_c+2);
	fillwin2(textWindow, " ");
	wbkgd(textWindow, COLOR_PAIR(10));
	wmove(textWindow, 0,0);
	windowText = text;
	//headText = header_text;
	string txt = headText + text;
	vector<string> data = adjustStringWide2(txt, box_width-4);
	for (unsigned int i=0; i<data.size(); i++) {
		wmove(textWindow, i,0);
		wprintw(textWindow, data[i].c_str());
	}
	wrefresh(boxWindow);
	wrefresh(textWindow);
	drawTitles();

	wrefresh(titleWindow);
	wrefresh(subTitleWindow);
}

void CursesInterface::showProgressBar(string header_text, string text, int progress, int progress_max)
{
	if (!dialogMode) return;
	init();
	lastUsedInterface = LASTUI_PROGRESS;
	// Sizes constants
	int box_height = 9;
	int box_width = COLS/2;
	if (box_width < 60) box_width = 60;
	if (box_width > COLS-4) box_width = COLS-4;
	int box_start_l = (int) (LINES-box_height)/2;
	int box_start_c = (int) (COLS-box_width)/2;

	// Title window
	titleWindow = subwin(stdscr, 1, COLS, 0, 0);
	wattron(titleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(titleWindow, " ");
	
	// Main background
	mainWindow = subwin(stdscr, LINES-1, COLS, 1, 0);
	wattron(mainWindow, COLOR_PAIR(10));
	fillwin2(mainWindow, BGFILL);

	// Subtitle
	subTitleWindow = subwin(stdscr, 1, box_width, box_start_l-1, box_start_c);
	wattron(subTitleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(subTitleWindow, " ");

	// BoxWindow
	boxWindow = subwin(stdscr, box_height, box_width, box_start_l, box_start_c);
	wattron(boxWindow, COLOR_PAIR(10));
	fillwin2(boxWindow, " ");
	box(boxWindow, ACS_VLINE, ACS_HLINE);

	// Text
	textWindow = subwin(stdscr, 3, box_width-4, box_start_l+1, box_start_c+2);
	wbkgd(textWindow, COLOR_PAIR(10));
	wmove(textWindow, 0,0);
	windowText = text;
	headText = header_text;
	string txt = header_text + text;
	vector<string> data = adjustStringWide2(txt, box_width-4);
	for (unsigned int i=0; i<data.size(); i++) {
		wmove(textWindow, i,0);
		wprintw(textWindow, data[i].c_str());
	}
	//waddstr(textWindow, txt.c_str());
	
	// Progress bar window
	progressWindow = subwin(stdscr, 3, box_width-6, box_start_l + 4, box_start_c+3);
	wbkgd(progressWindow, COLOR_PAIR(10));
	box(progressWindow, ACS_VLINE, ACS_HLINE);

	progressLineWindow = subwin(stdscr, 1, box_width-8, box_start_l + 5, box_start_c+4);
	wbkgd(progressLineWindow, COLOR_PAIR(10));


	progressMax = progress_max;
	if (progressMax == 0) progressMax = 1;
	fillwin2(progressLineWindow, " ");
	wmove(progressLineWindow, 0,0);
	int physProgressSize = box_width - 8;
	int physProgress = (int) ((double) progress * ((double) physProgressSize / (double) progressMax));
	wattron(progressLineWindow, A_BOLD | COLOR_PAIR(11));
	for (int i=0; i<physProgress; i++) {
		waddstr(progressLineWindow, " ");
	}
	wattron(progressLineWindow, COLOR_PAIR(10));

	// Drawing titles
	drawTitles();
	// Refreshing

	wrefresh(mainWindow);
	wrefresh(titleWindow);
	wrefresh(subTitleWindow);
	//wrefresh(boxShadowWindow);
	wrefresh(boxWindow);
	wrefresh(textWindow);
	wrefresh(progressWindow);
	wrefresh(progressLineWindow);
	refresh();

}
string CursesInterface::showInputBox(string text, string default_value) {
	if (!dialogMode) return "";
	init();
	string ret = default_value;
	if (showInputBox(text, &ret)) return ret;
	else {
		ret.clear();
		return ret;
	}
}
bool CursesInterface::showInputBox(string text, string * return_value, bool passwd_mode) { 

	if (!dialogMode) return false;
	init();

	lastUsedInterface = LASTUI_INPUT;
	currentButton = 0;
	string ret;
	WINDOW * cursorWindow;
	// Sizes constants
	
	int box_width = COLS/2;
	if (box_width < 60) box_width = 60;
	if (box_width > COLS-4) box_width = COLS-4;
	int box_height = 7+adjustStringWide2(text, box_width-4).size();
	int box_start_l = (int) (LINES-box_height)/2;
	int box_start_c = (int) (COLS-box_width)/2;


	// Title window
	titleWindow = subwin(stdscr, 1, COLS, 0, 0);
	wattron(titleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(titleWindow, " ");
	
	// Main background
	mainWindow = subwin(stdscr, LINES-1, COLS, 1, 0);
	wattron(mainWindow, COLOR_PAIR(10));
	fillwin2(mainWindow, BGFILL);

	// Subtitle
	subTitleWindow = subwin(stdscr, 1, box_width, box_start_l - 1, box_start_c);
	wattron(subTitleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(subTitleWindow, " ");

	// Shadow
	/*boxShadowWindow = subwin(stdscr, box_height+1, (int) COLS/2, (int) (LINES/2) - (int) (box_height/2), (int) (COLS/4) +1);
	wattron(boxShadowWindow, COLOR_PAIR(8));
	fillwin2(boxShadowWindow, " ");*/

	// BoxWindow
	boxWindow = subwin(stdscr, box_height, box_width, box_start_l, box_start_c);
	wattron(boxWindow, COLOR_PAIR(10));
	fillwin2(boxWindow, " ");

	// Text
	textWindow = subwin(stdscr, box_height-7, box_width-4, box_start_l+1, box_start_c+2);
	//wattron(textWindow, COLOR_PAIR(8));
	wbkgd(textWindow, COLOR_PAIR(10));
	wmove(textWindow, 0,0);
	windowText = text;
	vector<string> data = adjustStringWide2(text, box_width-4);
	for (unsigned int i=0; i<data.size(); i++) {
		wmove(textWindow, i, 0);
		wprintw(textWindow, data[i].c_str());
	}
	
	// Buttons
	button1 = subwin(stdscr, 1, 14, box_start_l+box_height - 2, box_start_c+4);
	button2 = subwin(stdscr, 1, 14, box_start_l+box_height - 2, box_start_c+4+18);
	wmove(button1, 0, (14 - utf8strlen(okStr))/2);
	waddstr(button1, okStr.c_str());
	wmove(button2, 0, (14-utf8strlen(cancelStr))/2);
	waddstr(button2, cancelStr.c_str());
	
	// Drawing titles
	drawTitles();
	


	// Progress bar window
	progressWindow = subwin(stdscr, 3, box_width-6, box_start_l + box_height-5, box_start_c+3);
	wbkgd(progressWindow, COLOR_PAIR(10));
	box(progressWindow, ACS_VLINE, ACS_HLINE);

	progressLineWindow = subwin(stdscr, 1, box_width - 8, box_start_l + box_height - 4, box_start_c+4);
	wbkgd(progressLineWindow, A_BOLD | COLOR_PAIR(9));

	// Cursor :)
	int cursor_position;
 
	wmove(progressLineWindow, 0,0);

	// Drawing titles
	drawTitles();
	
	int key;
	//echo();
	//curs_set(1);
	// Refreshing
	
	int counter = 0;
	// Add default value, if any
	if (!return_value->empty()) {
		counter = return_value->size();
		waddstr(progressLineWindow, return_value->c_str());
		ret = *return_value;
	}
input_refresh:
	drawTitles();
	if (currentButton == 0) {
		wbkgd(button1, A_BOLD | COLOR_PAIR (1));
		wbkgd(button2, A_BOLD | COLOR_PAIR (2));
	}
	else {
		wbkgd(button2, A_BOLD | COLOR_PAIR (1));
		wbkgd(button1, A_BOLD | COLOR_PAIR (2));

	}

	wrefresh(mainWindow);
	wrefresh(titleWindow);
	wrefresh(subTitleWindow);
	//wrefresh(boxShadowWindow);
	wrefresh(boxWindow);
	wrefresh(textWindow);
	wrefresh(progressWindow);
	wrefresh(progressLineWindow);
	wrefresh(button1);
	wrefresh(button2);
	cursor_position	= box_start_c + 4 + counter;

	cursorWindow = subwin(stdscr, 1,1, box_start_l + box_height - 4, cursor_position);
	wattron(cursorWindow, A_BLINK | COLOR_PAIR(1));
	fillwin2(cursorWindow, "_");

	wrefresh(cursorWindow);
	key = getch();
	while (key != KEY_BTAB && key!=TAB && key!= ENTER && key != ESCAPE) {
		if (key != KEY_LEFT && key != KEY_RIGHT && key != KEY_DOWN && key != KEY_UP && key != BACKSPACE && key != BACKSPACE2) {
			if (!passwd_mode) waddch(progressLineWindow, key);
			else waddch(progressLineWindow, '*');
			if (key<194) counter++; // if it isn't unicode special key - increase char counter
			ret+=key;
		}
		else {
			if (key == BACKSPACE || key == BACKSPACE2) {
				if (counter>0) {
					wmove(progressLineWindow,0,0);
					mvwdelch(progressLineWindow, 0, counter-1);
					counter--;
					ret.resize(ret.size()-1);
				}
			}
		}
		goto input_refresh;
	}
	if (key == KEY_BTAB || key == TAB) {
		if (currentButton == 0) {
			currentButton = 1;
			goto input_refresh;
		}
		if (currentButton == 1) {
			currentButton = 0;
			goto input_refresh;
		}
	}
	if (key == ENTER) {
		if (currentButton == 0) {
			*return_value = ret;
			return true;
		}
		else {
			return_value->clear();
			return false;
		}
	}
	if (key == ESCAPE) {
		return_value->clear();
		return false;
	}
	*return_value = "Ой!";
	return true;
}
string CursesInterface::showMenu2(string text, vector<MenuItem> menuItems, string def_select) {
	int def = 0;
	int ret;
	for (unsigned int i=0; i<menuItems.size(); i++) {
		if (menuItems[i].tag==def_select) {
			def = i;
			break;
		}
	}
	ret = showMenu(text, menuItems, def);
	if (ret==-1) return "";
	else return menuItems[ret].tag;
}

// ------------------------------- Mex388 --------------------------
/*        	WINDOW * mainWindow;
		WINDOW * titleWindow;
		WINDOW * subTitleWindow;
		WINDOW * boxWindow;
		WINDOW * textWindow;
		WINDOW * boxShadowWindow;
		WINDOW * button1;
		WINDOW * button2;
*/
void inversionMenu(WINDOW** items,int i/*1 - вврех -1 вниз */,int select /*тот который на данный момент уже выбран*/,int ColorSel,int ColorNSel)
{
	wbkgd(items[select],ColorNSel);
	wbkgd(items[select-i],ColorSel);
	
}
void chooseButton(WINDOW* button1,WINDOW* button2,bool choose, string str_ok, string str_cancel)
{
	switch(choose)
	{
		case false:
			{
				wbkgd(button1, A_BOLD | COLOR_PAIR(2)); //выделеная
				wbkgd(button2, A_BOLD | COLOR_PAIR(1));
				wmove(button1,0,(int) (14-utf8strlen(str_ok))/2);
				waddstr(button1,str_ok.c_str());
				wmove(button2,0,(int) (14-utf8strlen(str_cancel))/2);
				waddstr(button2,str_cancel.c_str());

				break;
			}
		case true:
			{
				wbkgd(button1, A_BOLD | COLOR_PAIR(1)); 
				wbkgd(button2, A_BOLD | COLOR_PAIR(2));// выделенная
				wmove(button1,0,(int) (14-utf8strlen(str_ok))/2);
				waddstr(button1,str_ok.c_str());
				wmove(button2,0,(int) (14-utf8strlen(str_cancel))/2);
				waddstr(button2,str_cancel.c_str());
		
				break;
			}
	}
	wrefresh(button1);
	wrefresh(button2);

}

int CursesInterface::showMenu(string str,vector<MenuItem> menulist,int def_select)
{
	if (!dialogMode) return -1;
	init();
	unsigned int box_width,box_height;
	int box_start_l,box_start_c;
	int count_items=menulist.size(); // Количество пунктов меню
	int select=def_select; // Выбранный пункт меню
	if (def_select>=(int) menulist.size()) select = 0;
	if (def_select < 0) select = 0;
	bool choose=true; // Выбранная кнопка, по умолчанию - ДА
	int key=0; // Буфер клавиатуры
	int maxword=0; // Максимальная ширина строки тега меню
	int menu_shift = 0;

	unsigned int max_tag_size = 0; // Максимум размера тега
	unsigned int max_menu_size = 0; // Максимум размера меню-строки
	for (int i=0; i<count_items; i++) { // 
		if (max_tag_size < utf8strlen(menulist[i].tag)) max_tag_size = utf8strlen(menulist[i].tag);
	}
	for (int i=0; i<count_items; i++) {
		if (max_menu_size < utf8strlen(menulist[i].value)) max_menu_size = utf8strlen(menulist[i].value);
	}
	box_width = max_tag_size + max_menu_size + 15;
	if (box_width<utf8strlen(subtitle)+4) box_width = utf8strlen(subtitle)+4;
	if (box_width < 55) box_width = 55;
	if ((int) box_width > COLS-4) box_width = COLS-4;
	box_height = count_items + 7 + adjustStringWide2(str, box_width-8).size()+3;
	if ((int) box_height > LINES-4) box_height = LINES-4;



	maxword = max_tag_size;


	
	box_start_c  = (int)(COLS-box_width) / 2;        //(int) ((double)cols*((double)1/(double)5));
	box_start_l  = (int)(LINES-box_height) / 2+1;      //(int) (((double)lines) * ((double)1/(double)4));
	if (box_start_l <3) box_start_l = 3;
	int max_help_lines=0;
	for (unsigned int i=0; i<menulist.size(); i++) {
		if (!menulist[i].description.empty() && (int) adjustStringWide2(menulist[i].description, box_width-4).size()>max_help_lines) max_help_lines = adjustStringWide2(menulist[i].description, box_width-4).size();
	}
	if (max_help_lines !=0) max_help_lines += 2;

	int max_visible_items = box_height - adjustStringWide2(str, box_width-8).size()-7;
	if (max_visible_items > count_items) max_visible_items = count_items;
	init_pair(20,COLOR_BLACK,COLOR_WHITE); // no select
	init_pair(21,COLOR_BLACK,COLOR_GREEN); // select

	titleWindow = subwin(stdscr, 1, COLS, 0, 0);
	wattron(titleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(titleWindow, " ");

	mainWindow = subwin(stdscr, LINES-1, COLS, 1, 0);
	wattron(mainWindow, COLOR_PAIR(10));
	fillwin2(mainWindow, BGFILL);

	subTitleWindow = subwin(stdscr, 1, box_width, box_start_l-1, box_start_c);
	wattron(subTitleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(subTitleWindow, " ");

	boxWindow  = subwin   (stdscr,box_height,box_width,box_start_l,box_start_c);
	wbkgd(boxWindow, COLOR_PAIR(10));
	fillwin2(boxWindow, " ");
	box (boxWindow,ACS_VLINE,ACS_HLINE);
	int text_height = adjustStringWide2(str, box_width-8).size();
	textWindow = subwin (stdscr, text_height, box_width-4, box_start_l+1, box_start_c+2);
	wbkgd(textWindow, COLOR_PAIR(10));
	wmove(textWindow, 0,0);
	vector<string> textMatrix = adjustStringWide2(str, box_width-8);
	for (unsigned int i=0; i<textMatrix.size(); i++) {
		wmove(textWindow, i,0);
		wprintw(textWindow, textMatrix[i].c_str());
	}

	if (max_help_lines>0) {
		progressWindow = subwin(stdscr, max_help_lines, box_width, box_start_l+box_height, box_start_c);
		wbkgd(progressWindow, COLOR_PAIR(10));
		box(progressWindow, ACS_VLINE, ACS_HLINE);
		wmove(progressWindow, 0,box_width-utf8strlen(_("Description"))/2);
		waddstr(progressWindow, _("Description"));
	}


	button1    = subwin   (stdscr,1,14,box_start_l+box_height-3,box_start_c+5);
     	button2    = subwin   (stdscr,1,14,box_start_l+box_height-3,box_start_c+box_width-20);	
	drawTitles();
	wrefresh(mainWindow);
	wrefresh(titleWindow);
	wrefresh(subTitleWindow);
	wrefresh(boxWindow);
	wrefresh(textWindow);
	chooseButton(button1,button2,choose, okStr.c_str(), cancelStr.c_str());
	wrefresh(button1);
	wrefresh(button2);
	if (max_help_lines > 0) wrefresh(progressWindow);


        items=new WINDOW*[count_items+1];//выделяем память для мелких менюшек

	//отображаем меню:
	for(int i=0;i<count_items && i<max_visible_items;i++)
	{
		items[i]=subwin(stdscr,1,box_width-2,box_start_l + text_height+2+i,box_start_c+1);
	}
	for(int i=0;i<count_items && i<max_visible_items;i++)
	{
		wbkgd(items[i],A_DIM | (A_UNDERLINE | COLOR_PAIR(20)));
		wmove(items[i],0,1);
		waddstr(items[i],menulist[i].tag.c_str());
		wrefresh(items[i]);
	}
	
	if (select - menu_shift >= max_visible_items) {
		while (select - menu_shift >= max_visible_items) {
			menu_shift += 10;// select - max_visible_items+2;
		}
		if (max_visible_items > count_items-menu_shift) menu_shift = count_items - max_visible_items;
		for (unsigned int i=0; (int) i<count_items && (int) i<max_visible_items; i++) {
			fillwin2(items[i], " ");
		}
	}	
	int prev_menu_shift = menu_shift;
	do{
		if (prev_menu_shift != menu_shift) {
			for (int i=0; i<count_items && i<max_visible_items; i++) {
				fillwin2(items[i], " ");
			}
		}
		prev_menu_shift = menu_shift;
		for(int i=0;i<count_items && i<max_visible_items;i++)
		{
			if (i+menu_shift!=select)
			{
				wbkgd(items[i],A_DIM | (COLOR_PAIR(20)));
				wmove(items[i],0,1);
				waddstr(items[i],menulist[i+menu_shift].tag.c_str());
				wmove(items[i],0, maxword+7);
				waddstr(items[i],truncateString(menulist[i+menu_shift].value, box_width-11-maxword).c_str());
				wrefresh(items[i]);
			}
			else
			{

				wbkgd(items[i],A_DIM | (COLOR_PAIR(21)));
				wmove(items[i],0,1);
				waddstr(items[i],menulist[i+menu_shift].tag.c_str());
				wmove(items[i],0, maxword+7);
				waddstr(items[i],truncateString(menulist[i+menu_shift].value, box_width-11-maxword).c_str());
				wrefresh(items[i]);
				if (max_help_lines>0) {
					fillwin2(progressWindow, " ");
					for (unsigned int h=0; h<adjustStringWide2(menulist[i+menu_shift].description, box_width-4).size(); h++) {
						wmove(progressWindow, h+1,2);
						wprintw(progressWindow,adjustStringWide2(menulist[i+menu_shift].description, box_width-4)[h].c_str());
					}
					box(progressWindow, ACS_VLINE, ACS_HLINE);
					wrefresh(progressWindow);
				}

			}

		}
		refresh();
		key=getch();

		switch(key)
		{
			case KEY_NPAGE:
			{
			if (select == count_items-1) break;
			select+=10;
			if (select > count_items) select = count_items-1;
			if (select - menu_shift >= max_visible_items) menu_shift+=10;
			if (menu_shift+max_visible_items>count_items) menu_shift=count_items - max_visible_items;
			break;
			}
			
			case KEY_PPAGE:
			{
			if (select == 0) break;
			
			if (select - menu_shift == 0) menu_shift-= 10;
			select-=10;
			if (select < 0) {
				select = 0;
				menu_shift = 0;
			}

			}
			break;
			case KEY_DOWN:
			{
			if (select == count_items-1) break;
			if (select > menu_shift + max_visible_items-2) menu_shift++;
			select++;
			break;
			}
			case KEY_UP:
			{
			if (select == 0) break;
			if (select - menu_shift == 0) menu_shift--;
			select--;
			break;
			}
			case ENTER:
			{
				if(choose==true) return select;
				if(choose==false)return -1;
				break;
			}
			case TAB:
			{
				choose = (choose==false ? true : false);
				chooseButton(button1,button2,choose, okStr.c_str(), cancelStr.c_str());
				
				break;
			}
		}

	}while(key!=ESCAPE);

return -1;
}

bool CursesInterface::showText(string Text,string str_ok,string str_cancel)
{
	if (!dialogMode) return false;
	init();
	int lines = LINES;
	int cols  = COLS;

	int box_height = lines-10;
	int box_width  = cols -10;

	int stlines = box_height-6;
	int stcols  = box_width -4;

	vector<string> TextMatrix = adjustStringWide2(Text, stcols-5);
	int box_start_c = (int)((double)(lines-box_height)*0.5);
	int box_start_l = (int)((double)(cols-box_width)*0.5);

	int key=0;
	bool choose=true;


	int upt =0;
	int downt=stlines;
	titleWindow = subwin(stdscr, 1, COLS, 0, 0);
	wattron(titleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(titleWindow, " ");
	
	// Main background
	mainWindow = subwin(stdscr, LINES-1, COLS, 1, 0);
	wattron(mainWindow, COLOR_PAIR(10));
	fillwin2(mainWindow, BGFILL);

	// Subtitle
	subTitleWindow = subwin(stdscr, 1, box_width, box_start_l-1, box_start_c);
	wattron(subTitleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(subTitleWindow, " ");

	boxWindow  = subwin (stdscr,box_height,box_width,box_start_l,box_start_c);
	wattron(boxWindow, COLOR_PAIR(10));
	fillwin2(boxWindow, " ");
	textWindow = subwin (stdscr,stlines, stcols, box_start_l+1, box_start_c+3);
	wbkgd(textWindow, A_DIM | COLOR_PAIR(10));
	fillwin2(textWindow, " ");

	box  (boxWindow,ACS_VLINE,ACS_HLINE);

	button1    = subwin   (stdscr,1,14,box_start_l+box_height-3,box_start_c+5);
     	button2    = subwin   (stdscr,1,14,box_start_l+box_height-3,box_start_c+box_width-20);	

	drawTitles();
	wrefresh(mainWindow);
	wrefresh(boxWindow);
	wrefresh(textWindow);

	
	chooseButton(button1,button2,choose, str_ok, str_cancel);
	wmove(textWindow, 0,0);
	for(unsigned int i=0; (int) i< stlines && i<TextMatrix.size();++i)
	{
		wmove(textWindow, i,0);
		wprintw(textWindow,TextMatrix[i].c_str());
	}
	refresh();
	wrefresh(mainWindow);
	wrefresh(boxWindow);
	wrefresh(textWindow);
	wrefresh(subTitleWindow);
	wrefresh(titleWindow);
        do {
		refresh();
		key=getch();
		int r=downt-upt;
		switch(key)
		{
			case KEY_DOWN:
			{
				if(downt>(int) TextMatrix.size()) break;
			      	upt++;
				downt++;
				fillwin2(textWindow," ");
				wmove   (textWindow,0,0);
				for(int i=upt;i<downt;i++)
				{
					waddstr(textWindow,TextMatrix[i].c_str());
					waddch (textWindow,'\n');
				}
				wrefresh(textWindow);
	
						break;
			}
			case KEY_UP:
			{
				if(upt==0)break;
				fillwin2(textWindow," ");
				wmove   (textWindow,0,0);
				upt--;
				downt--;
				for(int i=upt;i<downt;i++)
				{
					waddstr(textWindow,TextMatrix[i].c_str());
					waddch (textWindow,'\n');
				}
				
                               	wrefresh(textWindow);

				break;
			}
			case 339: //PAGE_UP:
			{
				
				for(int j=0;j<r;j++)
				{
			//		if(down)

					if(upt==0)break;
					fillwin2(textWindow," ");
					wmove   (textWindow,0,0);
					upt--;
					downt--;
					for(int i=upt;i<downt;i++)
					{
					waddstr(textWindow,TextMatrix[i].c_str());
					waddch (textWindow,'\n');
					}
				
					wrefresh(textWindow);
				}
	//			wrefresh(textWindow);
				break;
			}
			case 338: //PAGE_DOWN
			{
				
				for(int j=0;j<r;j++)
				{
					
			//		if(down)
				if(downt>(int)TextMatrix.size())break;
				upt++;
				downt++;
				fillwin2(textWindow," ");
				wmove   (textWindow,0,0);
				for(int i=upt;i<downt;i++)
				{
					waddstr(textWindow,TextMatrix[i].c_str());
					waddch (textWindow,'\n');
				}
				wrefresh(textWindow);
				}
	//			wrefresh(textWindow);
				break;
			}
			case ENTER:
			{
				if(choose==true) return true;
				if(choose==false)return false;
				break;
			}
			case TAB:
			{
				choose = (choose==false ? true : false);
				chooseButton(button1,button2,choose, str_ok, str_cancel);
				
				break;
			}
		}

	} while(key!=ESCAPE);


	

	return choose;

}
int CursesInterface::showExMenu(string text, vector<MenuItem> &menuItems,int def_select, int * key_pressed, bool (* callback) (int, int, vector<bool> *)) {
	if (!dialogMode) return 0;
	init();
	int count_items=menuItems.size();

	if(def_select>count_items) def_select=0;
	bool choose;
	int select_items=0;
	int lines, cols, box_height, box_width, stlines, stcols, key, box_start_c, box_start_l,select;
	unsigned int upt,downt;

        string str_yz = "[*]";//если выбран
	string str_nz = "[ ]";//если не выбран

	choose = true; // выборан "OK" || "CANCEL"
	
	vector<bool> ZZ;
	ZZ.resize(count_items);
	for(int i=0;i<count_items;i++)
		ZZ[i]=menuItems[i].flag;

	lines =LINES-1;//main window
	cols  =COLS;///main window

	box_height=30; //sub window
	int len_maxword=55;
	int t;	
	int ots;
	int entos = 5;

	for(int p=0;p<count_items;p++)
	{
		t   = 15 + utf8strlen(menuItems[p].tag)+utf8strlen(menuItems[p].value);
		ots =      utf8strlen(menuItems[p].tag)+5;
		if(len_maxword<t) len_maxword=t;
		if(entos<ots)     entos=ots;
	}
	if(entos!=5)entos++;
	box_width =len_maxword; //sub window
	if (box_width>cols) box_width = cols;

	if(count_items  >  lines-8)
	{
		box_height = lines-8;
	}
	else
	{
		box_height =  count_items+6+adjustStringWide2(text, box_width-8).size()+1;

	}

	stlines=box_height-5-adjustStringWide2(text, box_width-8).size();//окно вывода
	stcols=box_width -5; //окно вывода

	key=0;          //для клавишь в main while();
	select=def_select;//номер выбранного элемента по-умолчанию
       
	box_start_c  = (int)(((double)cols-(double)box_width)*0.5);//x-кт для sub window           //(int)((double)(cols-Len) / 2.0);        //(int) ((double)cols*((double)1/(double)5));
	box_start_l  = (int)(((double)lines-(double)box_height)*0.5);//y-кт для sub window 

	int stx_st  = box_start_c+1;
	int sty_st  = box_start_l+adjustStringWide2(text, box_width-8).size()+2;
	upt     = select;//верхний элемент (от 0 до N) для отображения
	downt   = select+stlines-2; //нижний элемент для отборажения

	init_pair(20,COLOR_BLACK,COLOR_WHITE); //color no select
	init_pair(21,COLOR_BLACK,COLOR_GREEN); //color select


	/* начальное отображение пустых окон*/
	mainWindow     =    subwin (stdscr,lines,cols,1,0);
	titleWindow    =    subwin (stdscr,1, cols, 0, 0);
	boxWindow      =    subwin (stdscr,box_height,box_width,box_start_l,box_start_c);
	subTitleWindow =    subwin (stdscr,1,box_width,box_start_l-1,box_start_c); 
	textWindow     = subwin (stdscr,stlines, stcols, sty_st,stx_st);

	wattron(mainWindow, COLOR_PAIR(10));
	fillwin2(mainWindow, BGFILL);
	wattron(titleWindow,A_BOLD | COLOR_PAIR(11));
	fillwin2(titleWindow, " ");
	wattron(boxWindow, COLOR_PAIR(10));
	fillwin2(boxWindow, " ");
	wattron(subTitleWindow, A_BOLD | COLOR_PAIR(11));
	fillwin2(subTitleWindow, " ");
	wattron(textWindow, COLOR_PAIR(10));
	fillwin2(textWindow, " ");
	box  (boxWindow,ACS_VLINE,ACS_HLINE);

	button1    = subwin   (stdscr,1,14,box_start_l+box_height-3,box_start_c+5);
     	button2    = subwin   (stdscr,1,14,box_start_l+box_height-3,box_start_c+box_width-20);	
	drawTitles();
	wrefresh(mainWindow);
	wrefresh(titleWindow);
	wrefresh(boxWindow);
	wrefresh(subTitleWindow);
	wrefresh(textWindow);
	chooseButton(button1,button2,choose, okStr.c_str(), cancelStr.c_str());

	wmove(boxWindow,1,1);
	
	vector<string> textMatrix = adjustStringWide2(text, box_width-8);
	for (unsigned int i=0; i<textMatrix.size(); i++) {
		wmove(boxWindow, i+1, 2);
		waddstr(boxWindow,textMatrix[i].c_str());
	}
	wrefresh(boxWindow);

		/*конец начального отображения*/
	

//----------------------------------------------------------------------------/
	select_items=0;
        items = new WINDOW*[stlines]; //создаем физические итемы тока стока, ск. влезет
	for(int i=0;i<stlines;i++)    //выделяем память для окон
	{
		items[i]=subwin(stdscr,1,stcols, sty_st+i, stx_st);//заносим сразу к-т
	}
	unsigned int y=0;
	for(unsigned int i=upt;i<downt;i++,y++)
	{
		if(i!=(unsigned int) select)
		{
			wbkgd(items[y],A_DIM | (COLOR_PAIR(20)));
			wmove(items[y],0,1);
			if(ZZ[i]==true)
			{
				waddstr(items[y],str_yz.c_str());
				waddstr(items[y],"  ");
			}
			else
			{
			    	waddstr(items[y],str_nz.c_str());
				waddstr(items[y],"  ");
			}
			wmove(items[y],0,5);
			waddstr(items[y],menuItems[i].tag.c_str());
			wmove(items[y],0,entos);
                    	waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
			wrefresh(items[y]);
		}
		else
		{
			wbkgd(items[y],A_DIM | (COLOR_PAIR(21)));
			wmove(items[y],0,1);
                      	if(ZZ[i]==true)
			{
				waddstr(items[y],str_yz.c_str());
				waddstr(items[y],"  ");
			}
			else
			{
			    	waddstr(items[y],str_nz.c_str());
				waddstr(items[y],"  ");
			}
			wmove(items[i],0,5);
			waddstr(items[y],menuItems[i].tag.c_str());
			wmove(items[y],0,entos);
                    	waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
			wrefresh(items[y]);
		}
	}

	wrefresh(boxWindow);
	do{

		key = getch();
		switch(key)
		{
			case KEY_DOWN:
			{
				if(downt>menuItems.size())break;
				if((unsigned int) select==downt-1)
				{
					
					if(downt+1>menuItems.size())break;
					upt++;
					downt++;
					select++;
					fillwin2(textWindow," ");
					wmove   (textWindow,0,0);
					y=0; //переменная для текст бокса
					for(unsigned int i=upt;i<downt;i++,y++)
					{
						if(y!=(unsigned int) select_items)
						{
							wbkgd(items[y],A_DIM | (COLOR_PAIR(20)));
							wmove(items[y],0,1);
							if(ZZ[i]==true)
							{
								waddstr(items[y],str_yz.c_str());
								waddstr(items[y]," ");
							}
							else
							{
								waddstr(items[y],str_nz.c_str());
								waddstr(items[y]," ");
							}
								wmove(items[y],0,5);
								waddstr(items[y],menuItems[i].tag.c_str());
								wmove(items[y],0,entos);
								waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
								wrefresh(items[y]);
						}
						else
						{
							wbkgd(items[y],A_DIM | (COLOR_PAIR(21)));
							wmove(items[y],0,1);
							if(ZZ[i]==true)
							{
								waddstr(items[y],str_yz.c_str());
								waddstr(items[y]," ");
							}
							else
							{
								waddstr(items[y],str_nz.c_str());
								waddstr(items[y]," ");
							}
							wmove(items[y],0,5);
							waddstr(items[y],menuItems[i].tag.c_str());
							wmove(items[y],0,entos);
							waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
							wrefresh(items[y]);
						}
					}		
				wrefresh(textWindow);
				}//if(select==downt)
				else
				{
					if(select==count_items-1)break;
					inversionMenu(items,-1,select_items,A_DIM | COLOR_PAIR(21),COLOR_PAIR(20));
					wmove(items[select_items],0,1);
				        wmove(items[select_items+1],0,1);
					select++;
					select_items++;
                                   	if(ZZ[select]==true)
					{
						waddstr(items[select_items],str_yz.c_str());
						waddstr(items[select_items]," ");
					}
					else
					{
						waddstr(items[select_items],str_nz.c_str());
						waddstr(items[select_items]," ");
					}
					wrefresh(items[select_items]);
					wrefresh(items[select_items-1]);
					break;
					
				}
	
			break;
			}
			case KEY_UP:
			{
				if(select==0)break;
				if(select_items==0)
				{
					upt--;
					downt--;
					select--;
					fillwin2(textWindow," ");
					wmove   (textWindow,0,0);
					y=0; //переменная для текст бокса
					for(unsigned int i=upt;i<downt;i++,y++)
					{
						if(y!=(unsigned int) select_items)
						{
							wbkgd(items[y],A_DIM | (COLOR_PAIR(20)));
							wmove(items[y],0,1);
							if(ZZ[i]==true)
							{
								waddstr(items[y],str_yz.c_str());
								waddstr(items[y]," ");
							}
							else
							{
								waddstr(items[y],str_nz.c_str());
								waddstr(items[y]," ");
							}
							wmove(items[y],0,5);
							waddstr(items[y],menuItems[i].tag.c_str());
							wmove(items[y],0,entos);
							waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
							wrefresh(items[y]);
						}
						else
						{
							wbkgd(items[y],A_DIM | (COLOR_PAIR(21)));
							wmove(items[y],0,1);
							if(ZZ[i]==true)
							{
								waddstr(items[y],str_yz.c_str());
								waddstr(items[y]," ");
							}
							else
							{
								waddstr(items[y],str_nz.c_str());
								waddstr(items[y]," ");
							}
							wmove(items[y],0,5);
							waddstr(items[y],menuItems[i].tag.c_str());
							wmove(items[y],0,entos);
							waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
							wrefresh(items[y]);
						}
					}		
					wrefresh(textWindow);
				}       //if(select==downt)
				else
				{
				

					inversionMenu(items,1,select_items,A_DIM | COLOR_PAIR(21),COLOR_PAIR(20));

					wmove(items[select_items],0,1);
				        wmove(items[select_items-1],0,1);
					select--;
					select_items--;
                                   	if(ZZ[select]==true)
					{
						waddstr(items[select_items],str_yz.c_str());
						waddstr(items[select_items]," ");
					}
					else
					{
						waddstr(items[select_items],str_nz.c_str());
						waddstr(items[select_items]," ");


					}
					wrefresh(items[select_items]);
					wrefresh(items[select_items+1]);
					break;
									
				}
				break;
			}
                       case PAGE_DOWN:
			{
			for(int yy=0;yy<stlines-3;yy++){
				if(downt>menuItems.size())break;
				if((unsigned int) select==downt-1)
				{
					
					if(downt+1>menuItems.size())break;
					upt++;
					downt++;
					select++;
					fillwin2(textWindow," ");
					wmove   (textWindow,0,0);
					y=0; //переменная для текст бокса
					for(unsigned int i=upt;i<downt;i++,y++)
					{
						if(y!=(unsigned int) select_items)
						{
							wbkgd(items[y],A_DIM | (COLOR_PAIR(20)));
							wmove(items[y],0,1);
							if(ZZ[i]==true)
							{
								waddstr(items[y],str_yz.c_str());
								waddstr(items[y]," ");
							}
							else
							{
								waddstr(items[y],str_nz.c_str());
								waddstr(items[y]," ");
							}
								wmove(items[y],0,5);
								waddstr(items[y],menuItems[i].tag.c_str());
								wmove(items[y],0,entos);
								waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
						}
						else
						{
							wbkgd(items[y],A_DIM | (COLOR_PAIR(21)));
							wmove(items[y],0,1);
							if(ZZ[i]==true)
							{
								waddstr(items[y],str_yz.c_str());
								waddstr(items[y]," ");
							}
							else
							{
								waddstr(items[y],str_nz.c_str());
								waddstr(items[y]," ");
							}
							wmove(items[y],0,5);
							waddstr(items[y],menuItems[i].tag.c_str());
							wmove(items[y],0,entos);
							waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
						}
					}		
				}//if(select==downt)
				else
				{
					if(select==count_items-1)break;
					inversionMenu(items,-1,select_items,A_DIM | COLOR_PAIR(21),COLOR_PAIR(20));
					wmove(items[select_items],0,1);
				        wmove(items[select_items+1],0,1);
					select++;
					select_items++;
                                   	if(ZZ[select]==true)
					{
						waddstr(items[select_items],str_yz.c_str());
						waddstr(items[select_items]," ");

					}
					else
					{
						waddstr(items[select_items],str_nz.c_str());
						waddstr(items[select_items]," ");
					}
				}
			}
			for(int h=0;h<stlines;h++)
			{
				wrefresh(items[h]);
			}
			wrefresh(textWindow);
			break;
			}
			case PAGE_UP:
			{
				for(int yy=0;yy<stlines-3;yy++)
				{
				if(select==0) break;
				if(select_items==0)
				{
					upt--;
					downt--;
					select--;
					fillwin2(textWindow," ");
					wmove   (textWindow,0,0);
					y=0; //переменная для текст бокса
					for(unsigned int i=upt;i<downt;i++,y++)
					{
						if(y!=(unsigned int) select_items)
						{
							wbkgd(items[y],A_DIM | (COLOR_PAIR(20)));
							wmove(items[y],0,1);
							if(ZZ[i]==true)
							{
								waddstr(items[y],str_yz.c_str());
								waddstr(items[y]," ");
							}
							else
							{
								waddstr(items[y],str_nz.c_str());
								waddstr(items[y]," ");
							}
							wmove(items[y],0,5);
							waddstr(items[y],menuItems[i].tag.c_str());
							wmove(items[y],0,entos);
							waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
						}
						else
						{
							wbkgd(items[y],A_DIM | (COLOR_PAIR(21)));
							wmove(items[y],0,1);
							if(ZZ[i]==true)
							{
								waddstr(items[y],str_yz.c_str());
								waddstr(items[y]," ");
							}
							else
							{
								waddstr(items[y],str_nz.c_str());
								waddstr(items[y]," ");
							}
							wmove(items[y],0,5);
							waddstr(items[y],menuItems[i].tag.c_str());
							wmove(items[y],0,entos);
							waddstr(items[y],truncateString(menuItems[i].value, box_width-entos-5).c_str());
						}
					}		
				}       
				else
				{
				

					inversionMenu(items,1,select_items,A_DIM | COLOR_PAIR(21),COLOR_PAIR(20));

					wmove(items[select_items],0,1);
				        wmove(items[select_items-1],0,1);
					select--;
					select_items--;
                                   	if(ZZ[select]==true)
					{
						waddstr(items[select_items],str_yz.c_str());
						waddstr(items[select_items]," ");
					}
					else
					{
						waddstr(items[select_items],str_nz.c_str());
						waddstr(items[select_items]," ");
					}
				}
				}
				for(int h=0;h<stlines;h++)
				{
					wrefresh(items[h]);
				}
				wrefresh(textWindow);
				break;
			}

			case ENTER:
			{
				if (choose) {
					for (int i=0; i<count_items; i++) {
						menuItems[i].flag=ZZ[i];
					}
					return select;
				}
				else return -1;
				break;
			}
			case KEY_LEFT:
			case KEY_RIGHT:
			case TAB:
			{
				choose = (choose==false ? true : false);
				chooseButton(button1,button2,choose, "OK", "ОТМЕНА");
				
				break;
			}
			case SPACE:
			{
				if(ZZ[select])
				{
					wmove(items[select_items],0,1);
					waddstr(items[select_items],str_nz.c_str());
					wrefresh(items[select_items]);
					ZZ[select]=false;
				}
				else
				{
				       	wmove(items[select_items],0,1);
					waddstr(items[select_items],str_yz.c_str());
					wrefresh(items[select_items]);
					ZZ[select]=true;
				}
				break;
			}
			case INSERT:
			case DELETE:
				if (key_pressed == NULL) break;
				for(int i=0;i<count_items;i++) menuItems[i].flag=ZZ[i];
				*key_pressed = key;
				return select;
			default:
				break;
		}
		if (callback!=NULL) {
			if (!callback(key, select, &ZZ)) return -1;
		}

	wrefresh(mainWindow);
	wrefresh(boxWindow);
	wrefresh(textWindow);
	chooseButton(button1,button2,choose, "OK", "ОТМЕНА");


	}while(key!=ESCAPE);

	for(int i=0;i<count_items;i++)
		menuItems[i].flag=ZZ[i];



return 0;

}
string CursesInterface::ncGetOpenFile(string defaultDir) {
	defaultDir += "/";
	while (defaultDir.size()>1 && defaultDir[defaultDir.size()-1]=='/' && defaultDir[defaultDir.size()-2]=='/') defaultDir.resize(defaultDir.size()-1);
	vector<MenuItem> items;
	vector<string> dirList;
	vector<int> types;
	string currentDir = defaultDir;
	int selection = 0;
	while(selection!=-1) {
		dirList = getDirectoryList(currentDir, &types);
		if (dirList.size() != types.size()) showMsgBox("WTF!");
		items.clear();
		items.push_back(MenuItem("[..]","<DIR>"));
		for (unsigned int i=0; i<dirList.size(); ++i) {
			if (types[i]==TYPE_DIR) items.push_back(MenuItem("[" + dirList[i] + "]", "<DIR>"));
			else items.push_back(MenuItem(dirList[i], ""));
		}
		selection = showMenu("[ " + currentDir + " ]", items);
		if (selection==-1) return "";
		if (selection == 0) {
			if (currentDir == "/") continue;
			currentDir.resize(currentDir.size()-1);
			currentDir = currentDir.substr(0, currentDir.find_last_of("/")+1);
			
			continue;
		}
		--selection;
		if (types[selection]==TYPE_FILE) return currentDir + dirList[selection];
		currentDir=currentDir + dirList[selection] + "/";
	}
	return "";
}

string CursesInterface::ncGetOpenDir(string defaultDir) {
	defaultDir += "/";
	while (defaultDir.size()>1 && defaultDir[defaultDir.size()-1]=='/' && defaultDir[defaultDir.size()-2]=='/') defaultDir.resize(defaultDir.size()-1);
	vector<MenuItem> items;
	vector<string> dirList;
	vector<int> types;
	string currentDir = defaultDir;
	int selection = 0;
	while(selection!=-1) {
		dirList = getDirectoryList(currentDir, &types);
		if (dirList.size() != types.size()) showMsgBox("WTF!");
		items.clear();
		items.push_back(MenuItem("[.]","Выбрать эту директорию"));
		items.push_back(MenuItem("[..]","<DIR>"));
		for (unsigned int i=0; i<dirList.size(); ++i) {
			if (types[i]==TYPE_DIR) items.push_back(MenuItem("[" + dirList[i] + "]", "<DIR>"));
			//else items.push_back(MenuItem(dirList[i], ""));
		}
		selection = showMenu("[ " + currentDir + " ]", items);
		if (selection==-1) return "";
		if (selection == 1) {
			if (currentDir == "/") continue;
			currentDir.resize(currentDir.size()-1);
			currentDir = currentDir.substr(0, currentDir.find_last_of("/")+1);
			
			continue;
		}
		if (selection==0) return currentDir;
		--selection;
		--selection;
		currentDir=currentDir + dirList[selection] + "/";
	}
	return "";
}

