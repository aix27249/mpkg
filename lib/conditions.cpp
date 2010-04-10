/*
    Dependency and suggestions conditions conversion
    $Id: conditions.cpp,v 1.13 2007/12/04 18:48:34 i27249 Exp $
*/


#include "conditions.h"
#include <mpkgsupport/mpkgsupport.h>
//#include "string_operations.h"
int condition2int(string condition)
{
	if (condition==COND_MORE) return VER_MORE;
	if (condition==COND_LESS) return VER_LESS;
	if (condition==COND_EQUAL) return VER_EQUAL;
	if (condition==COND_NOTEQUAL) return VER_NOTEQUAL;
	if (condition==COND_XMORE) return VER_XMORE;
	if (condition==COND_XLESS) return VER_XLESS;
	if (condition==COND_ANY) return VER_ANY;
	if (condition==COND_ANY2) return VER_ANY;
	mError(string(__func__) + "error input (unknown condition): [" + condition + "]");
	return -1; // SUPER_PUPER_ERROR
}
string condition2xml(string s_condition)
{
	int condition = atoi(s_condition.c_str());
	if (condition==VER_MORE) return COND_MORE;
	if (condition==VER_LESS) return COND_LESS;
	if (condition==VER_EQUAL) return COND_EQUAL;
	if (condition==VER_NOTEQUAL) return COND_NOTEQUAL;
	if (condition==VER_XMORE) return COND_XMORE;
	if (condition==VER_XLESS) return COND_XLESS;
	if (condition==VER_ANY) return COND_ANY;
	mError(string(__func__) + "error input (unknown condition): " + IntToStr(condition));
	return ""; // SUPER_PUPER_ERROR
}

string hcondition2xml(string condition)
{
	condition = cutSpaces(condition);
	if (condition == HCOND_MORE) return COND_MORE;
	if (condition == HCOND_LESS) return COND_LESS;
	if (condition == HCOND_NOTEQUAL) return COND_NOTEQUAL;
	if (condition == HCOND_XMORE || condition == HCOND_XMORE2) return COND_XMORE;
	if (condition == HCOND_XLESS || condition == HCOND_XLESS2) return COND_XLESS;
	if (condition == HCOND_EQUAL || condition == HCOND_EQUAL2) return COND_EQUAL;
	if (condition == HCOND_ANY || condition == HCOND_ANY2) {
		return COND_ANY;
	}
	mError(string(__func__) + "Unknown condition " + condition);
	return COND_ANY;
}
