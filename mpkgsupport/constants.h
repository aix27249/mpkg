/* Miscellaneous constants
    $Id: constants.h,v 1.10 2007/10/20 10:34:50 i27249 Exp $
*/


#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define CHKINSTALL_NOTFOUND 1
#define CHKINSTALL_DBERROR 2
#define CHKINSTALL_FILECONFLICT 3
#define CHKINSTALL_UNAVAILABLE 4
#define CHKINSTALL_REMOVE 5
#define CHKINSTALL_REMOVE_PURGE 6
#define CHKINSTALL_INSTALL 7
#define CHKINSTALL_INSTALLED 8
#define CHKINSTALL_PURGE 9
#define CHKINSTALL_AVAILABLE 10

#define ALREADY_INSTALLED 10
#define ERR_FILE_CONFLICT 11
#define ERR_UNKNOWN_STATUS 12
#define ALREADY_MARKED 13
#define ERR_UNAVAILABLE 14
#define ERR_UNKNOWN 15
#define ERR_NOTIMPLEMENTED 16

// Extended status definitions

//-----installed status-------
#define ST_INSTALLED	1
#define ST_NOTINSTALLED 0

//-----available status------
#define ST_AVAILABLE 	1
#define ST_NOTAVAILABLE 0

//-----config files----------
#define ST_CONFIGEXIST	1
#define ST_CONFIGNOTEXIST 0

//-----actions---------------
#define ST_NONE		0
#define ST_INSTALL	1
#define ST_REMOVE	2
#define ST_PURGE	3
#define ST_UPDATE	4 	// equivalent to ST_REMOVE, but is required for GUI to make unroll possible
#define ST_REPAIR	5
// File type definitions
// Regular file
#define FTYPE_PLAIN	0
// Config file (mpkg internal only)
#define FTYPE_CONFIG	1
// Temp file (mpkg internal only)
#define FTYPE_TEMP	5
// Special file definitions: not used now, but maybe in future
#define FTYPE_SYMLINK	2
#define FTYPE_DIR	3
#define FTYPE_BLK	4
#define FTYPE_FIFO	6
#define FTYPE_SOCK	7
#define FTYPE_CHAR	8




#endif
