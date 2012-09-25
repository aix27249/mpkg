/******************************************************************
 * Repository class: build index, get index...etc.
 * $Id: repository.cpp,v 1.74 2007/11/03 01:08:15 i27249 Exp $
 * ****************************************************************/
#include "repository.h"
#include <iostream>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/debugXML.h>
#include "terminal.h"
#include "xml2pkglist.h"
Repository::Repository(){
	package_descriptions = NULL;
}
Repository::~Repository(){}
bool validateRepStr(const string& r) {
	if (r.find(":/")!=std::string::npos) return true;
	else return false;
}


xmlNodePtr _rootNode;
xmlDocPtr __doc;

int slackpackages2list (string *p, string *m, PACKAGE_LIST *pkgList, string server_url) {
	pkgList->clear();
	//printf("making strings\n");
	vector<string> packageList = MakeStrings(*p);
	vector<string> md5List = MakeStrings(*m);
	PACKAGE *pkg = new PACKAGE; // temp package
	LOCATION *tmpLoc = new LOCATION;
	vector<int> packageStart;
	unsigned int md5strLength=0;
	string packageFullPath;
	for (unsigned int i=0; i<packageList.size(); ++i) {
		if (packageList[i].find("PACKAGE NAME:  ")==0) {
			packageStart.push_back(i);
		}
	}
	//printf("Found: %d packages\n", packageStart.size());

	string *c, tmp, short_description;
	string name, version, arch, build;
	bool bad_package;
	// Go reading and converting packages!
	size_t start=0, end=packageList.size();
	size_t k;
	for (unsigned int i=0; i<packageStart.size(); ++i) {
		bad_package = false;
		delete pkg;
		delete tmpLoc;
		pkg = new PACKAGE;
		tmpLoc = new LOCATION;
		start=packageStart[i];
		if (i+1<packageStart.size()) end = packageStart[i+1];
		else end = packageList.size();
		for (unsigned int l=start; !bad_package && l<end; ++l) {
			c = &packageList[l];
			if (c->find("PACKAGE NAME:  ")==0) {
				tmp = c->substr(c->find("PACKAGE NAME:  ") + strlen("PACKAGE NAME:  "));
				//printf("0: [%s]\n", tmp.c_str());
				// got filename: some-name-version-arch-build.tgz
				// GO PARSE IT!
				
				// Cut spaces if any:
				tmp = cutSpaces(tmp);
				//printf("1: [%s]\n", tmp.c_str());
				
				// Check extension
				if (getExtension(tmp)!="tgz" && getExtension(tmp)!="tlz" && getExtension(tmp)!="txz" && getExtension(tmp)!="tbz") {
					printf(_("Malformed package name or unknown package extension: %s\n"), tmp.c_str());
					bad_package = true;
					break;
				}
				
				// Got filename
				pkg->set_filename(tmp);

				// Cutting extension
				tmp = tmp.substr(0, tmp.size()-4);
				//printf("2: [%s]\n", tmp.c_str());

				// Build
				k=tmp.find_last_of("-");
				if (k==std::string::npos || k==tmp.size()-1) {
					printf(_("Malformed package name: %s\n"), tmp.c_str());
					bad_package = true;
					break;
				}
				build = tmp.substr(k+1);
				tmp = tmp.substr(0,k);
				//printf("build: [%s], remaining: [%s]\n", build.c_str(), tmp.c_str());

				// Arch
				k = tmp.find_last_of("-");
				if (k==std::string::npos || k==tmp.size()-1) {
					printf(_("Malformed package name: %s\n"), tmp.c_str());
					bad_package = true;
					break;
				}
				arch = tmp.substr(k+1);
				tmp = tmp.substr(0,k);
				//printf("arch: [%s], remaining: [%s]\n", build.c_str(), tmp.c_str());
			
				// Version
				k = tmp.find_last_of("-");
				if (k==std::string::npos || k==tmp.size()-1) {
					printf(_("Malformed package name: %s\n"), tmp.c_str());
					bad_package = true;
					break;
				}
				version = tmp.substr(k+1);
				tmp = tmp.substr(0,k);
				//printf("version: [%s], remaining: [%s]\n", build.c_str(), tmp.c_str());

				// Name: all that last.
				name = tmp;
				//printf("name: [%s]\n", name.c_str());

				// Collect data into object
				pkg->set_name(name);
				pkg->set_version(version);
				pkg->set_arch(arch);
				pkg->set_build(build);
				continue;
			}
			if (c->find("PACKAGE LOCATION:  ")==0) {
				tmp = c->substr(c->find("PACKAGE LOCATION:  ") + strlen("PACKAGE LOCATION:  "));
				// Cutting leading dotslash
				if (tmp.find("./")==0) tmp = tmp.substr(2);
				tmpLoc->set_server_url(server_url);
				tmpLoc->set_path(tmp);
				pkg->get_locations_ptr()->push_back(*tmpLoc);
				continue;
			}
			if (c->find("PACKAGE SIZE (compressed):  ")==0) {
				tmp = c->substr(c->find("PACKAGE SIZE (compressed):  ") + strlen("PACKAGE SIZE (compressed):  "));
				if (tmp.find(" K")==std::string::npos) {
					printf(_("Malformed metadata: compressed size unreadable\n"));
					bad_package = true;
					break;
				}
				tmp = IntToStr(atoi(tmp.substr(0,tmp.find(" ")).c_str())*1024);
				pkg->set_compressed_size(tmp);
				//printf("compressed size: %s\n", tmp.c_str());
				continue;
			}
			if (c->find("PACKAGE SIZE (uncompressed):  ")==0) {
				tmp = c->substr(c->find("PACKAGE SIZE (uncompressed):  ") + strlen("PACKAGE SIZE (uncompressed):  "));
				if (tmp.find(" K")==std::string::npos) {
					printf(_("Malformed metadata: uncompressed size unreadable\n"));
					bad_package = true;
					break;
				}
				tmp = IntToStr(atoi(tmp.substr(0,tmp.find(" ")).c_str())*1024);
				pkg->set_installed_size(tmp);
				//printf("uncompressed size: %s\n", tmp.c_str());
				continue;
			}

			// temporarily unimplemented ones
			if (c->find("PACKAGE REQUIRED:  ")==0) {
				continue;
			}
			if (c->find("PACKAGE SUGGESTS:  ")==0) {
				continue;
			}
			if (c->find("PACKAGE CONFLICTS:  ")==0) {
				continue;
			}
			
			// description
			if (c->find("PACKAGE DESCRIPTION:")==0) {
				// Description is a multiline structure
				tmp.clear();
				short_description.clear();
//				printf("found description\n");
				for (unsigned int d = l; d<end && d<l+12; ++d) {
					c = &packageList[d];
//					printf("c: %s\n",c->c_str());
					if (c->find(name+": ")==0) {
						//printf("got line\n");
						if (short_description.empty()) {
							short_description = c->substr(name.length()+2);
							pkg->set_short_description(short_description);
						}
						else {
							tmp += c->substr(name.length()+2)+"\n";
							//printf("found desc: %s\n", c->substr(name.length()+2).c_str());
						}
					}
				}
				//printf("sd: %s, desc: %s\n", short_description.c_str(), tmp.c_str());
				pkg->set_description(tmp);
				l=end;
			}
		} // End of reading PACKAGES.TXT
		
		// Finally, find MD5 for package
		if (bad_package) continue;

		packageFullPath = "./" + pkg->get_locations().at(0).get_path() + pkg->get_filename();
		
		md5strLength = 32+2+packageFullPath.size();//pkg->get_locations()->at(0).get_path()->size() + pkg->get_filename()->size();
		for (unsigned int l=0; l<md5List.size(); ++l) {
			if (md5List[l].size()==md5strLength && md5List[l].find(packageFullPath)!=std::string::npos) {
				// Update: adapt parser for accept some "malformed" md5 lists is disabled: if you want to enable this, uncomment next line, and set >= instead of == in the previous line
				//if (md5List[l].size()==md5strLength || md5List[l].substr(md5strLength).find_first_not_of(" \n\t")==std::string::npos) {
					tmp = md5List[l].substr(0,32);
					// Check for md5 validity
					if (tmp.find_first_not_of("0123456789abcdef")!=std::string::npos) {
						printf("md5 contains invalid characters: [%s]\n", tmp.c_str());
						bad_package=true;
						break;
					}
					pkg->set_md5(tmp);
				//} // To use more safe algorithm, uncomment this too
			}
		}
		if (!bad_package) pkgList->add(*pkg);
	}
	//printf("recognized %d packages\n", pkgList->size());
	delete pkg;
	delete tmpLoc;
	return 0;
}
vector<string>* pkgListPtr=NULL;
int pkgListIndexPackage(const char *filename, const struct stat *, int filetype) {
	if (filetype!=FTW_F) {
		return 0;
	}
	string baseFilename = getFilename(filename);
	string name, thisName, thisVersion, version, thisArch, arch, thisBuild, build;

	string ext = getExtension(filename);
	if (ext!="tgz" && ext != "tbz" && ext != "tlz" && ext != "txz") {
		return 0;
	}
	if (!parseSlackwareFilename(baseFilename, &thisName, &thisVersion, &thisArch, &thisBuild)) {
		return 0;
	}
	if (pkgListPtr) pkgListPtr->push_back(filename);
	return 0;
}

PkgList::PkgList(const vector<string>& paths) {
	pkgListPtr = &pkglist;
	for (unsigned int i=0; i<paths.size(); ++i) {
		ftw(paths[i].c_str(), pkgListIndexPackage, 600);
	}
}

PkgList::~PkgList() {
}

vector<string> PkgList::getBaseFor(const string& filename) {
	string basePath=getDirectory(filename);
	string baseFilename = getFilename(filename);
	string name, thisName, thisVersion, version, thisArch, arch, thisBuild, build;
	vector<string> ret;
	if (!parseSlackwareFilename(baseFilename, &thisName, &thisVersion, &thisArch, &thisBuild)) {
		printf("Failed to parse filename %s\n", baseFilename.c_str());
		return ret;
	}
	for (unsigned int i=0; i<pkglist.size(); ++i) {
		if (!parseSlackwareFilename(getFilename(pkglist[i]), &name, &version, &arch, &build)) {
			continue;
		}
		if (name != thisName) {
			continue;
		}
		if (version==thisVersion && build==thisBuild) {
			continue;
		}
		if (version==thisVersion && strverscmp2(thisBuild, build)>0) {
			ret.push_back(pkglist[i]);
			continue;
		}
		if (strverscmp2(thisVersion, version)>0) {
			ret.push_back(pkglist[i]);
			continue;
		}
	}
	return ret;
}


unsigned int pkgcounter;
vector<string> pkgDupeNames;
vector<string> pkgDupeVersions;
vector<string> pkgDupePaths;
vector<TagPair> repositoryTags;
vector<TagPair> distroVersion;
vector<string> skipDirs;
int pkgcount_ftw;
string relative_path_shift;
bool makeFilelist = true;
PACKAGE_LIST *__pkgCache;
xmlNodeSetPtr __xmlCache;
vector<DeltaSource> detectFileDeltas(const string& filename, const string deltadir="", const string oldpkgdir="", const vector<DeltaSource>* cache=NULL) {
	vector<DeltaSource> ret;
	string basePath=getDirectory(filename);
	string baseFilename=getFilename(filename);
//	printf("filename: [%s], basePath: [%s], baseF: [%s]\n", filename.c_str(), basePath.c_str(), baseFilename.c_str());
	string origFilename;
	vector<string> dirList;
       	if (deltadir.empty()) dirList = getDirectoryList(basePath, NULL, true);
	else dirList = getDirectoryList(deltadir, NULL, true);
	vector<string> oldpkgpaths;
	oldpkgpaths.push_back(basePath);
	if (!oldpkgdir.empty()) oldpkgpaths.push_back(oldpkgdir);
	PkgList pkgList(oldpkgpaths);
	bool oldfound=false;
	string srvOldFilename;
	for (unsigned int i=0; i<dirList.size(); ++i) {
		if (dirList[i].find("_to_" + baseFilename + ".dup")==std::string::npos) {
			//printf("%s: not a dup for this one\n", dirList[i].c_str());
			continue;
		}
		origFilename = dirList[i].substr(0, dirList[i].find("_to_"));
		for (unsigned int t=0; t<pkgList.pkglist.size(); ++t) {
			//printf("Checking %s\n", pkgList.pkglist[t].c_str());
			if (pkgList.pkglist[t].find(origFilename)!=std::string::npos) {
				srvOldFilename = pkgList.pkglist[t];
			//	printf("FOUND: %s\n", srvOldFilename.c_str());
				oldfound=true;
				break;
			}
		}
		if (!oldfound) {
			printf("Not found original: %s\n", origFilename.c_str());
			continue;
		}
//		printf("Found delta: %s\n", dirList[i].c_str());
		struct stat fstat;
		stat(string(deltadir + "/" + dirList[i]).c_str(), &fstat);
		string csize;
	        string oldfile_md5;	
		string dup_md5;
		csize = IntToStr(fstat.st_size);
		// Trying to use cache
		if (cache) {
			for (unsigned int t=0; t<cache->size(); ++t) {
				if (cache->at(t).dup_url.find(deltadir+"/"+dirList[i])!=std::string::npos) {
					if (cache->at(t).dup_size == csize) {
						oldfile_md5 = cache->at(t).orig_md5;
						dup_md5 = cache->at(t).dup_md5;
						printf("DELTA %s: cached\n", cache->at(t).dup_url.c_str());
						break;
					}
					else {
						printf("DELTA %s: NOT cached: size mismatch\n", cache->at(t).dup_url.c_str());
						break;
					}
				}
			}
			if (dup_md5.empty()) printf("DELTA %s: NEW DELTA\n", string(deltadir+"/" + dirList[i]).c_str());
		}
	       	if (oldfile_md5.empty()) oldfile_md5 = get_file_md5(srvOldFilename);
	        if (dup_md5.empty()) dup_md5 = get_file_md5(deltadir+"/" + dirList[i]);

		ret.push_back(DeltaSource(deltadir+"/"+dirList[i], dup_md5, origFilename, oldfile_md5, csize));
	}
	return ret;
}
int ProcessPackage(const char *filename, const struct stat *file_status, int filetype)
{
	//XMLNode tmpX;
	unsigned short x=0, y=0;

	if (file_status->st_ino!=0) x=y;

	//mDebug("processing package "+ (string) filename);
       	string ext = getExtension(filename);
	vector<string> fileList;

	if (filetype==FTW_F)
	{
		if (ext=="spkg" && !enableSpkgIndexing) return 0;
		if (ext!="tgz" && ext!="spkg" && ext!="tlz" && ext!="tbz" && ext!="txz") return 0;

		pkgcounter++;
		// Checking for repository branch
		string path = getDirectory(filename);
		string *branch = NULL;
		string *distro = NULL;
		unsigned int max_size = 0;
		unsigned int max_d_size = 0;
		for (unsigned int i=0; i<skipDirs.size(); ++i) {
			if (path.find(skipDirs[i])) return 0;
		}
		for (unsigned int i=0; i<repositoryTags.size(); ++i) {
			if (path.find(repositoryTags[i].tag)==0 && repositoryTags[i].tag.size()>max_size) {
				max_size = repositoryTags[i].tag.size();
				branch = &repositoryTags[i].value;
			}
		}
		for (unsigned int i=0; i<distroVersion.size(); ++i) {
			if (path.find(distroVersion[i].tag)==0 && distroVersion[i].tag.size()>max_d_size) {
				max_d_size = distroVersion[i].tag.size();
				distro = &distroVersion[i].value;
			}
		}

		string branch_info;
		if (branch) branch_info = " (branch: " + *branch + ")";
		string distro_info;
		if (distro) distro_info = " (distro: " + *distro + ")";

		if (!dialogMode) printf(_("[%d/%d] indexing file %s%s%s\n"),pkgcounter,pkgcount_ftw, filename, branch_info.c_str(), distro_info.c_str());
		else {
			ncInterface.setSubtitle(_("Indexing repository"));
			ncInterface.showProgressBar("["+IntToStr(pkgcounter) + "/" + IntToStr(pkgcount_ftw), _("] indexing file ") + (string) filename + branch_info, pkgcounter, pkgcount_ftw);
			ncInterface.setProgress(pkgcounter);
		}
		FILE *log=fopen("index.log", "a");
		LocalPackage lp(filename, relative_path_shift);
		// Checking for cache
		int pkgCacheIndex = -1;
		for (size_t i=0; i<__pkgCache->size(); ++i) {
			if (__pkgCache->at(i).get_filename() == getFilename(filename) && __pkgCache->at(i).get_compressed_size() == IntToStr(getFileSize(filename))) {
				//printf("%s vs %s\n", __pkgCache->at(i).get_locations()[0].get_path().c_str(), getDirectory(filename).c_str());
				if (__pkgCache->at(i).get_locations()[0].get_path() == getDirectory(filename)+"/") {
					//fprintf(stderr, "%s: Cached\n", filename);
					pkgCacheIndex = i;
					break;
				}
			}
		}
		int errCode;
	       	if (pkgCacheIndex == -1) {
			errCode = lp.injectFile();
			if (makeFilelist) lp.fill_filelist();
			if (branch) lp.set_repository_branch(*branch);
			if (distro) lp.set_distro_version(*distro);
		}
		else {
			errCode = 0;
			lp.data = __pkgCache->at(pkgCacheIndex);
		}
		/* I think we will drop deltas support.
		const vector<DeltaSource> *deltaCache = NULL;
		if (pkgCacheIndex != -1) deltaCache = &__pkgCache->at(pkgCacheIndex).deltaSources;
		vector<DeltaSource> deltas=detectFileDeltas(filename, _cmdOptions["deltapath"], _cmdOptions["oldpkgpath"], deltaCache);
		if (pkgCacheIndex == -1) lp.set_deltasources(deltas);
		else setXmlDeltaSources(deltas, __xmlCache->nodeTab[pkgCacheIndex]);
		//printf("Delta set\n");
		//if (!errCode) lp.create_signature(); // DISABLED BCOZ NOT NEEDED
		*/
		if (log)
		{
			if (errCode==0)
			{
				fprintf(log, "indexing package %s: OK\n", filename);
				fclose(log);
			}
			else 
			{
				fprintf(log, "indexing file %s FAILED: error code %d\n", filename, errCode);
				fclose(log);
				return 0;
			}

		}
		else mError(_("unable to open log file"));
		int bufsize;
		if (pkgCacheIndex == -1) {
			xmlChar * membuf = lp.getPackageXMLNodeXPtr(&bufsize);
			xmlDocPtr __tmpXmlDoc = xmlParseMemory((const char *) membuf, bufsize);//"UTF-8", 0);
			xmlFree(membuf);
			xmlNodePtr __packagesRootNode = xmlDocGetRootElement(__tmpXmlDoc);
			xmlAddChild(_rootNode, __packagesRootNode);
		}
		else {
			// Bug #1286: need to be rewritten via metaframe. See http://trac.agilialinux.ru/ticket/1286
			xmlAddChild(_rootNode, __xmlCache->nodeTab[pkgCacheIndex]);
		}
		// Make file list. Don't regenerate this if using cache
		if (makeFilelist && pkgCacheIndex == -1) {
			string subpath = getDirectory(filename);
			if (subpath.find(relative_path_shift)==0) subpath = subpath.substr(relative_path_shift.size());
			string cmd = "mkdir -p " + relative_path_shift + "/.mpkg_filelists/" + subpath;
			system(cmd);
			
			if (_cmdOptions["md5fileindex"]=="true") {
				string tmpdir = get_tmp_dir();
				system("tar xf " + string(filename) + " --no-same-owner -C " + tmpdir);
				vector<string> md5sums;
				int ftype;
				for (size_t i=0; i<lp.data.get_files().size(); ++i) {
					ftype = get_file_type_stat(tmpdir + "/" + lp.data.get_files().at(i));
					if (ftype==FTYPE_PLAIN || ftype==FTYPE_SYMLINK) {
						md5sums.push_back(get_file_md5(tmpdir + "/" + lp.data.get_files().at(i)));
					}
					else {
						if (ftype==FTYPE_DIR) md5sums.push_back("DIR");
						else md5sums.push_back("SPECIAL");
					}
					
					fileList.push_back(lp.data.get_files().at(i) + " " + md5sums[md5sums.size()-1] + " " + IntToStr(ftype));
				}
				system("chmod -R 777 " + tmpdir);
				system("rm -rf " + tmpdir);
			}
			else {
				for (size_t i=0; i<lp.data.get_files().size(); ++i) {
					fileList.push_back(lp.data.get_files().at(i));
				}
			}
			WriteFileStrings(relative_path_shift + "/.mpkg_filelists/" + subpath + "/" + getFilename(filename) + ".filelist", fileList);
		}
		// Dupe check
		for (unsigned int i=0; i<pkgDupeNames.size(); i++)
		{
			if (lp.data.get_name()==pkgDupeNames[i])
			{
				// Dupe found, notify!
				if (!dialogMode && verbose) say(_("Note: duplicate package %s %s found for %s %s\n"), pkgDupeNames[i].c_str(), pkgDupeVersions[i].c_str(), lp.data.get_filename().c_str(), lp.data.get_fullversion().c_str());
				FILE *duplog = fopen("dupes.log", "a");
				if (duplog)
				{
					fprintf(duplog, "%s: %s => %s\n", getFilename(filename).c_str(), pkgDupePaths[i].c_str(), lp.data.get_locations().at(0).get_full_url().c_str());
					fclose(duplog);
				}
			}
		}
		pkgDupeNames.push_back(lp.data.get_name());
		pkgDupeVersions.push_back(lp.data.get_fullversion());
		pkgDupePaths.push_back(lp.data.get_locations().at(0).get_full_url());
	}
	return 0;
}

int countPackage(const char *filename, const struct stat *file_status, int filetype)
{
	
	//XMLNode tmpX;
	unsigned short x=0, y=0;

	if (file_status->st_ino!=0) x=y;

	//mDebug("processing package "+ (string) filename);
       	string ext = getExtension(filename);

	if (filetype==FTW_F) {
		if (ext=="spkg" && !enableSpkgIndexing) return 0;
		if (ext == "tgz" || ext == "tlz" || ext == "spkg" || ext == "txz" || ext == "tbz" ) pkgcount_ftw++;
	}
	vector<string> tmp;
	if (getFilename(filename)==".branch") {
		printf("%s: detected as branch, adding to tag db\n", filename);
		tmp = ReadFileStrings(filename);
		if (!tmp.empty()) {
			printf("Branch: %s\n", cutSpaces(tmp[0]).c_str());
			repositoryTags.push_back(TagPair(getDirectory(filename), cutSpaces(tmp[0])));
		}
	}
	if (getFilename(filename)==".distro") {
		printf("%s: detected as distro, adding to distroVersion db\n", filename);
		tmp = ReadFileStrings(filename);
		if (!tmp.empty()) {
			printf("Distro: %s\n", cutSpaces(tmp[0]).c_str());
			distroVersion.push_back(TagPair(getDirectory(filename), cutSpaces(tmp[0])));
		}
	}

	if (getFilename(filename)==".mpkg_skip") {
		printf("%s: requested to skip\n", filename);
		skipDirs.push_back(getDirectory(filename));
	}


	if (dialogMode) ncInterface.setProgressMax(pkgcount_ftw);
	return 0;
}
void importIndexCache(const std::string& path) {
	Repository rep;
	rep.get_index("file://" + getAbsolutePath(path), __pkgCache);
	fprintf(stderr, "Using cached indexing: cached %d packages\n", __pkgCache->size());
}

int Repository::build_index(string path, bool make_filelist)
{
	makeFilelist = make_filelist;
	relative_path_shift = path;
	pkgDupeNames.clear();
	pkgDupeVersions.clear();
	unlink("index.log");
	unlink("dupes.log");
	unlink("legacy.log");
	pkgcounter=0;
	
	__doc = xmlNewDoc((const xmlChar *)"1.0");
	_rootNode = xmlNewNode(NULL, (const xmlChar *)"repository");
	xmlDocSetRootElement(__doc, _rootNode);
    	
	// Next, run thru files and extract data.
	// We consider that repository root is current directory. So, what we need to do:
	// Enter each sub-dir, get each file which name ends with .tgz | .tlz | .spkg, extracts xml (and other) data from them, 
	// and build an XML tree for whole repository, then write it to ./packages.xml
	
	// Zero: get special directories
	vector<string> tmp = ReadFileStrings(path+"/.deltapath");
	if (!tmp.empty()) {
		_cmdOptions["deltapath"]=tmp[0];
	}
	tmp = ReadFileStrings(path+"/.oldpkgpath");
	if (!tmp.empty()) {
		_cmdOptions["oldpkgpath"]=tmp[0];
	}
	if (!dialogMode && verbose) printf("DELTAPATH: %s\nOLDPKGPATH: %s\n", _cmdOptions["deltapath"].c_str(), _cmdOptions["oldpkgpath"].c_str());

	// for the first, let's calculate the package count
	pkgcount_ftw=0;
	ftw(path.c_str(), countPackage,11);
	PACKAGE_LIST indexCachePkgList;
	__pkgCache = &indexCachePkgList;

	// Due to bug 1286, index caching now disabled. It will be re-enabled when this bug will be fixed.
	if (_cmdOptions["index_cache"]=="yes") {
		mWarning("Due to bug #1286, index caching disabled for now. Running non-cached indexing.");
		//importIndexCache(path);
	}
	
	
	ftw(path.c_str(), ProcessPackage, 11);
	// Finally, write our XML tree to file
	string xmlFile = path + "/packages.xml";
	xmlSaveFileEnc(xmlFile.c_str(), __doc, "UTF-8");
	if (_cmdOptions["pkglist_versioning"]=="true") {
		for (size_t i=0; i<pkgDupeVersions.size() && i<pkgDupeNames.size(); ++i) {
			pkgDupeNames[i] += " " + pkgDupeVersions[i];
		}
	}
	WriteFileStrings("package_list", pkgDupeNames);
	// Compress file
	//mDebug("Compressing files");
	if (!dialogMode) say(_("Compressing files\n"));
	if (system("gzip -f " + path+"/packages.xml")==0) {
		system("zcat " + path + "/packages.xml.gz | xz -e > " + path + "/packages.xml.xz");
	       if (!dialogMode) say(_("\n-------------SUMMARY------------------\nTotal: %d packages\n\nRepository index created successfully\n"),pkgcounter);
	       else ncInterface.showMsgBox(_("Repository index created\nTotal: ") + IntToStr(pkgcounter) + _(" packages"));
	}
	else mError(_("Error creating repository index!"));
	// Create setup_variants.list
	vector<string> svar_raw = getDirectoryList(path + "/setup_variants", NULL, true);
	vector<string> svar;
	for (size_t i=0; i<svar_raw.size(); ++i) {
		if (svar_raw[i].find(".desc")==svar_raw[i].size()-strlen(".desc")) svar.push_back(svar_raw[i].substr(0, svar_raw[i].size()-strlen(".desc")));
	}
	WriteFileStrings(path+"/setup_variants.list", svar);
	return 0;
}

// Add other such functions for other repository types.
int Repository::get_index(string server_url, PACKAGE_LIST *packages, unsigned int type) {
	if (_abortActions) return MPKGERROR_ABORTED;

	currentStatus = _("Updating data from ")+ server_url+"...";

	//mDebug("get_index!");
	// First: detecting repository type
	// Trying to download in this order (if successful, we have detected a repository type):
	// 1. packages.xml.xz or packages.xml.gz (Native MPKG)
	// 2. PACKAGES.TXT	(Legacy Slackware)
	// 3. Packages.gz	(Debian)
	// (and something else for RPM, in future)
	string index_filename = get_tmp_file();
	string md5sums_filename = get_tmp_file();
	string cm = "gunzip -f "+index_filename+".gz >/dev/null 2>/dev/null";
	bool mpkgDownloadOk = false;
	if (type == TYPE_MPKG || type == TYPE_AUTO) {
	       	if (CommonGetFile(server_url + "packages.xml.xz", index_filename+".xz")==DOWNLOAD_OK) {
		       cm = "xz -df " + index_filename + ".xz > /dev/null 2>/dev/null";
		       mpkgDownloadOk = true;
	       	}
		else if (_cmdOptions["cdrom_permanent_fail"]!="yes" && CommonGetFile(server_url + "packages.xml.gz", index_filename+".gz")==DOWNLOAD_OK) {
			mpkgDownloadOk = true;
		}
		if (mpkgDownloadOk) {
			if (system(cm.c_str())==0 && \
					ReadFile(index_filename).find("<?xml version=\"1.0\"")!=std::string::npos && ReadFile(index_filename).find("<repository")!=std::string::npos)
			{
				currentStatus = _("Detected native MPKG repository");
				type = TYPE_MPKG;
			}
		}
	}
	if (_abortActions) return MPKGERROR_ABORTED;

	if (type == TYPE_SLACK || type == TYPE_AUTO) {
		if (_cmdOptions["cdrom_permanent_fail"]!="yes" && CommonGetFile(server_url + "PACKAGES.TXT", index_filename)==DOWNLOAD_OK) {
			if (ReadFile(index_filename).find("PACKAGE NAME:  ")!=std::string::npos) {
				currentStatus = _("Detected legacy Slackware repository");
				if (_cmdOptions["cdrom_permanent_fail"]!="yes" && CommonGetFile(server_url + "CHECKSUMS.md5", md5sums_filename) == DOWNLOAD_OK) {
					type = TYPE_SLACK;
				}
				else {
					mError(_("Error downloading checksums"));
					return -1; // Download failed: no checksums or checksums download error
				}
			}
		}
	}
	if (_abortActions) return MPKGERROR_ABORTED;

	if (type != TYPE_MPKG && type != TYPE_SLACK) {
		currentStatus = _("Error updating data from ") +server_url+_(": download error or unsupported type");
		mError(server_url + string(": ") + _("Error downloading package index: download error, or unsupported repository type"));
		return -1;
	}

	PACKAGE *pkg = new PACKAGE;
	string xml_name=index_filename;
	
	xmlDocPtr indexDoc;
	xmlNodePtr indexRootNode;
	
	int ret=0;
	currentStatus = "["+server_url+_("] Importing data...");

	if (_abortActions) return MPKGERROR_ABORTED;

	string *pList;
	string *mList;
	string xmlData;
	size_t xmlStart;
	PACKAGE_LIST tempPkgList;

	switch(type) {
		case TYPE_MPKG:
			xmlData = ReadFile(xml_name.c_str());
			xmlStart = xmlData.find("<?xml");
			if (xmlStart==std::string::npos) {
				return -1;
			}
			if (xmlStart!=0) xmlData.erase(xmlData.begin(), xmlData.begin() + xmlStart);

			//indexDoc = xmlParseMemory(xmlData.substr(xmlStart).c_str(), xmlData.substr(xmlStart).size()); // FIXME: This takes GIANT amount of memory
			//indexDoc = xmlReadDoc((const xmlChar *) xmlData.c_str(), "", "UTF-8", XML_PARSE_COMPACT | XML_PARSE_NOBLANKS | XML_PARSE_NOXINCNODE | XML_PARSE_NONET);
			indexDoc = xmlReadDoc((const xmlChar *) xmlData.c_str(), "", "UTF-8", XML_PARSE_COMPACT | XML_PARSE_NOXINCNODE | XML_PARSE_NONET);
			xmlData.clear();


			if (indexDoc == NULL) {
				xmlFreeDoc(indexDoc);
				return -1;
			}

			indexRootNode = xmlDocGetRootElement(indexDoc);
			if (indexRootNode == NULL) {
				mError(_("Failed to get index"));
				xmlFreeDoc(indexDoc);
				return -1;
			}

			
			if (xmlStrcmp(indexRootNode->name, (const xmlChar *) "repository") ) {
				mError(_("Invalid index file"));
				xmlFreeDoc(indexDoc);
				return -1;
			}

			xml2pkglist(indexDoc, tempPkgList, server_url, package_descriptions, mConfig.getValue("force_offline_descriptions")=="yes");

			xmlFreeDoc(indexDoc);
			xmlCleanupMemory();
			xmlCleanupParser();
			
			for (size_t i=0; i<tempPkgList.size(); ++i) {
				// NEW: filter repository by architecture
				if (checkAcceptedArch(tempPkgList.get_package_ptr(i))) packages->add(tempPkgList[i]);
			}
			
			msay((string) CL_5 + _("Index update:") + (string) CL_WHITE +" ["+server_url+"]: " + (string) CL_GREEN + _("done") + (string) CL_WHITE + _(" (total: ") + IntToStr(tempPkgList.size()) + _(" packages, accepted: ") + IntToStr(packages->size()) + ")", SAYMODE_INLINE_END, stderr);

			tempPkgList.clear();
			break;

		case TYPE_SLACK:
			pList = new string;
			mList = new string;
			*pList = ReadFile(index_filename);
			*mList = ReadFile(md5sums_filename);

			ret = slackpackages2list(pList, mList, packages, server_url);
			if (pList!=NULL) delete pList;
			if (mList!=NULL) delete mList;
			break;

		default:
			break;
	}
	delete pkg;
	return ret;
}

