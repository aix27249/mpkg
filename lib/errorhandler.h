#ifndef MPKG_ERRORHANDLER_H__
#define MPKG_ERRORHANDLER_H__
#include <string>
#include <vector>
using namespace std;
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
} MpkgErrorReturn;


struct ErrorOptions {
	MpkgErrorReturn ret;
	string text;
};

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
} MpkgErrorCode;


struct ErrorDescription {
	string text;
	MpkgErrorCode code;
	vector<ErrorOptions> action;
};


string getErrorDescription(int errCode); // FIXME: BAD FUNCTION, no localization


class MPKGErrorHandler {
	public:
		MPKGErrorHandler();
		~MPKGErrorHandler();

		void registerErrorHandler(MpkgErrorReturn (*newErrorHandler) (ErrorDescription errorDescription, const string& errorDetails));
		void unregisterErrorHandler();

		MpkgErrorReturn callError(MpkgErrorCode err, string errDetails="");
	private:
		MpkgErrorReturn (*errorHandler) (ErrorDescription errorDescription, const string& errorDetails);
		vector<ErrorDescription> errorList;

		void initErrorManager();


};

extern MPKGErrorHandler mpkgErrorHandler;
#endif
