
#include <mpkg/libmpkg.h>
#include "createdeltas.h"
unsigned int _totalcount=0, _totaldeltacount=0;

PkgList* pkgClassPtr=NULL;


void createDeltaPair(const string& orig, const string& dest) {
	//printf("%s\n", __func__);
	string dpath=_cmdOptions["deltapath"];
	if (dpath.empty()) dpath = getDirectory(dest);
	string dup_name = dpath +  "/" + getFilename(orig) + "_to_" + getFilename(dest) + ".dup";
	if (FileExists(dup_name)) {
		printf("File %s already exists\n", dup_name.c_str());
		return;
	}
	//FIXME: paths may differ HARDLY, make deltup see them
	printf("Creating delta in %s: %s => %s\n", dpath.c_str(), orig.c_str(), dest.c_str());
	string dupcmd;
	if (getExtension(orig)=="txz" && getExtension(dest)=="txz") {
		string tmp = get_tmp_dir();
		dupcmd =  "( xzcat " + orig + " > " + tmp + "/" + getFilename(orig) + " && xzcat " + dest + " > " + tmp + "/" + getFilename(dest) + " && cd " + tmp + " && bdelta " + getFilename(orig) + " " + getFilename(dest) + " " + getAbsolutePath(dpath) + "/" + getFilename(orig) + "_to_" + getFilename(dest) + ".dup && exit 1 )"; 
	}
	else dupcmd = "( cd " + getDirectory(dest) + " || exit 1 ; deltup -mjbe 9 -D " + getAbsolutePath(getDirectory(orig)) + " " + getFilename(orig) + " " + getFilename(dest) + " " + getAbsolutePath(dpath) + "/" + getFilename(orig) + "_to_" + getFilename(dest) + ".dup || exit 1 )";
	printf("%s\n", dupcmd.c_str());
	system(dupcmd);
	_totaldeltacount++;
}

int processPackage(const char *filename, const struct stat *, int filetype) {
	if (filetype!=FTW_F) {
	//	printf("%s: not a file\n", filename);
		return 0;
	}
	string basePath=getDirectory(filename);
	string baseFilename = getFilename(filename);
	string name, thisName, thisVersion, version, thisArch, arch, thisBuild, build;
	string ext = getExtension(filename);
	_totalcount++;
	if (ext!="tgz" && ext != "tbz" && ext != "tlz" && ext != "txz") {
		return 0;
	}
	if (!parseSlackwareFilename(baseFilename, &thisName, &thisVersion, &thisArch, &thisBuild)) {
		printf("Failed to parse filename %s\n", baseFilename.c_str());
		return 0;
	}
	vector<string> bases = pkgClassPtr->getBaseFor(filename);
	for (unsigned int i=0; i<bases.size(); ++i) {
		createDeltaPair(bases[i], filename);
	}

	return 0;
}


int main(int , char **) {
	
	vector<string> basePkgPaths;
	basePkgPaths.push_back(".");
	vector<string> tmp=ReadFileStrings(".deltapath");
	if (!tmp.empty()) _cmdOptions["deltapath"]=tmp[0];
	printf("DELTAPATH: %s\n", _cmdOptions["deltapath"].c_str());
	tmp=ReadFileStrings(".oldpkgpath");
	if (!tmp.empty()) {
		_cmdOptions["oldpkgpath"]=tmp[0];
		basePkgPaths.push_back(tmp[0]);
	}
	PkgList pkgList(basePkgPaths);
	pkgClassPtr = &pkgList;

	ftw(".", processPackage, 600);
	printf("Total %d files found, deltified: %d\n", _totalcount, _totaldeltacount);
	return 0;
}

