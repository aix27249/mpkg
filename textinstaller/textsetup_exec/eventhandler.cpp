#include "eventhandler.h"
TextEventHandler::TextEventHandler() {
}

TextEventHandler::~TextEventHandler() {
}

void TextEventHandler::setDetailsTextCall(const string& msg) {
	ncInterface.setProgressText(msg);
}
void TextEventHandler::setSummaryTextCall(const string& msg) {
	ncInterface.setSubtitle(msg);
}
void TextEventHandler::setProgressCall(int progress) {
	ncInterface.setProgress(progress);
}
void TextEventHandler::sendReportError(const string& text) {
	ncInterface.showMsgBox(text);
}

