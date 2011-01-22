#ifndef MPKG_DELTAS_H__
#define MPKG_DELTAS_H__
#include "mpkg.h"
#include "package.h"
#include "terminal.h"
bool tryGetDelta(PACKAGE *p, const string workingDir="/var/mpkg/cache/");
long double guessDeltaSize(const PACKAGE& p, const string workingDir="/var/mpkg/cache/");
int applyXZPatch(const string& orig, const string& dest, const string& patchname, const string workingDir="/var/mpkg/cache/");


#endif
