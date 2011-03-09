#ifndef TEXTSETUP_EVENTHANDLER_H__
#define TEXTSETUP_EVENTHANDLER_H__
#include <agiliasetup.h>

class TextEventHandler: public StatusNotifier {
	public:
		TextEventHandler();
		~TextEventHandler();

		void setDetailsTextCall(const string& msg);
		void setSummaryTextCall(const string& msg);
		void setProgressCall(int progress);
		void sendReportError(const string& text);

};

#endif
