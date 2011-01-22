#ifndef MPKG_SPKGSUPPORT_H__
#define MPKG_SPKGSUPPORT_H__

#include "mpkg.h"

int emerge_package(string file_url, string *package_name, string march="", string mtune="", string olevel="", string *builddir_name_ret=NULL); // Assembly from source
#endif
