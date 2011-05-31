#include "checklibdeps.h"
#include "libmpkg.h"
#include "terminal.h"
PkgScanResults::PkgScanResults() {
}

PkgScanResults::~PkgScanResults() {
}

int PkgScanResults::parseData(string filename, vector<string> data) {
	for (size_t i=0; i<data.size(); ++i) {
		if (isUndefinedSym(data[i])) parseUndefined(filename, data[i]);
		else parseNotFound(filename, data[i]);
	}
	return data.size();
}

void PkgScanResults::parseUndefined(const string &filename, const string & data) {
	if (data.size()<strlen("undefined symbol: ")) return;
	size_t usym_s = strlen("undefined symbol: ");
	string symbol = data.substr(usym_s, data.substr(usym_s).find_first_of(" \t"));

	PkgSymbolError sError;
	sError.filename = filename;
	sError.symbol = symbol;
	symbolErrors.push_back(sError);
}

void PkgScanResults::parseNotFound(const string &filename, const string & data) {
	string libname = cutSpaces(data);
	libname = libname.substr(0, libname.find_first_of(" \t"));

	PkgNotFoundError sError;
	sError.filename = filename;
	sError.libname = libname;
	notFoundErrors.push_back(sError);


}


bool PkgScanResults::isUndefinedSym(const string &data_str) {
	if (data_str.find("undef")==0) return true;
	return false;
}

size_t PkgScanResults::size() const {
	return symbolErrors.size() + notFoundErrors.size();
}

vector<string> PkgScanResults::getLostSymbols(const vector<string>& symFilter) const {
	vector<string> ret;
	bool found;
	for (size_t i=0; i<symbolErrors.size(); ++i) {
		found = false;
		for (size_t t=0; !found && t<ret.size(); ++t) {
			if (ret[t]==symbolErrors[i].symbol) found = true;
		}
		if (found) continue;
		found = true;
		if (!symFilter.empty()) {
			found = false;
			for (size_t t=0; !found && t<symFilter.size(); ++t) {
				if (symFilter[t]==symbolErrors[i].symbol) found = true;


			}
		}
		if (found) ret.push_back(symbolErrors[i].symbol);
	}
	return ret;
}

vector<string> PkgScanResults::getLostLibs(const vector<string>& libFilter) const {
	vector<string> ret;
	bool found;
	for (size_t i=0; i<notFoundErrors.size(); ++i) {
		found = false;
		for (size_t t=0; !found && t<ret.size(); ++t) {
			if (ret[t]==notFoundErrors[i].libname) found = true;
		}
		if (found) continue;
		found = true;
		if (!libFilter.empty()) {
			found = false;
			for (size_t t=0; !found && t<libFilter.size(); ++t) {
				if (libFilter[t]==notFoundErrors[i].libname) found = true;


			}
		}
		if (found) ret.push_back(notFoundErrors[i].libname);
	}
	return ret;
}

size_t PkgScanResults::filteredSize(const vector<string>& symFilter, const vector<string>& libFilter) const {
	return getLostSymbols(symFilter).size() + getLostLibs(libFilter).size();
}


// Overloaded for list
map<const PACKAGE *, PkgScanResults> checkRevDeps(const PACKAGE_LIST &pkgList, bool fast, bool skip_symbols) {
	map<const PACKAGE *, PkgScanResults> ret;
	for (size_t i=0; i<pkgList.size(); ++i) {
		// If not installed - skip it
		if (!pkgList[i].installed()) continue;
		fprintf(stderr, "[%d/%d] %s\n", (int) i+1, (int) pkgList.size(), pkgList[i].get_name().c_str());
		ret[&pkgList[i]] = checkRevDeps(pkgList[i], fast, skip_symbols);
		if (ret[&pkgList[i]].size()>0) fprintf(stderr, "\t%sERRORS:%s %d\n", CL_RED, CL_WHITE, (int) ret[&pkgList[i]].size());
	}
	return ret;
}

PkgScanResults checkRevDeps(const PACKAGE &pkg, bool fast, bool skip_symbols) {
	PkgScanResults ret;
	
	// Check if package has files filled in, if no - report error and return empty results. This check includes that package is installed and checkable.
	if (pkg.get_files().empty()) {
		//mError("FATAL: package " + pkg.get_name() + " has no files filled in, check impossible\n");
		return ret;
	}

	// Now go scanning
	string fname;
	string tmpfile = get_tmp_file();
	vector<string> data;
	string ld_preload;
	string ldd_options;
	if (!skip_symbols) ldd_options = " -r ";
	// Create LD_LIBRARY_PATH variable. For this, we need a full list of .so paths of package
	vector<string> ld_paths;
	bool ld_found;
	for (size_t i=0; i<pkg.get_files().size(); ++i) {
		if (pkg.get_files().at(i).find(".so")==pkg.get_files().at(i).size()-3) {
			ld_found = false;
			for (size_t t=0; !ld_found && t<ld_paths.size(); ++t) {
				if (ld_paths[t]==getDirectory(pkg.get_files().at(i))) ld_found = true;
			}
			if (!ld_found) ld_paths.push_back(getDirectory(pkg.get_files().at(i)));
		}
	}
	string ld_library_path = "LD_LIBRARY_PATH=$LD_LIBRARY_PATH";
	for (size_t i=0; i<ld_paths.size(); ++i) {
		ld_library_path += ":/" + ld_paths[i];
	}
	for (size_t i=0; i<pkg.get_files().size(); ++i) {
		fname = pkg.get_files().at(i);
		if (fast) {
			if (fname.find("usr/lib")!=0 && fname.find("usr/bin/")!=0 && fname.find("bin/")!=0 && fname.find("sbin/")!=0 && fname.find("usr/sbin/")!=0) continue;
		}
		// Skip directories and special dirs with huge amount of files
		if (fname.empty() || fname[fname.size()-1]=='/' || fname.find("etc/")==0 || fname.find("dev/")==0 || fname.find("lib/modules/")==0 || fname.find("usr/share/")==0 || fname.find("usr/man/")==0 || fname.find("usr/include/")==0 || fname.find("usr/doc/")==0 || fname.find("usr/lib/locale/")==0 || fname.find("usr/lib64/locale/")==0 || fname.find("opt/")==0) continue;
		// Skip non-executable ones
		if (access(string("/" + fname).c_str(), X_OK)) continue;

		// Too slow, disabled
		//msay("[" + pkg.get_name() + ": errs: " + IntToStr(ret.symbolErrors.size() + ret.notFoundErrors.size()) + "] [" + IntToStr(i+1) + "/" + IntToStr(pkg.get_files().size()) + "]: /" + fname);
		if (fname.find("usr/lib/")==0 && fname.find("python")!=std::string::npos) ld_preload = "LD_PRELOAD=/usr/lib/libpython2.6.so ";
		else if (fname.find("usr/lib64/")==0 && fname.find("python")!=std::string::npos) ld_preload = "LD_PRELOAD=/usr/lib64/libpython2.6.so ";
		else ld_preload = "";

		system(ld_preload + " " + ld_library_path + " ldd " + ldd_options + " '" + SYS_ROOT + fname + "' 2>&1 | grep -P 'undefined symbol|not found' > " + tmpfile);
		data = ReadFileStrings(tmpfile);
		if (data.empty()) continue;
		ret.parseData(fname, data);
	}

	unlink(tmpfile.c_str());
	return ret;
}


