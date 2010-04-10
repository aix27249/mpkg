/*
    Comparsion conditions constants and conversion functions
    $Id: conditions.h,v 1.9 2007/12/04 18:48:34 i27249 Exp $
*/

#ifndef CONDITIONS_H_
#define CONDITIONS_H_
#include "debug.h"
#define VER_MORE 1	// >
#define VER_LESS 2	// <
#define VER_EQUAL 3	// ==
#define VER_NOTEQUAL 4	// !=
#define VER_XMORE 5	// >=
#define VER_XLESS 6	// <=
#define VER_ANY 7	// any

#define COND_MORE 	"more"		// >	1
#define COND_LESS 	"less"		// <	2
#define COND_EQUAL 	"equal"		// ==	3
#define COND_NOTEQUAL 	"notequal"	// !=	4
#define COND_XMORE 	"atleast"	// >=	5
#define COND_XLESS 	"notmore"	// <=	6
#define COND_ANY	"any"		// any	7
#define COND_ANY2	"(any)"		// (any) 8 // Workaround for broken packages

#define HCOND_MORE 	">"
#define HCOND_LESS 	"<"
#define HCOND_EQUAL 	"=="
#define HCOND_EQUAL2	"="
#define HCOND_NOTEQUAL 	"!="
#define HCOND_XMORE 	">="
#define HCOND_XMORE2	"=>"
#define HCOND_XLESS 	"<="
#define HCOND_XLESS2	"=<"
#define HCOND_ANY	"any"
#define HCOND_ANY2	"(any)"


#include <string>
using namespace std;
int condition2int(string condition);
string hcondition2xml(string condition);
string condition2xml(string s_condition);

#endif //CONDITIONS_H_
