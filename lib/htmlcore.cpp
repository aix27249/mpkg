#include "htmlcore.h"

void newHtmlPage() {
	if (!htmlMode || serverMode) return;
	string data;
	string link;
	if (!htmlNextLink.empty()) link = "<a href=\"" + htmlNextLink + "\">$NEXTNAME</a><br><br>\n";
	data = "<h1>MPKG: система пакетов MPKG/h1>" + link + "\n";
	WriteFile(htmlPage, data);
}

void clearHtmlPage() {
	if (serverMode) return;
	unlink(htmlPage.c_str());
}

void printHtml(string data, bool raw) {
	if (!htmlMode || serverMode) return;
	if (raw) data = ReadFile(htmlPage) + data;
	else data = ReadFile(htmlPage) + "<p>" + data + "<br>\n";
	WriteFile(htmlPage, data);
}

void printHtmlError(string data) {
	if (!htmlMode || serverMode) return;
	data = "<b><font color=\"red\">" + string(_("Error: ")) + "</font></b>" + data;
	printHtml(data);
}

void printHtmlWarning(string data) {
	if (!htmlMode || serverMode) return;
	data = "<b><font color=\"#FFAD1E\">" + string(_("Warning: ")) + "</font></b>" + data;
	printHtml(data);
}

void printHtmlProgress(bool newPage) {
	if (!htmlMode || serverMode) return;
	
	unlink(htmlPage.c_str());
	string data;
	string link;
	if (!htmlNextLink.empty()) link = "<a href=\"" + htmlNextLink + "\">$NEXTNAME</a><br><br>\n";

	if (newPage) data = "<h1>MPKG: система пакетов MPKG</h1>" + link;
	data += "<h3>Прогресс выполнения задач: " + IntToStr((int) (pData.getTotalProgress()/pData.getTotalProgressMax())) + "%</h3>\n<table id=\"progress_table\">\n";
	int tmp_c;
	for (unsigned int i=0; i<pData.size(); ++i) {
		tmp_c =(int) ((double) 100 * (pData.getItemProgress(i)/pData.getItemProgressMaximum(i)));
		data += "<tr><td>" + pData.getItemName(i) + "</td><td>" + pData.getItemCurrentAction(i) + "</td><td>" + IntToStr(tmp_c) + "%</td></tr>\n";
	}
	data += "</table>\n";
	printHtml(data);
}

void printHtmlRedirect() {
	printHtml(redirectPage("$REDIRECT"), true);
}
string redirectPage(string url, bool print_now) {
	if (!htmlMode || serverMode) return "";
	string ret = "<script type=\"text/javascript\">\r\n<!--\r\nwindow.location = \"" + url + "\"\r\n//-->\r\n</script>\r\n";
	if (print_now) printf("%s", ret.c_str());
	return ret;
}

string refreshPage(unsigned int timeout, bool print_now) {
	if (!htmlMode || serverMode) return "";
	string ret = "<script type=\"text/JavaScript\">\r\n<!--\r\nfunction timedRefresh(timeoutPeriod) {\r\nsetTimeout(\"location.reload(true);\",timeoutPeriod);\r\n}\r\n// -->\r\n</script>\r\n<body onload=\"JavaScript:timedRefresh(" + IntToStr(timeout) + ");\">\r\n";
	if (print_now) printf("%s", ret.c_str());
	return ret;
	
}


