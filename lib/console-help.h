#ifndef MPKG_CONSOLE_HELP_H__
#define MPKG_CONSOLE_HELP_H__

#include "version.h"
#include "mpkg.h"
void showBanner();
int showCmdHelp(const string cmd, bool is_error);
#endif
