/* Debugging output function(s)
$Id: debug.h,v 1.13 2007/08/26 00:20:40 i27249 Exp $
*/


#ifndef DEBUG_H_
#define DEBUG_H_

#include <string>
using namespace std;
#include <stdlib.h>
//#include "config.h"
#include <mpkgsupport/mpkgsupport.h>
//#include "colors.h"
typedef int DEBUG_LEVEL;
#define TEMP_XML_DOC "/tmp/mpkg-temp-doc.xml"

#define mDebug(m) DbgPrint(__FILE__, __LINE__, __func__, m)
#define mLog(m) _mLog(__FILE__, __LINE__, __func__, m)
#define mError(m) _mError(__FILE__, __LINE__, __func__, m, false)
#define mWarning(m) _mError(__FILE__, __LINE__, __func__, m, true)
#define say printf
#ifdef DEBUG
#define ASSERT(m) (assert(m))
#else
#define ASSERT(m)
#endif
extern bool hideErrors;
std::string _mError(const char* file, int line, const char *func, std::string message, bool warn);
void DbgPrint(const char* file, int line, const char *func, std::string message);
std::string _mLog(const char * file, int line, const char *func, std::string msg);
//void debug(std::string str);

std::string strim(std::string& s, const std::string& drop = "\n\t ");

#endif //DEBUG_H_

