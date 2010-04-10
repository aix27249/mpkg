#ifndef HTMLCORE_H_
#define HTMLCORE_H_
#include "config.h"

void printHtml(string, bool raw = false); //
void printHtmlError(string);
void printHtmlWarning(string);
void printHtmlProgress(bool newPage=true);
void printHtmlRedirect();
void newHtmlPage(); // 
void clearHtmlPage(); //
string redirectPage(string url, bool print_now=false);
string refreshPage(unsigned int timeout=4000, bool print_now=false);
#endif
