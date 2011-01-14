#ifndef _CHECKLIBDEPS_H__
#define _CHECKLIBDEPS_H__
#include "dataunits.h"

struct PkgSymbolError {
	string filename, symbol;
};

struct PkgNotFoundError {
	string filename, libname;
};

class PkgScanResults {
	public:
		PkgScanResults();
		~PkgScanResults();
		
		vector<PkgSymbolError> symbolErrors;
		vector<PkgNotFoundError> notFoundErrors;

		int parseData(string filename, vector<string> data);
		bool isUndefinedSym(const string &data_str);

		void parseUndefined(const string & filename, const string & data);
		void parseNotFound(const string & filename, const string & data);

		size_t size() const;

		vector<string> getLostSymbols(const vector<string>& symFilter) const;
		vector<string> getLostLibs(const vector<string>& libFilter) const;
		size_t filteredSize(const vector<string>& symFilter, const vector<string>& libFilter) const;
};


map<const PACKAGE *, PkgScanResults> checkRevDeps(const PACKAGE_LIST &pkgList, bool fast = false);
PkgScanResults checkRevDeps(const PACKAGE &pkg, bool fast = false);

#endif
