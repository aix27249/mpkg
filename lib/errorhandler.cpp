#include "errorhandler.h"
#include "errorcodes.h"
#include "terminal.h"
#include <nwidgets/ncurses_if.h>
#include "config.h"
#include "debug.h"
MpkgErrorReturn genericConsoleErrorHandler(ErrorDescription e, const string& errorDetails) {
	if (e.action.size()==1) { // Case of no choice
		if (e.action[0].ret==MPKG_RETURN_CONTINUE) {
			say("%s\n",e.text.c_str());
			if (!errorDetails.empty()) say(_("Details: %s\n"),errorDetails.c_str());
		}
		else {
			mError(e.text);
			if (!errorDetails.empty()) mError(_("Details: ") + errorDetails);
		}
		return e.action[0].ret;
	}
	else {
		mError(e.text);
		printf("%s\n", _("Choose an action:"));
		int ret=0;
		int invalidResponces = 0;
		for (size_t x=0; x<e.action.size(); x++) {
			cout << "\t[" << x+1 << "]  " << e.action[x].text << "\n";
		}
		while(invalidResponces<3) {
			cin>>ret;
			if (ret>0 && (size_t) ret<=e.action.size()) {
				// Return value OK, returning
				say(_("You decided to %s\n"), e.action[ret-1].text.c_str());
				return e.action[ret-1].ret;
			}
			say(_("Invalid input\n"));
		}
		return MPKG_RETURN_ABORT;
	}
}
MpkgErrorReturn genericNWidgetsErrorHandler(ErrorDescription e, const string& errorDetails) {
	string details;
	if (!errorDetails.empty()) details = "\nDetails: " + errorDetails;

	// Call dialog event resolver
	vector<MenuItem> menuItems;
	for (size_t i=0; i<e.action.size(); i++) {
		menuItems.push_back(MenuItem(IntToStr(i), e.action[i].text));
	}
	if (e.action.size()==1) {
		if (e.action[0].ret==MPKG_RETURN_CONTINUE) {
			ncInterface.setSubtitle(_("Information"));
			ncInterface.showInfoBox(e.text + details);
		}
		else {
			mError(e.text);
			ncInterface.showMsgBox(e.text + details);
		}
		return e.action[0].ret;
	}
	int ret = ncInterface.showMenu(e.text, menuItems);
	if (ret>=0 && (size_t) ret<e.action.size()) {
		return e.action[ret].ret;
	}
	else {
		mError("Aborted");
		return MPKG_RETURN_ABORT;
	}
}

// Global object that will handle errors
MPKGErrorHandler mpkgErrorHandler;

MPKGErrorHandler::MPKGErrorHandler() {
	if (!dialogMode) registerErrorHandler(genericConsoleErrorHandler);
	else registerErrorHandler(genericNWidgetsErrorHandler);
	initErrorManager();
}

MPKGErrorHandler::~MPKGErrorHandler() {
}

void MPKGErrorHandler::registerErrorHandler(MpkgErrorReturn (*newErrorHandler) (ErrorDescription error, const string& errorDetails)) {
	errorHandler = newErrorHandler;
}

void MPKGErrorHandler::unregisterErrorHandler() {
	errorHandler = NULL;
}

void MPKGErrorHandler::initErrorManager() {
	errorList.clear();
	ErrorDescription e;
	ErrorOptions a;
	vector<ErrorOptions> al;



	
	e.action.clear();
	e.code=MPKG_DOWNLOAD_OK;
	e.text=_("Download completed successfully");
	a.ret=MPKG_RETURN_CONTINUE;
	a.text="OK";
	e.action.push_back(a);
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_DOWNLOAD_TIMEOUT;
	e.text=_("Download error: timeout");
	a.ret=MPKG_RETURN_RETRY;
	a.text=_("Retry");
	al.push_back(a);
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort");
	al.push_back(a);
	a.ret=MPKG_RETURN_IGNORE;
	a.text=_("Skip/Ignore");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_DOWNLOAD_MD5;
	e.text=_("Download error: file downloaded, but MD5 doesn't match");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_DOWNLOAD_HOST_NOT_FOUND;
	e.text=_("Download error: host not found");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_DOWNLOAD_FILE_NOT_FOUND;
	e.text=_("Download error: file not found");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_DOWNLOAD_LOGIN_INCORRECT;
	e.text=_("Download error: login incorrect");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_DOWNLOAD_FORBIDDEN;
	e.text=_("Download error: forbidden");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_DOWNLOAD_OUT_OF_SPACE;
	e.text=_("Download error: out of space");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_DOWNLOAD_WRITE_ERROR;
	e.text=_("Download error: error writing output file");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_DOWNLOAD_ERROR;
	e.text=_("Download error: cannot download file");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INDEX_OK;
	e.text=_("Database updated successfully");
	a.ret=MPKG_RETURN_CONTINUE;
	a.text="OK";
	e.action.clear();
	e.action.push_back(a);
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INDEX_DOWNLOAD_TIMEOUT;
	e.text=_("Database update error: error downloading repository index: timeout");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INDEX_PARSE_ERROR;
	e.text=_("Database update error: invalid index received: parse error");
	al.clear();
	a.ret=MPKG_RETURN_SKIP;
	a.text=_("Skip");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INDEX_HOST_NOT_FOUND;
	e.text=_("Database update error: error downloading repository index: host not found");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INDEX_NOT_RECOGNIZED;
	e.text=_("Database update error: repository type not recognized");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INDEX_LOGIN_INCORRECT;
	e.text=_("Database update error: error downloading index: login incorrect");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INDEX_FORBIDDEN;
	e.text=_("Database update error: error downloading index: forbidden");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INDEX_ERROR;
	e.text=_("Database update error: unknown error while importing index");
	e.action=al;
	errorList.push_back(e);
	
	e.action.clear();
	al.clear();
	e.code=MPKG_INSTALL_OK;
	e.text=_("Installation completed successfully");
	e.action.clear();
	a.ret=MPKG_RETURN_CONTINUE;
	a.text="OK";
	e.action.push_back(a);
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INSTALL_OUT_OF_SPACE;
	e.text=_("Installation error: out of space");
	al.clear();
	a.ret=MPKG_RETURN_RETRY;
	a.text=_("Retry");
	al.push_back(a);
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INSTALL_SCRIPT_ERROR;
	e.text=_("Installation error: script execution error");
	al.clear();
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort");
	al.push_back(a);
	a.ret=MPKG_RETURN_SKIP;
	a.text=_("Ignore");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INSTALL_EXTRACT_ERROR;
	e.text=_("Installation error: package extraction failed");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INSTALL_META_ERROR;
	e.text=_("Installation error: incorrect package metadata");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INSTALL_FILE_CONFLICT;
	e.text=_("Installation error: unresolvable file conflicts");
	al.clear();
	a.ret=MPKG_RETURN_IGNORE;
	a.text=_("Ignore");
	al.push_back(a);
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_INSTALL_NOT_IN_DB;
	e.text=_("Installation error: package not found in database");
	al.clear();
	a.ret=MPKG_RETURN_SKIP;
	a.text=_("Skip");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_CDROM_MOUNT_ERROR;
	al.clear();
	a.ret=MPKG_RETURN_RETRY;
	a.text=_("Retry");
	al.push_back(a);
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort");
	al.push_back(a);
	e.text=_("Installation error: error mounting CD-ROM media");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_CDROM_WRONG_VOLNAME;
	e.text=_("Installation error: wrong CD-ROM media");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_SUBSYS_SQLDB_INCORRECT;
	e.text=_("Internal error: database structure is invalid");
	al.clear();
	a.ret=MPKG_RETURN_REINIT;
	a.text=_("Recreate structure (ALL DATA WILL BE LOST!)");
	al.push_back(a);
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort and exit");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_SUBSYS_SQLDB_OPEN_ERROR;
	e.text=_("Internal error: cannot open database file");
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort and exit");

	e.action.clear();
	al.clear();
	e.code=MPKG_SUBSYS_XMLCONFIG_READ_ERROR;
	e.text=_("Internal error: cannot read configuration file");
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_SUBSYS_XMLCONFIG_WRITE_ERROR;
	e.text=_("Internal error: cannot write configuration file");
	al.clear();
	a.ret=MPKG_RETURN_RETRY;
	a.text=_("Retry");
	al.push_back(a);
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort and exit");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_SUBSYS_SQLQUERY_ERROR;
	e.text=_("Internal error: SQL query failed");
	e.action.clear();
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort and exit");
	e.action.push_back(a);
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_SUBSYS_TMPFILE_CREATE_ERROR;
	e.text=_("Internal error: cannot create temporary file");
	al.clear();
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort and exit");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_SUBSYS_FILE_WRITE_ERROR;
	e.text=_("Internal error: error writing file");
	al.clear();
	a.ret=MPKG_RETURN_RETRY;
	a.text=_("Retry");
	al.push_back(a);
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Abort");
	al.push_back(a);
	e.action=al;
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_SUBSYS_FILE_READ_ERROR;
	e.text=_("Internal error: error reading file");
	e.action=al;
	errorList.push_back(e);
	
	e.action.clear();
	al.clear();
	e.code=MPKG_STARTUP_COMPONENT_NOT_FOUND;
	e.text=_("Startup error: some of required components not found");
	a.ret=MPKG_RETURN_ABORT;
	a.text=_("Exit");
	e.action.clear();
	e.action.push_back(a);
	errorList.push_back(e);

	e.action.clear();
	al.clear();
	e.code=MPKG_STARTUP_NOT_ROOT;
	e.text=_("Startup error: you must run this program as root");
	errorList.push_back(e);

}


MpkgErrorReturn MPKGErrorHandler::callError(MpkgErrorCode err, string errorDetails) {
	for (size_t i=0; i<errorList.size(); i++)	{
		if (errorList[i].code==err) {
			printf("called error code %d [%s]\n", err, errorDetails.c_str());
			return errorHandler(errorList[i], errorDetails);
		}
	}
	return MPKG_RETURN_ABORT; // Invalid error code - do abort

}
