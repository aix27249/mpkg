#include "deltas.h"
long double guessDeltaSize(const PACKAGE& p, const string workingDir) {
	if (p.deltaSources.empty()) return 0;
	if (_cmdOptions["enable_delta"]!="true") return 0;
	// Searching for suitable base file
	for (unsigned int i=0; i<p.deltaSources.size(); ++i) {
		if (!FileExists(workingDir + p.deltaSources[i].orig_filename)) continue;
		return strtod(p.deltaSources[i].dup_size.c_str(), NULL);
	}
	return 0;
}
// Experimental function to apply patch between two txz packages
int applyXZPatch(const string& orig, const string& dest, const string& patchname, const string workingDir) {
	string tmp = get_tmp_dir();
	return system("( cd " + tmp + " || exit 1 ; xzcat " + workingDir + orig + " > " + orig + " || exit 1 ; bpatch " + orig + " " + dest + ".tar " + workingDir + patchname + " || exit 1 ; xz -zf " + dest + ".tar && mv " + dest + ".tar.xz " + workingDir + dest + " || exit 1 ; cd " + workingDir + " && rm -rf " + tmp + " )");
}

bool tryGetDelta(PACKAGE *p, const string workingDir) {
	if (setupMode) return false;
	if (_cmdOptions["deltup"]!="true") {
		//cout << _("No delta utilities\n");
		return false;
	}
	if (p->deltaSources.empty()) {
		if (verbose && !dialogMode) say(_("\n\tNo deltas found for %s\n"), p->get_filename().c_str());
		return false;
	}
	if (_cmdOptions["enable_delta"]!="true") {
		return false;
	}
	bool dupOk=false;
	int deltupRet;
	// Searching for suitable base file
	string got_md5;
	string hideInDialog;
	if (dialogMode) hideInDialog = " >/dev/null 2>/dev/null ";
	for (unsigned int i=0; i<p->deltaSources.size(); ++i) {
		if (!FileExists(workingDir + p->deltaSources[i].orig_filename)) {
			if (verbose) say(_("No original file for delta %s\n"), string(workingDir + p->deltaSources[i].orig_filename).c_str());
			continue;
		}
		if (get_file_md5(workingDir + p->deltaSources[i].orig_filename)!=p->deltaSources[i].orig_md5) {
			if (verbose) say(_("MD5 of original file doesn't match for delta %s\n"), string(workingDir + p->deltaSources[i].orig_md5).c_str());
			continue;
		}
		// If file found and md5 are correct, check if dup is already has been downloaded. Also, say what we doing now...
		pData.setItemCurrentAction(p->itemID, _("Checking package delta"));
			
		msay(_("Checking delta and trying to download it: ") + p->get_name());

		if (FileExists(workingDir + getFilename(p->deltaSources[i].dup_url))) {
			if (get_file_md5(workingDir + getFilename(p->deltaSources[i].dup_url))==p->deltaSources[i].dup_md5) {
				dupOk=true;
			}
			else unlink(string(workingDir + getFilename(p->deltaSources[i].dup_url)).c_str()); // Bad dup is so bad...
		}
		DownloadResults dres;
		if (!dupOk) {
			dres = CommonGetFile(p->deltaSources[i].dup_url, workingDir + getFilename(p->deltaSources[i].dup_url));
			if (dres != DOWNLOAD_OK) {
				printf("Failed to download delta from %s\n", p->deltaSources[i].dup_url.c_str());
				return false;
			}
		}
		// Try to merge
		unlink(string(workingDir + p->get_filename()).c_str());
		// If package type is txz, apply dirty hack (maybe it is slow, but it works)
		if (getExtension(getFilename(p->deltaSources[i].orig_filename))=="txz" && getExtension(p->get_filename())=="txz") {
			deltupRet = applyXZPatch(getFilename(p->deltaSources[i].orig_filename), p->get_filename(), getFilename(p->deltaSources[i].dup_url), workingDir);
		}
		else {
			deltupRet = system("( cd " + workingDir + " " + hideInDialog + " || exit 1 ; deltup -p -d " + workingDir + " -D " + workingDir + " " + workingDir + getFilename(p->deltaSources[i].dup_url) + hideInDialog + " || exit 1 )");
		}
		if (deltupRet==0) {
			// Check md5 of result
			got_md5 = get_file_md5(workingDir + p->get_filename());
			if (got_md5 == p->get_md5()) {
				if (!dialogMode) say(_("Merged file MD5 OK\n"));
			}
			else {
				if (!dialogMode) say(_("Merge OK, but got wrong MD5!\n"));
				return false;
			}
			if (!dialogMode) say(_("Delta patch successfully merged in %s\n"), p->get_filename().c_str());
			// Deleting delta file, because we don't need it anymore:
			unlink(string(workingDir + getFilename(p->deltaSources[i].dup_url)).c_str());
			return true;
		}
		else {
			if (!dialogMode) {
				if (i+1<p->deltaSources.size()) say(_("%s patch failed to merge, trying next one\n"), p->deltaSources[i].dup_url.c_str());
				else say(_("%s patch failed to merge, going to download full package\n"), p->deltaSources[i].dup_url.c_str());
			}
		}
	}
	return false;
}

