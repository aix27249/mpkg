/* Debugging output function(s)
 $Id: debug.cpp,v 1.23 2007/11/04 14:15:08 i27249 Exp $
 */
 

#include "debug.h"
#include "htmlcore.h"
#define ENABLE_LOGGING
#define RELEASE
bool hideErrors = false;
std::string _mLog(const char* file, int line, const char *func, std::string msg) {
	FILE * fd = fopen("/var/log/mLog.log", "a");
	if (!fd) return msg;
	fprintf(fd, "[%s] %s: line %d: %s\n", func, file, line, msg.c_str());
	fclose(fd);
	return msg;
}
std::string _mError(const char* file, int line, const char *func, std::string message, bool warn)
{
	if (hideErrors) return message;
	FILE *tty;
	if (!setupMode) tty=stderr;
	else tty=fopen("/dev/tty4","w");
	if (!tty) 
	{
		printf("Failed to open tty4!\n");
		sleep(2);
		tty=stderr;
	}
	if (msayState) {
		fprintf(tty, "\n");
		msayState = false;
	}
	if (setupMode || !dialogMode) {
#ifndef RELEASE
		if (!warn) fprintf(tty, "%s[ERROR]: %sin %s  (%s:%i):%s %s\n",CL_RED, CL_YELLOW, func, file, line, CL_WHITE, message.c_str());
		else fprintf(tty, "%s[WARNING]: %sin %s  (%s:%i):%s %s\n",CL_YELLOW, CL_GREEN, func, file, line, CL_WHITE, message.c_str());
#else
		if (!warn) fprintf(tty, "%s%s:%s %s\n",CL_RED, _("Error"), CL_WHITE, message.c_str());
		else fprintf(tty, "%s%s:%s %s\n",CL_YELLOW, _("Warning"), CL_WHITE, message.c_str());
#endif
	}
#ifdef ENABLE_LOGGING
	string logfile = (string) log_directory + "mpkg-errors.log";
	FILE *log = fopen(logfile.c_str(), "a");
	if (log)
	{
		fprintf(log, "%s  (%s:%i): %s\n", func, file, line, message.c_str());
		fclose(log);
	}
#endif

	if (setupMode) fclose(tty);
	if (dialogMode) ncInterface.showMsgBox(message);
	if (htmlMode) {
		if (warn) printHtmlWarning(toHtml(message));
		else printHtmlError(toHtml(message));
	}
return message;
}

#ifdef DEBUG
void DbgPrint(const char* file, int line, const char *func, std::string message) {
	fprintf(stdout, "%s[DEBUG] %sin %s  (%s:%i):%s %s\n",CL_GREEN, CL_YELLOW, func, file, line, CL_WHITE, message.c_str());
#else

void DbgPrint(const char* , int , const char *, std::string) {
#endif

#ifndef RELEASE
  #ifdef ENABLE_LOGGING
	//printf("Log trying\n");
	string logfile;
       	logfile = (string) log_directory + "mpkg-debug.log";
	//printf("logfile: %s\n", logfile.c_str());
	//printf("log done\n");
	FILE *log = fopen(logfile.c_str(), "a");
	if (log)
	{
		fprintf(log, "%s  (%s:%i): %s\n",func, file, line, message.c_str());
		fclose(log);
	}
  #endif
#endif

}


std::string strim(std::string& s, const std::string& drop)
{
	std::string r=s.erase(s.find_last_not_of(drop)+1);
	return r.erase(0, r.find_first_not_of(drop)); 
}

