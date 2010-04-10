#ifndef NCURSES_IF_H_
#define NCURSES_IF_H_
#include <ncursesw/ncurses.h>
#include <libintl.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#define _(string) gettext(string)
using namespace std;

class MenuItem {
	public:
		MenuItem(){}
		MenuItem(string a, string b, string c="", bool flag=false) {
			tag = a;
			value = b;
			description = c;
			this->flag=flag;
		}
		string tag;
		string value;
		string description;
		bool   flag;
};
enum LastUsedIF {
	LASTUI_NONE = 0,
	LASTUI_PROGRESS,
	LASTUI_MENU,
	LASTUI_YESNO,
	LASTUI_INPUT,
	LASTUI_INFOBOX,
	LASTUI_MSGBOX
};
class CursesInterface {
	public:
		// Constructor/destructor
		CursesInterface(bool _initLocale=true);
		~CursesInterface();

		// Global text functions
		void setTitle(string text);
		void setSubtitle(string text);
		void showInfoBox(string text);
		void showMsgBox(string text);
		bool showYesNo(string text, string yes_button=_("Yes"), string no_button = _("No"));
		// Progress bar functions
		// All of them immediately shows some progress bar. if not sure that text is set before, run showProgressBar
		void showProgressBar(string header_text, string text, int progress, int progress_max);
		void increaseProgress();
		void setProgress(int progress);
		void setProgressText(string text);
		void setProgressMax(int progress_max);
		void setProgressHeadText(string text);
		bool showInputBox(string text, string * return_value, bool passwd_mode=false);
		string showInputBox(string text, string default_value = "");

		// ------------ mex388 functions
		int showMenu(string text, vector<MenuItem> menuItems,int def_select=0);
		string showMenu2(string text, vector<MenuItem> menuItems, string def_select="");
		int showExMenu(string text, vector<MenuItem> &menuItems,int def_select=0, int *key_pressed=NULL, bool (*callback) (int, int, vector<bool> *) = NULL);
                bool showText(string text, string str_ok=_("Accept"), string str_cancel=_("Do not accept"));
		void uninit();

		void init();
		int lastUsedInterface;
		bool init_complete;
		string title, subtitle, windowText;
		string headText;
		string okStr, cancelStr;
		string _BGF;// "\342\226\222"

		bool initLocale;
		void setStrings();
	private:
		int progressMax;
		int currentProgress;

		void drawTitles();
		WINDOW * mainWindow;
		WINDOW * titleWindow;
		WINDOW * subTitleWindow;
		WINDOW * boxWindow;
		WINDOW * textWindow;
		WINDOW * boxShadowWindow;
		WINDOW * button1;
		WINDOW * button2;

		WINDOW * progressWindow;
		WINDOW * progressLineWindow;
	
		WINDOW** items;

		int currentButton;
};

#endif // NCURSES_IF_H_
