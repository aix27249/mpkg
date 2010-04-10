/*
 * MOPSLinux package system - error codes
 * $Id: errorcodes.h,v 1.5 2007/08/02 10:39:13 i27249 Exp $
 */

#ifndef ERRORCODES_H_
#define ERRORCODES_H_
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <map>
#include <nwidgets/ncurses_if.h>
//#include "file_routines.h"
//#include "ncurses_if.h"

//#include "dialog.h"
using namespace std;
#define EMODE_CONSOLE 0
#define EMODE_DIALOG 1
#define EMODE_QT 2
#define _(string) gettext(string)

typedef enum {
    MPKG_OK = 0,
	MPKG_DOWNLOAD_OK,
	MPKG_DOWNLOAD_TIMEOUT,
	MPKG_DOWNLOAD_MD5,
	MPKG_DOWNLOAD_HOST_NOT_FOUND,
	MPKG_DOWNLOAD_FILE_NOT_FOUND,
	MPKG_DOWNLOAD_LOGIN_INCORRECT,
	MPKG_DOWNLOAD_FORBIDDEN,
	MPKG_DOWNLOAD_OUT_OF_SPACE,
	MPKG_DOWNLOAD_WRITE_ERROR,
	MPKG_DOWNLOAD_ERROR,
	
	MPKG_INDEX_OK,
	MPKG_INDEX_DOWNLOAD_TIMEOUT,
	MPKG_INDEX_PARSE_ERROR,
	MPKG_INDEX_HOST_NOT_FOUND,
	MPKG_INDEX_NOT_RECOGNIZED,
	MPKG_INDEX_LOGIN_INCORRECT,
	MPKG_INDEX_FORBIDDEN,
	MPKG_INDEX_ERROR,
	
	MPKG_INSTALL_OK,
	MPKG_INSTALL_OUT_OF_SPACE,
	MPKG_INSTALL_SCRIPT_ERROR,
	MPKG_INSTALL_EXTRACT_ERROR,
	MPKG_INSTALL_META_ERROR,
	MPKG_INSTALL_FILE_CONFLICT,
	MPKG_INSTALL_NOT_IN_DB,

	MPKG_CDROM_MOUNT_ERROR,
	MPKG_CDROM_WRONG_VOLNAME,
	
	MPKG_SUBSYS_SQLDB_INCORRECT,
	MPKG_SUBSYS_SQLDB_OPEN_ERROR,
	MPKG_SUBSYS_XMLCONFIG_READ_ERROR,
	MPKG_SUBSYS_XMLCONFIG_WRITE_ERROR,
	MPKG_SUBSYS_SQLQUERY_ERROR,
	MPKG_SUBSYS_TMPFILE_CREATE_ERROR,
	MPKG_SUBSYS_FILE_WRITE_ERROR,
	MPKG_SUBSYS_FILE_READ_ERROR,
	
	MPKG_STARTUP_COMPONENT_NOT_FOUND,
	MPKG_STARTUP_NOT_ROOT
} mpkgErrorCode;

typedef enum {
	MPKG_RETURN_WAIT = 0,
	MPKG_RETURN_ABORT,
	MPKG_RETURN_SKIP,
	MPKG_RETURN_IGNORE,
	MPKG_RETURN_CONTINUE,
	MPKG_RETURN_ACCEPT,
	MPKG_RETURN_DECLINE,
	MPKG_RETURN_RETRY,
	MPKG_RETURN_REINIT,
	MPKG_RETURN_OK
} mpkgErrorReturn;

mpkgErrorCode getErrorCode();
mpkgErrorReturn getErrorReturn();
mpkgErrorReturn waitResponce(mpkgErrorCode);
void setErrorCode(mpkgErrorCode value);
void setErrorReturn(mpkgErrorReturn value);

extern mpkgErrorCode errorCode;
extern mpkgErrorReturn errorReturn;

typedef struct
{
	mpkgErrorReturn ret;
	string text;
} errorOptions;

typedef struct
{
	string text;
	mpkgErrorCode code;
	vector<errorOptions> action;
} errorDescription;

void initErrorManager(int mode=EMODE_CONSOLE);

mpkgErrorReturn callError(mpkgErrorCode, string errDetails="");

extern vector<errorDescription> errorList;
string getErrorDescription(int errCode);

#define MPKGERROR_OK		0
#define MPKGERROR_NOPACKAGE	-1
#define MPKGERROR_IMPOSSIBLE	-2
#define MPKGERROR_SQLQUERYERROR	-3
#define MPKGERROR_DOWNLOADERROR	-4
#define MPKGERROR_ABORTED	-5
#define MPKGERROR_CRITICAL	-6
#define MPKGERROR_AMBIGUITY	-7
#define MPKGERROR_INCORRECTDATA -8
#define MPKGERROR_FILEOPERATIONS -9
#define MPKGERROR_UNRESOLVEDDEPS -10
#define MPKGERROR_COMMITERROR	-11
#define MPKGERROR_NOSUCHVERSION -12
/*#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
#define MPKGERROR_
*/
#endif
