/*
 * MOPSLinux package system - error codes
 * $Id: errorcodes.h,v 1.5 2007/08/02 10:39:13 i27249 Exp $
 */

#ifndef ERRORCODES_H_
#define ERRORCODES_H_
/*#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <map>
#include <nwidgets/ncurses_if.h>
using namespace std;
//#define EMODE_CONSOLE 0
//#define EMODE_DIALOG 1
//#define EMODE_QT 2
#define _(string) gettext(string)


//mpkgErrorCode getErrorCode();
//mpkgErrorReturn getErrorReturn();
//mpkgErrorReturn waitResponce(mpkgErrorCode);
//void setErrorCode(mpkgErrorCode value);
//void setErrorReturn(mpkgErrorReturn value);

//extern mpkgErrorCode errorCode;
//extern mpkgErrorReturn errorReturn;


//mpkgErrorReturn callError(mpkgErrorCode, string errDetails="");
*/

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

#endif
