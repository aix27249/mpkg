/*
	MPKG packaging system
	Data types descriptions
	$Id: dataunits.cpp,v 1.82 2007/12/10 03:12:58 i27249 Exp $
*/



#include "dataunits.h"
#include "dependencies.h"
#include "conditions.h"
#include "debug.h"
//#include "constants.h"
#include <mpkgsupport/mpkgsupport.h>
int loopCount=0;
// LOCATION class functions

// Comparsion
bool LOCATION::equalTo(const LOCATION &location) const
{
	if (server_url!=location.get_server_url()) return false;
	if (location_path!=location.get_path()) return false;
	return true;
}

// Data retrieving

const int& LOCATION::get_id() const {
	return location_id;
}

int LOCATION::get_type() const {
	if (server_url.find("file://")==0) return SRV_FILE;
	if (server_url.find("local://")==0) return SRV_LOCAL;
	if (server_url.find("cdrom://")==0) return SRV_CDROM;
	return SRV_NETWORK;
}

const bool& LOCATION::get_local() const {
	return local;
}

const string& LOCATION::get_path() const {
	return location_path;
}

const string& LOCATION::get_server_url() const {
	return server_url;
}

const string LOCATION::get_full_url() const {
	string ret = server_url + location_path;
	strReplace(&ret, "/./", "/");
	return ret;
}

// Data writing
void LOCATION::set_id(const int& id)
{
	location_id=id;
}

void LOCATION::set_path(const string& path)
{
	if (path[path.length()-1]!='/') location_path=path + "/";
	else location_path=path;
}

void LOCATION::set_local() {
	local=true;
}

void LOCATION::unset_local() {
	local=false;
}

void LOCATION::set_server_url(const string& new_url) {
	server_url=new_url;
}

// Dimensions, empty, etc
bool LOCATION::IsEmpty() const {
	return location_path.empty();
}

void LOCATION::clear() {
	location_id=0;
	server_url.clear();
	location_path.clear();
	local=false;
}


// Constructor & destructor
LOCATION::LOCATION() {
	local=false;
	location_id=0;
}
LOCATION::~LOCATION() {
}


// DEPENDENCY class functions
void DEPENDENCY::setBuildOnly(const bool flag) {
	buildOnly = flag;
}
const bool& DEPENDENCY::isBuildOnly() const {
	return buildOnly;
}
// Comparsion
bool DEPENDENCY::equalTo(const DEPENDENCY& dependency) const {
	if (dependency_package_name!=dependency.get_package_name()) return false;
	if (version_data.version!=dependency.get_package_version()) return false;
	if (version_data.condition!=dependency.get_condition()) return false;
	if (dependency_type!=dependency.get_type()) return false;
	if (dependency_broken!=dependency.get_broken()) return false;
	return true;
}

// Data retrieving
const int& DEPENDENCY::get_id() const {
	return dependency_id;
}

const string& DEPENDENCY::get_condition() const {
	return version_data.condition;
}

const string& DEPENDENCY::get_type() const {
	return dependency_type;
}

const string& DEPENDENCY::get_package_name() const {
	return dependency_package_name;
}

const string& DEPENDENCY::get_package_version() const{
	return version_data.version;
}

const int& DEPENDENCY::get_broken() const {
	return dependency_broken;
}

const versionData& DEPENDENCY::get_version_data() const {
	return version_data;
}

// Visual data retriveal (UI purposes)
const string DEPENDENCY::get_vbroken() const {
	if (dependency_broken==DEP_FILECONFLICT) return "Conflicting files found";
	if (dependency_broken==DEP_DBERROR) return "Database error";
	if (dependency_broken==DEP_UNAVAILABLE) return "Package unavailable";
	if (dependency_broken==DEP_BROKEN) return "(deprecated_error) Dependency is broken, see sources for details";
	if (dependency_broken==DEP_CHILD) return "Child dependency is broken";
	if (dependency_broken==DEP_VERSION) return "Required version not found";
	if (dependency_broken==DEP_CONFLICT) return "Dependency conflict, unable to resolve";
	if (dependency_broken==DEP_NOTFOUND) return "Required package not found";
	if (dependency_broken==DEP_OK) return "OK";
	return "Dependency NOT SET!";
}

const string DEPENDENCY::get_vcondition() const {
	if (version_data.condition==IntToStr(VER_MORE)) return ">";
	if (version_data.condition==IntToStr(VER_LESS)) return "<";
	if (version_data.condition==IntToStr(VER_EQUAL)) return "==";
	if (version_data.condition==IntToStr(VER_NOTEQUAL)) return "!=";
	if (version_data.condition==IntToStr(VER_XMORE)) return ">=";
	if (version_data.condition==IntToStr(VER_XLESS)) return "<=";
	if (version_data.condition==IntToStr(VER_ANY)) return "(any)";
	string tmp = "(unknown condition " + version_data.condition + ")";
	return tmp;
}

const string DEPENDENCY::getDepInfo() const {
	if (version_data.condition!=IntToStr(VER_ANY)) return dependency_package_name + " " + get_vcondition() + " " + get_package_version();
	else return dependency_package_name;
};

// Data writing
void DEPENDENCY::set_id(const int& id) {
	dependency_id=id;
}

void DEPENDENCY::set_condition(const string& condition) {
	version_data.condition=condition;
}

void DEPENDENCY::set_type(const string& type) {
	dependency_type=type;
}

void DEPENDENCY::set_package_name(const string& package_name) {
	dependency_package_name=package_name;
}

void DEPENDENCY::set_package_version(const string& package_version) {
	version_data.version=package_version;
}

void DEPENDENCY::set_broken(const int& broken) {
	dependency_broken=broken;
}

// Emptyness, etc

bool DEPENDENCY::IsEmpty() const {
	return dependency_package_name.empty();
}

void DEPENDENCY::clear() {
	dependency_broken=0;
	version_data.version.clear();
	version_data.condition.clear();
	dependency_package_name.clear();
	dependency_type.clear();
	dependency_id=0;
}

// Constructor & destructor
DEPENDENCY::DEPENDENCY() {
	dependency_id=0;
	dependency_broken=0;
	buildOnly=false;
}
DEPENDENCY::~DEPENDENCY(){
}


// FILES class

// Comparsion
bool FILES::equalTo(const FILES& file) const {
	if (file_name!=file.get_name()) return false;
	else return true;
}

// Data retriveal
const int& FILES::get_id() const {
	return file_id;
}

const string& FILES::get_name() const {
	return file_name;
}

const int& FILES::get_type() const {
	return file_type;
}

const string& FILES::get_backup_file() const {
	return backup_file;
}

bool FILES::config() const {
	if (file_type==FTYPE_CONFIG) return true;
	else return false;
}


// Data writing
void FILES::set_id(const int& id) {
	file_id=id;
}

void FILES::set_name(const string& name) {
	file_name=name;
}

void FILES::set_type(const int& type) {
	file_type=type;
}

void FILES::set_backup_file(const string& fname) {
	backup_file=fname;
}

// Empty, clear
bool FILES::IsEmpty() const {
	return file_name.empty();
}

void FILES::clear() {
	file_id=0;
	file_type=0;
	file_name.clear();
	backup_file.clear();
}

// Constructor & destructor
FILES::FILES()
{
	file_id=0;
	file_type=FTYPE_PLAIN;
}

FILES::~FILES()
{
}
void _sortLocations(vector<LOCATION>& locations)
{
	// Sorting order:
	// 0: local://
	// 1: file://
	// 2: <all network>
	// 3: cdrom://

	vector<LOCATION> input = locations;
	locations.clear();

	for (int type_id=0; type_id<=3; type_id++){
		for (unsigned int i=0; i<input.size(); ++i) {
			if (input.at(i).get_type()==type_id) {
				locations.push_back(input.at(i));
			}
		}
	}
}
void PACKAGE::set_installed_by_dependency(const int& value) {
	package_installed_by_dependency = value;
}

const int& PACKAGE::get_installed_by_dependency() const {
	return package_installed_by_dependency;
}
const int& PACKAGE::get_type() const {
	return package_type;
}
void PACKAGE::set_type(const int& type) {
	package_type = type;
}

const vector<string> PACKAGE::get_repository_tags_vector() const {
	// Data format: blablabla tag 1 | blablabla tag2 | tag3
	// spaces on edges are omitted
	string tmp = cutSpaces(package_repository_tags);
	vector<string> ret;
	string tmp2;
	while (tmp.find("|")!=std::string::npos) {
		if (tmp.find("|")==tmp.size()-1) {
			tmp.clear();
			break;
		}
		if (tmp.find("|")==0) {
			tmp = cutSpaces(tmp.substr(1));
			continue;
		}
		ret.push_back(cutSpaces(tmp.substr(0, tmp.find("|"))));
		tmp = cutSpaces(tmp.substr(tmp.find("|")+1));
	}
	return ret;
}
const string & PACKAGE::get_repository_tags() const {
	return package_repository_tags;
}
void PACKAGE::set_repository_tags(const string& rtags) {
	package_repository_tags = rtags;
}
void PACKAGE::sortLocations() {
	_sortLocations(package_locations);
}

bool PACKAGE::isTaggedBy(const string& tag) const {
	for (unsigned int i=0; i<package_tags.size(); i++) {
		if (package_tags[i]==tag) return true;
	}
	return false;
}

// Comparsion
bool PACKAGE::equalTo (const PACKAGE& npkg) const
{
	if (package_md5!=npkg.get_md5()) return false;
	return true;
}
bool PACKAGE::locationsEqualTo(const PACKAGE& pkg) const {
	if (package_locations.size()!=pkg.get_locations().size()) return false;
	for (unsigned int i=0; i<package_locations.size(); i++)
	{
		for (unsigned int j=0; j<pkg.get_locations().size(); j++)
		{
			if (!package_locations[i].equalTo(pkg.get_locations().at(j)))
				return false;
		}
	}
	return true;
}
bool PACKAGE::tagsEqualTo(const PACKAGE& pkg) const {
	if (package_tags.size()!=pkg.get_tags().size()) return false;
	for (unsigned int i=0; i<package_tags.size(); ++i)
	{
		for (unsigned int j=0; j<pkg.get_tags().size(); j++)
		{
			if (package_tags[i]!=pkg.get_tags().at(j)) return false;
		}
	}
	return true;
}

bool PACKAGE::depsEqualTo(const PACKAGE& pkg) const {
	if (package_dependencies.size()!=pkg.get_dependencies().size()) return false;
	for (unsigned int i=0; i<package_tags.size(); ++i) {
		for (unsigned int j=0; j<pkg.get_dependencies().size(); ++j)
		{
			if (!package_dependencies[i].equalTo(pkg.get_dependencies().at(j))) return false;
		}
	}
	return true;
}

// Data retriveal
bool PACKAGE::isItRequired(const PACKAGE& testPackage) const	// Tests if a testPackage is required by this and meets the dependency requirements
{
	for (unsigned int i=0; i<package_dependencies.size(); i++)
	{
		if (package_dependencies[i].get_package_name() == testPackage.get_name() && \
				meetVersion(package_dependencies[i].get_version_data(), testPackage.get_version()))
			return true;
	}
	return false;
}

bool PACKAGE::isUpdate() const {
	if (!installed() && hasMaxVersion && !installedVersion.empty()) {
	       if (installedVersion != maxVersion || installedBuild!=maxBuild) {
			return true;
	       }
	}
	return false;
}

bool PACKAGE::deprecated() const {
	return !hasMaxVersion;
}
const string& PACKAGE::get_provides() const {
	//printf("returning provides %s\n", provides.c_str());
	return provides;
}

const string& PACKAGE::get_conflicts() const {
	return conflicts;
}

const int& PACKAGE::get_id() const {
	return package_id;
}

const string& PACKAGE::get_name() const {
	return package_name;
}
const string& PACKAGE::get_corename() const {
	if (provides.empty()) return package_name;
	else return provides;
}
const string& PACKAGE::get_version() const {
	return package_version;
}

const string& PACKAGE::get_arch() const {
	return package_arch;
}

const string& PACKAGE::get_build() const {
	return package_build;
}

const string& PACKAGE::get_compressed_size() const {
	return package_compressed_size;
}

const string& PACKAGE::get_installed_size() const {
	return package_installed_size;
}

const string& PACKAGE::get_short_description() const {
	return package_short_description;
}

const string& PACKAGE::get_description() const {
	return package_description;
}

const string& PACKAGE::get_changelog() const {
	return package_changelog;
}

const string& PACKAGE::get_packager() const {
	return package_packager;
}

const string& PACKAGE::get_packager_email() const {
	return package_packager_email;
}

bool PACKAGE::available(bool includeLocal) const {
	if (package_locations.empty()) return false;
	else
	{
		if (includeLocal) return true;
		for (unsigned int i=0; i<package_locations.size(); i++)
		{
			if (package_locations[i].get_type()!=SRV_LOCAL)
			{
				return true;
			}
		}
	}
	return false;
}
bool PACKAGE::reachable(bool includeConfigFiles) const {
	if (includeConfigFiles) {
		if (available(true) || installed() || configexist()) return true;
		else return false;
	}
	else {
		if (available(true) || installed()) return true;
		else return false;
	}
}
bool PACKAGE::installed() const {
	return package_installed;
}

bool PACKAGE::configexist() const {
	return package_configexist;
}

int PACKAGE::action() const {
	return package_action;
}
const string PACKAGE::get_vstatus(bool color) const {
	string stat;
	if (available())
	{
		if (color) stat+=CL_6;
		stat +="A";
		if (color) stat+=CL_WHITE;
	}
	else stat+="_";

	if (installed()) 
	{
		if (color) stat+=CL_GREEN;
		stat += "I";
		if (color) stat+=CL_WHITE;
	}
	else {
		stat += "_";
	}
	if (configexist())
	{
		if (color) stat+=CL_BLUE;
		stat += "C";
		if (color) stat+=CL_WHITE;
	}
	else stat+="_";
	
	switch(action())
	{
		case ST_INSTALL:
			if (color) stat+=CL_YELLOW;
			stat+="i";
			if (color) stat+=CL_WHITE;
			break;
		case ST_REMOVE:
			if (color) stat+=CL_RED;
			stat+="r";
			if (color) stat+=CL_WHITE;
			break;
		case ST_PURGE:
			if (color) stat+=CL_8;
			stat+="p";
			if (color) stat+=CL_WHITE;
			break;
		case ST_UPDATE:
			if (color) stat+=CL_5;
			stat+="u";
			if (color) stat += CL_WHITE;
			break;
		case ST_REPAIR:
			if (color) stat+=CL_7;
			stat+="R";
			if (color) stat+=CL_WHITE;
			break;
		case ST_NONE:
			stat+="_";
			break;
		default:
			stat+="?";
			break;
	}
	return stat;
}

const string& PACKAGE::get_md5() const {
	return package_md5;
}
const string& PACKAGE::get_filename() const {
	return package_filename;
}
const string& PACKAGE::get_betarelease() const {
	return package_betarelease;
}

bool PACKAGE::isBeta() const {
	if (package_betarelease=="0" || package_betarelease.empty()) return false;
	else return true;
}
int PACKAGE::get_err_type() const {
	return package_err_type;
}

void PACKAGE::set_err_type(const int& err) {
	package_err_type=err;
}


void PACKAGE::set_broken(bool flag)
{
	isBroken=flag;
}
void PACKAGE::set_requiredVersion(const versionData& reqVersion) {
	requiredVersion = reqVersion;
}

const string PACKAGE::get_fullversion() const {
	return get_version() + "-" + get_build();
}

string PACKAGE::getAlternative() const {
	if (provides.empty()) return "";
	if (provides==package_name) return "";
	if (package_name.find(provides)!=0) return "";
	if (package_name.size()<provides.size()+2) return "";
	return package_name.substr(provides.length()+1);
}

vector<string> PACKAGE::getAlternativeVector() const {
	string alt = getAlternative();
	vector<string> ret = splitString(alt, "-");
	return ret;
}

bool PACKAGE::checkAlternative(const string& alt) const {
	vector<string> ch = getAlternativeVector();
	for (size_t i=0; i<ch.size(); ++i) {
		if (ch[i]==alt) return true;
	}
	return false;
}

void PACKAGE::clear()
{
	package_id=-1;
	package_name.clear();
	package_version.clear();
	package_arch.clear();
	package_build.clear();
	package_compressed_size.clear();
	package_installed_size.clear();
	package_short_description.clear();
	package_description.clear();
	package_changelog.clear();
	package_packager.clear();
	package_packager_email.clear();
	package_installed=0;
	package_configexist=0;
	package_action=0;
	package_md5.clear();
	package_filename.clear();
	package_files.clear();
	package_locations.clear();
	package_dependencies.clear();
	package_tags.clear();
	provides.clear();
	conflicts.clear();
}

void PACKAGE::add_dependency(const string& package_name, const string& dep_condition, const string& package_version) {
	DEPENDENCY dep;
	dep.set_package_name(package_name);
	dep.set_package_version(package_version);
	dep.set_condition(dep_condition);
	for (unsigned int i=0;i<package_dependencies.size();i++)
	{
		if (package_dependencies.at(i).get_package_name()==dep.get_package_name() && \
		    package_dependencies.at(i).get_package_version()==dep.get_package_version())
		       	return;
	}
	package_dependencies.push_back(dep);
}

void PACKAGE::add_file(const string& file_name) {
	FILES file;
	file.set_name(file_name);
	package_files.push_back(file);
}

void PACKAGE::add_tag(const string& tag) {
	package_tags.push_back(tag);
}

void PACKAGE::set_provides(const string& _provides) {
	if (_provides!="0") provides = _provides;
	else provides="";
}

void PACKAGE::set_conflicts(const string& _conflicts) {
	if (_conflicts!="0") conflicts = _conflicts;
	else conflicts="";
}
void PACKAGE::set_id(const int& id) {
	package_id=id;
}

void PACKAGE::set_name(const string& name) {
	package_name=name;
}

void PACKAGE::set_version(const string& version) {
	package_version=version;
}

void PACKAGE::set_arch(const string& arch) {
	package_arch=arch;
}

void PACKAGE::set_build(const string& build) {
	package_build=build;
}

void PACKAGE::set_compressed_size(const string& compressed_size) {
	package_compressed_size=compressed_size;
}

void PACKAGE::set_installed_size(const string& installed_size) {
	package_installed_size=installed_size;
}

void PACKAGE::set_short_description(const string& short_description) {
	package_short_description=short_description;
}

void PACKAGE::set_description(const string& description) {
	package_description=description;
}

void PACKAGE::set_changelog(const string& changelog) {
	package_changelog=changelog;
}

void PACKAGE::set_packager(const string& packager) {
	package_packager=packager;
}

void PACKAGE::set_packager_email(const string& packager_email) {
	package_packager_email=packager_email;
}

void PACKAGE::set_installed(const bool flag) {
	package_installed = flag;
}

void PACKAGE::set_configexist(bool flag)
{
	package_configexist = flag;
}

void PACKAGE::set_action(const int& new_action, string reason) {
	package_action = new_action;
	package_action_reason = reason;
}

void PACKAGE::set_md5(const string& md5) {
	package_md5=md5;
}

void PACKAGE::set_filename(const string& filename) {
	package_filename=filename;
}

void PACKAGE::set_betarelease(const string& betarelease) {
	package_betarelease=betarelease;
}

#ifdef ENABLE_INTERNATIONAL
vector<DESCRIPTION>* PACKAGE::get_descriptions()
{
	return &package_descriptions;
}

int PACKAGE::set_descriptions(vector<DESCRIPTION>* desclist)
{
	package_descriptions=*desclist;
}
#endif

vector<FILES>* PACKAGE::get_files_ptr() {
	return &package_files;
}
const vector<FILES>& PACKAGE::get_files() const {
	return package_files;
}

const vector<FILES>& PACKAGE::get_config_files() const {
	return config_files;
}

vector<FILES>* PACKAGE::get_config_files_ptr() {
	return &config_files;
}
const vector<FILES>& PACKAGE::get_temp_files() const {
	return temp_files;
}

vector<DEPENDENCY>* PACKAGE::get_dependencies_ptr() {
	return &package_dependencies;
}
const vector<DEPENDENCY>& PACKAGE::get_dependencies() const {
	return package_dependencies;
}

vector<LOCATION>* PACKAGE::get_locations_ptr() {
	return &package_locations;
}
const vector<LOCATION>& PACKAGE::get_locations() const {
	return package_locations;
}

vector<string>* PACKAGE::get_tags_ptr() {
	return &package_tags;
}

const vector<string>& PACKAGE::get_tags() const {
	return package_tags;
}

const string PACKAGE::get_scriptdir() const {
	return SCRIPTS_DIR + package_filename + "_" + package_md5 + "/";
}

void PACKAGE::set_files(const vector<FILES>& files) {
	package_files=files;
}

void PACKAGE::set_config_files(const vector<FILES>& conf_files) {
	config_files=conf_files;
}

void PACKAGE::sync()
{
	for (unsigned int i=0; i< config_files.size(); i++)
	{
		for (unsigned int t=0; t<package_files.size(); t++)
		{
			if (config_files[i].get_name()=='/' + package_files[t].get_name())
			{
				package_files[t].set_type(FTYPE_CONFIG);
				break;
			}
		}
	}

	if (config_files.empty())
	{
		for (unsigned int i=0; i < package_files.size(); i++)
		{
			if (package_files[i].get_type()==FTYPE_CONFIG)
			{
				config_files.push_back(package_files[i]);
			}
		}
	}
	for (unsigned int i=0; i< temp_files.size(); i++)
	{
		for (unsigned int t=0; t<package_files.size(); t++)
		{
			if (temp_files[i].get_name()=='/' + package_files[t].get_name())
			{
				package_files[t].set_type(FTYPE_TEMP);
				break;
			}
		}
	}

	if (temp_files.empty())
	{
		for (unsigned int i=0; i < package_files.size(); i++)
		{
			if (package_files[i].get_type()==FTYPE_TEMP)
			{
				temp_files.push_back(package_files[i]);
			}
		}
	}

}

void PACKAGE::set_dependencies(const vector<DEPENDENCY>& dependencies) {
	package_dependencies=dependencies;
}

void PACKAGE::set_locations(const vector<LOCATION>& locations) {
	package_locations=locations;
}

void PACKAGE::set_tags(const vector<string>& tags) {
	package_tags=tags;
}

bool PACKAGE::IsEmpty() const {
	return package_name.empty();
}

bool PACKAGE::isRemoveBlacklisted() const {
	for (unsigned int i=0; i<removeBlacklist.size(); i++) {
		if (removeBlacklist[i]==this->package_name) return true;
	}
	return false;
}


PACKAGE::PACKAGE()
{
	needSpecialUpdate = false;
	updatingBy=NULL;
	isBroken = false;
	isRequirement = false;
	package_id=-1;
	package_installed=false;
	package_configexist=false;
	package_action=ST_NONE;
	newPackage = false;
	package_err_type=DEP_OK;
	package_type = PKGTYPE_BINARY;
	package_installed_by_dependency = 0;
}
PACKAGE::~PACKAGE()
{
}
/*bool PACKAGE::operator == (PACKAGE pkg) {
	return package_name == *pkg.get_name();
}
bool PACKAGE::operator != (PACKAGE pkg) {
	return package_name != *pkg.get_name();
}
bool PACKAGE::operator >= (PACKAGE pkg) {
	return package_name >= *pkg.get_name();
}
bool PACKAGE::operator <= (PACKAGE pkg) {
	return package_name <= *pkg.get_name();
}
bool PACKAGE::operator > (PACKAGE pkg) {
	return package_name > *pkg.get_name();
}
bool PACKAGE::operator < (PACKAGE pkg) {
	return package_name < *pkg.get_name();
}
*/
void PACKAGE::clearVersioning()
{
	hasMaxVersion=false;
	maxVersion.clear();
	maxBuild.clear();
	installedVersion.clear();
	installedBuild.clear();
	alternateVersions.clear();
}
vector<PACKAGE *> PACKAGE_LIST::getAllAlternatives(const string& corename) {
	vector<PACKAGE *> ret;
	for (size_t i=0; i<packages.size(); ++i) {
		if (packages[i].get_corename()==corename) ret.push_back(&packages[i]);
	}
	return ret;
}

// Returns count of strings from base that WAS NOT FOUND in alts
int getUnusedCount(const vector<string> &base, const vector<string> &alts) {
	int ret=0;
	bool found;
	for (size_t i=0; i<base.size(); ++i) {
		found = false;
		for (size_t t=0; !found && t<alts.size(); ++t) {
			if (base[i]==alts[t]) {
				found = true;
			}
		}
		if (!found) ret++;
	}
	return ret;
}

void PACKAGE_LIST::switchAlternatives(const vector<string>& alternatives) {
	// ----------------------------------VARIABLES SECTION------------------------------------- //
	vector<PACKAGE *> altPackages;
	vector<size_t> meetCount;
	vector<PACKAGE *> maxMeeters;
	PACKAGE * selectedPackage;
	size_t max_meet, min_unused, this_meet, this_unused;
	vector<string> log;

	// ----------------------------Init log by initial conditions------------------------------ //
	for (size_t i=0; i<alternatives.size(); ++i) {
		log.push_back("REQUESTED: " + alternatives[i]);
	}

	// ----------------------------------------RESET--------------------------------------------//

	// First of all: reset to initial state.
	// By i, we search for alternated packages. By t, we look for appropriate core packages, and move action to them
	for (size_t i=0; i<packages.size(); ++i) {
		if (packages[i].get_name()==packages[i].get_corename()) continue;
		if (packages[i].action()!=ST_INSTALL) continue; // Found alt who wants to be installed, looking for core one
		for (size_t t=0; t<packages.size(); ++t) {
			if (i==t) continue;
			if (packages[t].get_corename()==packages[i].get_corename() && packages[t].get_name()==packages[t].get_corename()) {
				packages[i].set_action(ST_NONE, "switchAlt reset"); // Alt goes away
				packages[t].set_action(ST_INSTALL, "switchAlt reset"); // Core one goes to queue
			}
		}
	} // End of reset.

	// --------------------------------PROCESSING--------------------------------------------//

	// Now, for every package who wants to be installed, look for alter ones and choose best from them.
	// Note that now i and t is opposite than prior case: i is core, t is alt.
	for (size_t i=0; i<packages.size(); ++i) {
		if (packages[i].action()!=ST_INSTALL) continue; // Filter no-action packages for i
		if (packages[i].get_name()!=packages[i].get_corename()) continue; // Filter non-core packages for i
		// First, filling the altPackages vector
		altPackages.clear();
		meetCount.clear();
		maxMeeters.clear();
		selectedPackage=NULL;
		for (size_t t=0; t<packages.size(); ++t) {
			if (i==t) continue;
			if (packages[t].get_corename()==packages[i].get_corename()) altPackages.push_back(&packages[t]);
		}
		// Now, let's choose the best candidate from altPackages, comparing with alternatives
		// Priorities:
		// 1. The more alternatives package meet that's better

		// 1.1 Finding maximum and filling meetCount with exact count for each package
		max_meet=0;
		for (size_t t=0; t<altPackages.size(); ++t) {
			this_meet=0;
			for (size_t a=0; a<alternatives.size(); ++a) {
				this_meet+=altPackages[t]->checkAlternative(alternatives[a]);
			}
			meetCount.push_back(this_meet);
			if (this_meet>max_meet) max_meet=this_meet;
		}
		// 1.1.1 if no one meets, continue
		if (max_meet==0) continue;
		// 1.2 Filling maxMeeters vector with maximum ones
		for (size_t t=0; t<altPackages.size(); ++t) {
			if (max_meet==meetCount[t]) maxMeeters.push_back(altPackages[t]);
		}
		// 1.3 If we found nothing, there is nothing to select.
		if (maxMeeters.empty()) continue;

		// 2. From group of maximum-meeted ones (maxMeeters vector), select one who contains minimum of other alt flags (preferably none)
		// 2.1 First maxMeeter will be start point and first selected candidate. 
		min_unused=getUnusedCount(maxMeeters[0]->getAlternativeVector(), alternatives);
		selectedPackage=maxMeeters[0];

		// 2.2 If someone will be better than 0th one, it will replace him, changing min_unused value. When min_unused value reaches zero, break.
		for (size_t t=1; t<maxMeeters.size(); ++t) {
			this_unused=getUnusedCount(maxMeeters[t]->getAlternativeVector(), alternatives);
			if (min_unused>this_unused) {
				min_unused=this_unused;
				selectedPackage=maxMeeters[t];
			}
			
			if (min_unused==0) break;
		}

		// 3. Check if we found alternative
		if (!selectedPackage) continue;
		
		// 4. If we found someone, swap actions.
		packages[i].set_action(ST_NONE, "switchAlt swap");
		selectedPackage->set_action(ST_INSTALL, "switchAlt swap");

		// Finally, store this swap to log
		log.push_back("SWITCH: " + packages[i].get_name() + " " + packages[i].get_fullversion() + " to " + selectedPackage->get_name() + " " + selectedPackage->get_fullversion() + ", max_meet=" + IntToStr(max_meet) + ", min_unused=" + IntToStr(min_unused));
	}

	WriteFileStrings("/var/log/alternatives.log", log);
	
}

vector<string> PACKAGE_LIST::getAlternativeList(bool separate) const {
	vector<string> ret;
	string alt;
	vector<string> altv;
	bool found;
	for (size_t i=0; i<packages.size(); ++i) {
		alt = packages[i].getAlternative();
		if (alt.empty()) continue;
		
		found = false;
		if (separate) {
			altv = splitString(alt, "-");
			for (size_t v=0; v<altv.size(); ++v) {
				found = false;
				for (size_t z=0; z<ret.size(); ++z) {
					if (ret[z]==altv[v]) {
						found = true;
						break;
					}
				}
				if (!found) ret.push_back(altv[v]);
			}
		}
		else {
			for (size_t z=0; z<ret.size(); ++z) {
				if (ret[z]==alt) {
					found = true;
					break;
				}
			}
			if (!found) ret.push_back(alt);
		}
	}
	return ret;

}

PACKAGE * PACKAGE_LIST::getPackageByIDPtr(const int& id, bool quiet) {
	for (unsigned int i=0; i<packages.size(); i++)
	{
		if (packages[i].get_id()==id) return &packages[i];
	}
	if (!quiet) mError("No such id " + IntToStr(id) + " in package list");
	return NULL;
}


const PACKAGE& PACKAGE_LIST::getPackageByID(const int& id) const {
	for (unsigned int i=0; i<packages.size(); i++) {
		if (packages[i].get_id()==id) return packages[i];
	}
	mError("No such id " + IntToStr(id) + " in package list, aborting");
	abort(); // TODO: check conditions which may apply to this case. UPD: these conditions exist... :(
}

bool checkDependencyResolvable(PACKAGE *package, vector< vector <PACKAGE *> > *matrix, vector<PACKAGE> *checkList) {
	vector<bool> resolved;
	bool resolvable = false;
	for (unsigned int i=0; i<package->get_dependencies().size(); i++) {
		resolved.push_back(false);
		resolvable = false;
		if (checkList!=NULL) {
			for (unsigned int d=0; d<checkList->size(); d++) {
				if (checkList->at(d).get_name() == package->get_dependencies().at(i).get_package_name()) {
					resolvable=true;
					break;
				}
			}
			if (!resolvable) resolved[i]=true;
		}
		for (unsigned int x=0; !resolved[i] && x<matrix->size(); x++) {
			for (unsigned int y=0; !resolved[i] && y<matrix->at(x).size(); y++) {
				// Проверяем i-ю зависимость на удовлетворение пакетом из matrix[x][y]
				if (package->get_dependencies().at(i).get_package_name()==matrix->at(x).at(y)->get_name()) {
					if (meetVersion(package->get_dependencies().at(i).get_version_data(), matrix->at(x).at(y)->get_version())) {
						// Подходит
						resolved[i]=true;
					}
				}
			}
		}
	}
	for (unsigned int i=0; i<resolved.size(); i++) {
		if (!resolved[i]) return false;
	}
	return true;
}
void PACKAGE_LIST::sortByPriorityNew(const bool& reverse_order) {
	vector<PACKAGE *> currentRing;
	vector<PACKAGE> sortedList;
	vector< vector <PACKAGE *> > matrix;
	unsigned int packages_calculated=0, prev_packages_calculated=0;
	bool skip=false;
	// Step 1. Заполнение нулевого кольца: пакеты без зависимостей
	for (unsigned int i=0; i<packages.size(); i++) {
		if (packages[i].get_dependencies().size()==0) {
			currentRing.push_back(&packages[i]);
			packages_calculated++;
		}
	}
	matrix.push_back(currentRing);

	// Step 2. Заполнение очередного кольца, зависимости которого могут быть удовлетворены предыдущими кольцами
	// Причем, пакеты в данном кольце не должны быть в каких либо кольцах еще.
	while (packages_calculated!=prev_packages_calculated && packages_calculated != packages.size()) {
		prev_packages_calculated = packages_calculated;
		currentRing.clear();
		for (unsigned int i=0; i<packages.size(); i++) {
			skip=false;
			// Проверяем, есть ли данный пакет уже в предыдущих кольцах
			for (unsigned int x=0; !skip && x<matrix.size(); x++) {
				for (unsigned int y=0; !skip && y<matrix[x].size(); y++) {
					if (matrix[x][y]==&packages[i]) {
						skip=true;
					}
				}
			}
			if (!skip) {
				// Если пакета еще нету в кольцах, проверяем его зависимости
				if (checkDependencyResolvable(&packages[i], &matrix, &packages)) {
					currentRing.push_back(&packages[i]);
					packages_calculated++;
				};
			}
		}
		matrix.push_back(currentRing);
	}
	currentRing.clear();
	// Проверяем, чего осталось
	
	if (packages_calculated != packages.size()) {
		// Если так, то у нас - кольцевые зависимости. Кидаем эти пакеты в конец списка не сортируя, ибо ничего с ними не поделаешь...
		for (unsigned int i=0; i<packages.size(); i++) {
			skip=false;
			// Проверяем, есть ли данный пакет уже в предыдущих кольцах
			for (unsigned int x=0; !skip && x<matrix.size(); x++) {
				for (unsigned int y=0; !skip && y<matrix[x].size(); y++) {
					if (matrix[x][y]==&packages[i]) {
						skip=true;
					}
				}
			}
			if (!skip) {
				currentRing.push_back(&packages[i]);
			}
		}
		matrix.push_back(currentRing);
	}
	// Теперь мы получили вектор колец, иными словами - матрицу пакетов.
	// Создаем теперь сортированный список.
	// Помним о том, что нулевое кольцо нам не нужно.
	if (reverse_order) {
		for (unsigned int x=matrix.size(); x>0; x--) {
			for (unsigned int y=0; y<matrix[x-1].size(); y++) {
				sortedList.push_back(*matrix[x-1][y]);
			}
		}

	}
	else {
		for (unsigned int x=0; x<matrix.size(); x++) {
			for (unsigned int y=0; y<matrix[x].size(); y++) {
				sortedList.push_back(*matrix[x][y]);
			}
		}
	}
	if (sortedList.size()!=packages.size()) {
		printf("WARNING! Size mismatch!\n");
	}
	packages = sortedList;
	sortedList.clear();
	matrix.clear();
}
void checkTag(string tag, vector<string> * tags, vector <string> *sortedTags) {
	for (unsigned int i=0; i<tags->size(); i++) { 
		// Вторым делом - libs
		if (tags->at(i)==tag) {
			sortedTags->push_back(tags->at(i));
			return;
		}
	}
}
struct comparepkgptr {
	bool operator() (PACKAGE * a, PACKAGE * b) {return a->get_name() < b->get_name(); }
} comparePackagePtr;

void PACKAGE_LIST::sortByTags(const bool& reverse_order) {
	vector< vector<PACKAGE *> > matrix; // Матрица пакетов
	vector<PACKAGE *> currentRing;
	vector<string> tags, sortedTags; // Имеющиеся в списке теги
	bool tagNotInList = true;
	// Шаг 1: заполнение списка имеющихся тегов
	for (unsigned int i=0; i<packages.size(); i++) {
		for (unsigned int t=0; t<packages[i].get_tags().size(); t++) {
			tagNotInList=true;
			for (unsigned int a=0; a<tags.size(); a++) {
				if (packages[i].get_tags().at(t)==tags[a]) {
					tagNotInList=false;
					break;
				}
			}
			if (tagNotInList) tags.push_back(packages[i].get_tags().at(t));
		}
	} // Конец получения тегов
	// Сортируем теги по алфавиту
	sort(tags.begin(), tags.end());

	// Шаг 2. Сортируем теги как надо. Сначала - ручной порядок:
	checkTag("base", &tags, &sortedTags);
	checkTag("base-utils", &tags, &sortedTags);
	checkTag("console", &tags, &sortedTags);
	checkTag("skel", &tags, &sortedTags);
	checkTag("libs", &tags, &sortedTags);
	checkTag("library", &tags, &sortedTags);
	checkTag("utils", &tags, &sortedTags);
	checkTag("devel", &tags, &sortedTags);
	checkTag("network", &tags, &sortedTags);
	checkTag("x11-skel", &tags, &sortedTags);
	checkTag("x11-base", &tags, &sortedTags);
	checkTag("x11", &tags, &sortedTags);
	checkTag("xserver", &tags, &sortedTags);
	checkTag("kde", &tags, &sortedTags);
	checkTag("kde4", &tags, &sortedTags);
	checkTag("xapps", &tags, &sortedTags);
	checkTag("koffice", &tags, &sortedTags);
	checkTag("openoffice", &tags, &sortedTags);
	checkTag("server", &tags, &sortedTags);
	checkTag("tcl", &tags, &sortedTags);
	checkTag("school", &tags, &sortedTags);
	checkTag("kernel-source", &tags, &sortedTags);

	// Оставшиеся теги добавляем в конец в алфавитном порядке
	bool already;
	for (unsigned int i=0; i<tags.size(); i++) {
		already=false;
		if (tags[i]=="mopscripts") continue; // этот тег пойдет последним, даже после пакетов без тегов вообще.
		for (unsigned int s=0; s<sortedTags.size(); s++) {
			if (tags[i]==sortedTags[s]) {
				already=true;
				break;
			}
		}
		if (!already) sortedTags.push_back(tags[i]);
	}
	tags = sortedTags;
	sortedTags.clear();

	// Сортируем пакеты по тегам
	for (unsigned int i=0; i<tags.size(); i++) {
		currentRing.clear();
		for (unsigned int t=0; t<packages.size(); t++) {
			if (packages[t].isTaggedBy(tags[i])) {
				// Проверяем, а не занесли ли его в матрицу уже?
				already = false;
				for (unsigned int x=0; !already && x<matrix.size(); x++) {
					for (unsigned int y=0; !already && y<matrix[x].size(); y++) {
						if (matrix[x][y]==&packages[t]) {
							already = true;
						}
					}
				}
				if (!already) currentRing.push_back(&packages[t]);
			}
		}
		if (!currentRing.empty()) matrix.push_back(currentRing);
	}
	currentRing.clear();
	// Итак, остались лишь пакеты, не имеющие тегов вообще. Добавим их:
	for (unsigned int i=0; i<packages.size(); i++) {
		if (packages[i].get_tags().empty()) {
			currentRing.push_back(&packages[i]);
		}
	}
	if (!currentRing.empty()) {
		matrix.push_back(currentRing);
		currentRing.clear();
	}
	// Ищем если есть пакеты с тегом mopscripts. Он должен всегда ставиться последним.
	for (unsigned int i=0; i<packages.size(); i++) {
		if (packages[i].isTaggedBy("mopscripts")) {
			already = false;
			for (unsigned int x=0; !already && x<matrix.size(); x++) {
				for (unsigned int y=0; !already && y<matrix[x].size(); y++) {
					if (matrix[x][y]==&packages[i]) {
						already = true;
					}
				}
			}
			if (!already) currentRing.push_back(&packages[i]);
		}
	}
	if (!currentRing.empty()) {
		matrix.push_back(currentRing);
		currentRing.clear();
	}


	// И вот наконец-то мы имеем матрицу пакетов. Задача: 1) отсортировать каждое кольцо по алфавиту, 2) склеить кольца в правильном порядке и занести обратно в packages
	vector<PACKAGE> sortedList;
	for (unsigned int x=0; x<matrix.size(); x++) {
		sort(matrix[x].begin(), matrix[x].end(), comparePackagePtr);
		for (unsigned int y=0; y<matrix[x].size(); y++) {
			sortedList.push_back(*matrix[x][y]);
		}
	}
	if (reverse_order) reverse(sortedList.begin(), sortedList.end());
	packages = sortedList;
	sortedList.clear();
}

void PACKAGE_LIST::sortByPriority(const bool& reverse_order) {	
	if (!priorityInitialized) buildDependencyOrder();
	int min_priority = 0;
	for (unsigned int i=0; i<packages.size(); i++)
	{
		if (packages[i].priority>min_priority) min_priority = packages[i].priority;
	}

	vector<PACKAGE> sorted;
	if (!reverse_order)
	{	
		for (int p=0; p<=min_priority; p++)
		{
			for (unsigned int i=0; i<packages.size(); i++)
			{
				if (packages[i].priority==p) sorted.push_back(packages[i]);
			}
		}
	}
	else
	{
		for (int p=min_priority; p>=0; p--)
		{
			for (unsigned int i=0; i<packages.size(); i++)
			{
				if (packages[i].priority==p) sorted.push_back(packages[i]);
			}
		}
	}
	packages = sorted;

}
void PACKAGE_LIST::sortByLocationTypes()
{
	// Step 1) sorting each package locations
	// Step 2) sorting packages by first location
	
	vector<PACKAGE> input = packages;
	packages.clear();
	// Processing step 1
	for (unsigned int i=0; i<input.size(); i++)
	{
		_sortLocations(*input.at(i).get_locations_ptr());
	}

	// Processing step 2
	for (int type_id=0; type_id<=4; type_id++)
	{
		for (unsigned int i=0; i<input.size(); i++) {
			if (input.at(i).get_locations().size()!=0) {
				if (input.at(i).get_locations().at(0).get_type()==type_id) packages.push_back(input.at(i));
			} else if (type_id==4) packages.push_back(input.at(i));
		}
	}
	input.clear();
}
void ttylog(string data) {
	system("echo " + data + " >/dev/tty4");
}
void PACKAGE_LIST::sortByLocations() {
//	ttylog("SORT_BY_LOCATIONS");
	sortByLocationTypes(); // Sort by types
//	ttylog("TYPES: DONE");
	// Sort list by default locations
	vector<string> locations;
	bool skip;
	// Creating a list of available locations
	for (unsigned int i=0; i<packages.size(); i++) {
		skip=false;
		if (packages[i].get_locations().empty()) continue; // Skip packages with no locations
		for (unsigned int t=0; t<locations.size(); t++) {
			if (locations[t]==packages[i].get_locations().at(0).get_server_url()) {
				skip=true;
				break;
			}
		}
		if (!skip) locations.push_back(packages[i].get_locations().at(0).get_server_url());
	}
//	ttylog("LOCATIONLIST: DONE");
	// Recreating a list of packages
	vector<PACKAGE> p;
	for (unsigned int t=0; t<=locations.size(); t++) { // t<=locations.size() is real need thing: we will use last t for packages with no locations
		for (unsigned int i=0; i<packages.size(); i++) {
			if (packages[i].get_locations().empty()) {
				if (t==locations.size()) p.push_back(packages[i]); // If package has no locations and we reached the last t, add it
			}
			else if (t<locations.size()) {
				if (packages[i].get_locations().at(0).get_server_url()==locations[t]) p.push_back(packages[i]);
			}
		}
	}
	packages = p;
//	ttylog("SORT DONE");
}

double PACKAGE_LIST::totalCompressedSize() const {
	double ret=0;
	for (unsigned int i=0; i<packages.size(); i++)
	{
		ret+=strtod(packages[i].get_compressed_size().c_str(), NULL);
	}
	return ret;
}


double PACKAGE_LIST::totalInstalledSize() const {
	double ret=0;
	for (unsigned int i=0; i<packages.size(); i++) {
		ret+=strtod(packages[i].get_installed_size().c_str(), NULL);
	}
	return ret;
}

double PACKAGE_LIST::totalCompressedSizeByAction(int select_action) const {
	double ret=0;
	for (unsigned int i=0; i<packages.size(); i++) {
		if (packages[i].action()==select_action) ret+=strtod(packages[i].get_compressed_size().c_str(), NULL);
	}
	return ret;
}
double PACKAGE_LIST::totalInstalledSizeByAction(int select_action) const {
	double ret=0;
	for (unsigned int i=0; i<packages.size(); i++)
	{
		if (packages[i].action()==select_action)
		{
			ret+=strtod(packages[i].get_installed_size().c_str(), NULL);
		}
	}
	return ret;
}


int PACKAGE_LIST::getPackageNumberByMD5(const string& md5) const {
	for (unsigned int i=0; i<packages.size(); i++)
	{
		if (packages[i].get_md5()==md5) return i;
	}
	return -1;
}
void PACKAGE_LIST::clearVersioning()
{
	for (unsigned int i=0; i<packages.size(); i++)
	{
		packages[i].clearVersioning();
	}
}

bool PACKAGE_LIST::equalTo (const PACKAGE_LIST& nlist) const
{
	if (size()!=nlist.size()) return false;
	for (unsigned int i=0; i<size(); i++)
	{
		if (!packages[i].equalTo(nlist[i])) return false;
	}
	return true;
}

bool PACKAGE_LIST::hasInstalledOnes() const {
	if (packages.size()==0) return false;
	for (unsigned int i=0; i<packages.size(); i++) {
		if (packages[i].installed()) {
			return true;
		}
	}
	return false;
}

const PACKAGE* PACKAGE_LIST::getInstalledOne() const {
	for (unsigned int i=0; i<packages.size(); i++) {
		if (packages[i].installed()) return &packages[i];
	}
	return NULL;
}

PACKAGE* PACKAGE_LIST::getMaxVersion() {
	if (packages.size()>0)	{
		return &packages[getMaxVersionNumber(packages[0].get_name())];
	}
	else
	{
		return NULL;
	}
}
const PACKAGE& PACKAGE_LIST::operator [](int num) const {
	return packages[num];
}

const PACKAGE& PACKAGE_LIST::at(const int& num) const {
	return packages[num];
}
PACKAGE* PACKAGE_LIST::get_package_ptr(const int& num) {
		return &packages[num];
}

PACKAGE* PACKAGE_LIST::getPackageByTableID(unsigned int id) {
	if (id >=packages.size())
	{
		mError("No such id");
		return &__empty;
	}
	return &packages[tableID[id]];
}

void PACKAGE_LIST::setTableID(const int& pkgNum, const int& id) {
	tableID[id]=pkgNum;
	reverseTableID[pkgNum]=id;
}

int PACKAGE_LIST::getTableID(const int& pkgNum) {
	return reverseTableID[pkgNum];
}

int PACKAGE_LIST::getRealNum(const int& id)
{
	return tableID[id];
}
void PACKAGE_LIST::set_package(const int& num, const PACKAGE& package)
{
	if ((unsigned int) num>=packages.size())
	{
		mError("Incorrect num "+IntToStr(num));
		return;
	}
	packages[num]=package;
	setTableID(num, num);
}

unsigned int PACKAGE_LIST::size() const {
	return packages.size();
}

/*void PACKAGE_LIST::add(const PACKAGE& package)
{
    packages.push_back(*package);
    setTableID(packages.size()-1, packages.size()-1);
}*/
void PACKAGE_LIST::add(const PACKAGE& package) {
	packages.push_back(package);
	setTableID(packages.size()-1, packages.size()-1);
}
void PACKAGE_LIST::push_back(const PACKAGE_LIST& plist) {
	add(plist);
}
/*void PACKAGE_LIST::add(PACKAGE_LIST *pkgList)
{
	add_list(pkgList, false);
}*/
void PACKAGE_LIST::add(const PACKAGE_LIST& pkgList) {
	add_list(pkgList, false);
}
void PACKAGE_LIST::add_list(const PACKAGE_LIST& pkgList, bool skip_identical)
{
	// IMPORTANT NOTE!
	// If skip_identical is true, the locations will be MERGED together!
	bool identical_found=false;
	bool location_found=false;
	for (unsigned int i=0; i<pkgList.size();i++)
	{
		if (skip_identical)
		{
			identical_found=false;
			// Checking if lists have identical items, remove it
			for (unsigned int s=0; s<packages.size(); s++)
			{
				if (packages[s].get_md5()==pkgList[i].get_md5())
				{
					identical_found=true;
					// Comparing locations and merging
					for (unsigned int l=0; l<pkgList[i].get_locations().size(); l++) {
						location_found=false;
						for (unsigned int ls=0; ls<packages[s].get_locations().size(); ls++) {
							if (packages[s].get_locations().at(ls).equalTo(pkgList[i].get_locations().at(l)))
							{
								location_found=true;
								break;
							}
						}
						if (!location_found)
						{
							packages[s].get_locations_ptr()->push_back(pkgList[i].get_locations().at(l));
						}
					}
					break;
				}

			}
		}
		if (!identical_found)
		{
			this->add(pkgList[i]);
		}
	}
}


void PACKAGE_LIST::clear(unsigned int new_size)
{
	packages.clear();
    	packages.resize(new_size);
	tableID.clear();
	reverseTableID.clear();
}

void PACKAGE_LIST::set_size(unsigned int new_size)
{
	packages.resize(new_size);
	for (unsigned int i=0; i<new_size; i++)
	{
		setTableID(i,i);
	}

}

bool PACKAGE_LIST::IsEmpty() const {
	return packages.empty();
}

const vector<DEPENDENCY>& PACKAGE_LIST::getDepList(const int& i) const {
	return packages[i].get_dependencies();
}

PACKAGE* PACKAGE_LIST::findMaxVersion()
{
	string max_version, max_build;
	int id=0;
	string tmp_ver, tmp_build;
	for (unsigned int i=0;i<packages.size();i++)
	{
		tmp_ver = packages[i].get_version();
		tmp_build = packages[i].get_build();
		if (compareVersions(tmp_ver, tmp_build, max_version, max_build)>=0)
		{
			max_version=tmp_ver;
			max_build=tmp_build;
			id=i;
		}
	}
	return &packages[id];
}

int PACKAGE_LIST::getMaxVersionID(const string& package_name) const {
	if (!versioningInitialized) mWarning("Versioning unitialized!");
	for (unsigned int i=0; i<packages.size(); i++) {
		if (packages[i].get_name() == package_name && packages[i].hasMaxVersion) {
			return packages[i].get_id();
		}
	}
	return MPKGERROR_NOPACKAGE;
}

int PACKAGE_LIST::getMaxVersionNumber(const string& package_name) const {
	if (!versioningInitialized) mWarning("Versioning unitialized!");
	for (unsigned int i=0; i<packages.size(); i++)
	{
		if (packages[i].get_name() == package_name && packages[i].hasMaxVersion)
		{
			return i;
		}
	}
	return MPKGERROR_NOPACKAGE;
}
int PACKAGE_LIST::getInstalledVersionNumber(const string& package_name) const {
	for (unsigned int i=0; i<packages.size(); ++i) {
		if (packages[i].get_name()==package_name && packages[i].installed()) return i;
	}
	return MPKGERROR_NOPACKAGE;
}
int PACKAGE_LIST::getInstalledVersionID(const string& package_name) const {
	int pkg = getInstalledVersionNumber(package_name);
	if (pkg!=MPKGERROR_NOPACKAGE) return packages[pkg].get_id();
	else return MPKGERROR_NOPACKAGE;
}

PACKAGE_LIST::PACKAGE_LIST()
{
	versioningInitialized=false;
	priorityInitialized=false;
}
PACKAGE_LIST::~PACKAGE_LIST()
{
}

/*
int DESCRIPTION::set_id(int id)
{
	description_id=id;
	return 0;
}

int DESCRIPTION::set_language(string language)
{
	description_language=language;
	return 0;
}

int DESCRIPTION::set_text(string text)
{
	description_text=text;
	return 0;
}

int DESCRIPTION::set_shorttext(string shorttext)
{
	short_description_text=shorttext;
	return 0;
}


int DESCRIPTION::get_id()
{
	return description_id;
}

string DESCRIPTION::get_language()
{
	return description_language;
}

string DESCRIPTION::get_text( bool sql)
{
	if (sql) return PrepareSql(description_text);
	else return description_text;
}

string DESCRIPTION::get_shorttext( bool sql)
{
	if (sql) return PrepareSql(short_description_text);
	else return short_description_text;
}

void DESCRIPTION::clear()
{
	description_id=0;
	description_language.clear();
	description_text.clear();
}

DESCRIPTION::DESCRIPTION(){}
DESCRIPTION::~DESCRIPTION()
{
	description_id=0;
}

DESCRIPTION * DESCRIPTION_LIST::get_description(unsigned int num)
{
	if (num>=0 && num <descriptions.size())
	{
		return &descriptions[num];
	}
	else
	{
		mError("DESCRIPTION_LIST: Error in range!");
		abort();
	}
}

int DESCRIPTION_LIST::set_description(unsigned int num, DESCRIPTION description)
{
	if (num>=0 && num < descriptions.size())
	{
		descriptions[num]=description;
		return 0;
	}
	else return -1;
}

int DESCRIPTION_LIST::add(DESCRIPTION description)
{
	descriptions.push_back(description);
	return 0;
}

unsigned int DESCRIPTION_LIST::size()
{
	return descriptions.size();
}

bool DESCRIPTION_LIST::empty()
{
	return descriptions.empty();
}

void DESCRIPTION_LIST::clear()
{
	descriptions.clear();
}

DESCRIPTION_LIST::DESCRIPTION_LIST(){}
DESCRIPTION_LIST::~DESCRIPTION_LIST(){}

cloneList::cloneList(){
initialized = false;
}
*/

//------------------------ bkp ---------------------
/*
void PACKAGE_LIST::initVersioning()
{
	// TODO: ввести beta fields
	// Что надо определить:
	// Для каждого пакета - список номеров того же пакета других версий (НЕ ВКЛЮЧАЯ этот же пакет)
	// Максимально доступную версию
	// Версию установленного пакета
	// Флаг максимальности версии (если таковых пакетов несколько, ставится у одного любого)
	//
	// Делаем пока не оптимально но надежно
	// Шаг первый. Список альтернативных версий
	
	int pkgSize = packages.size();
	for (int i=0; i<pkgSize; i++)
	{

		packages[i].clearVersioning();
		for (int j=0; j<pkgSize; j++)
		{
			// Если это не тот же пакет и имена совпадают - добавляем номер в список
			if (i!=j && packages[i].get_name()==packages[j].get_name())
			{
				if (packages[j].reachable()) packages[i].alternateVersions.push_back(j);
			}
		}
	}

	// Шаг второй. Для каждого пакета ищем максимальную версию
	string max_version, max_build; // Переменная содержащая максимальную версию
	int max_version_id; // номер пакета содержавшего максимальную версию
	string this_version, this_build;
	string installed_version, installed_build;
	for (size_t i=0; i<packages.size(); ++i)
	{
		//mDebug("initVersioning [stage 2]: step "+IntToStr(i));
		max_version.clear();
		max_version_id=-1;
		this_version.clear();
		installed_version.clear();
		if (packages[i].installed())
		{
			installed_version = packages[i].get_version();
			installed_build = packages[i].get_build();
		}

		// Если у пакета нет других версий - значит максимальную версию имеет он
		
		if (packages[i].alternateVersions.empty())
		{
			max_version = packages[i].get_version();
			max_build = packages[i].get_build();
			packages[i].hasMaxVersion=true;
			max_version_id = i;
		}
		else
		{
			for (unsigned int j=0; j<packages[i].alternateVersions.size(); j++)
			{
				this_version = packages[packages[i].alternateVersions[j]].get_version();
				this_build = packages[packages[i].alternateVersions[j]].get_build();
				if (packages[packages[i].alternateVersions[j]].installed())
				{
					installed_version = packages[packages[i].alternateVersions[j]].get_version();
					installed_build = packages[packages[i].alternateVersions[j]].get_build();
				}
				if (compareVersions(this_version, this_build, max_version, max_build)>0)
				{
					max_version = this_version;
					max_build = this_build;
					max_version_id = packages[i].alternateVersions[j];
				}
			}
			if (max_version.empty()) // Если максимальной версии так и не нашлось (все пакеты - одинаковой версии) - то ставим максимум текущему
			{
				max_version = packages[i].get_version();
				max_build = packages[i].get_build();
				max_version_id = i;
			}
			else
			{
				// Проверим - а вдруг именно этот пакет имеет максимальную версию?
				this_version = packages[i].get_version();
				this_build = packages[i].get_build();
				if (compareVersions(this_version, this_build, max_version, max_build)>0)
				{
					max_version = this_version;
					max_build = this_build;
					max_version_id = i;
				}
				// Устанавливаем найденному пакету нужные флаги
			}
		}
		// Запишем установленную версию
		packages[i].hasMaxVersion=false;
		packages[max_version_id].hasMaxVersion=true;
		packages[i].maxVersion=max_version;
		packages[i].maxBuild = max_build;
		packages[i].installedVersion = installed_version;
		packages[i].installedBuild = installed_build;
	}
	versioningInitialized=true;
}
*/


void PACKAGE_LIST::initVersioning()
{
	// TODO:
	// 1. Выставить флаг hasMaxVersion, maxBuild, installedVersion, installedBuild
	//	packages[i].maxVersion=max_version;
	//	packages[i].maxBuild = max_build;
	//	packages[i].installedVersion = installed_version;
	//	packages[i].installedVersion = installed_build;
	// 2. Заполнить alternateVersions
	
	// 0. Сброс
	for (size_t i=0; i<packages.size(); ++i) {
		packages[i].clearVersioning();
	}
	
	// 1. Делаем пачку векторов для каждого имени пакета
	map<string, vector<size_t> > pkgMap;
	for (size_t i=0; i<packages.size(); ++i) {
		pkgMap[packages[i].get_name()].push_back(i);
		for (size_t t=0; t<pkgMap[packages[i].get_name()].size(); ++t) {
			if (i!=pkgMap[packages[i].get_name()][t]) packages[pkgMap[packages[i].get_name()][t]].alternateVersions.push_back(i);
			if (packages[i].installed()) {
				packages[pkgMap[packages[i].get_name()][t]].installedVersion = packages[i].get_version();
				packages[pkgMap[packages[i].get_name()][t]].installedBuild = packages[i].get_build();
			}
		}
	}

	// Almost all done. Let's walk thru packages with it's alternateVersions and fill maximums.
	string maxv, maxb;
	for (size_t i=0; i<packages.size(); ++i) {
		if (!packages[i].maxVersion.empty()) continue;
		maxv = packages[i].installedVersion;
		maxb = packages[i].installedBuild;
		if (maxv.empty()) maxv = packages[i].get_version();
		if (maxb.empty()) maxb = packages[i].get_build();
		for (size_t t=0; t<packages[i].alternateVersions.size(); ++t) {
			if (compareVersions(packages[packages[i].alternateVersions[t]].get_version(), packages[packages[i].alternateVersions[t]].get_build(), maxv, maxb)>0) {
				maxv = packages[packages[i].alternateVersions[t]].get_version();
				maxb = packages[packages[i].alternateVersions[t]].get_build();
			}
		}
		packages[i].maxVersion = maxv;
		packages[i].maxBuild = maxb;
		if (maxv==packages[i].get_version() && maxb == packages[i].get_build()) packages[i].hasMaxVersion = true;
		for (size_t t=0; t<packages[i].alternateVersions.size(); ++t) {
			packages[packages[i].alternateVersions[t]].maxVersion = maxv;
			packages[packages[i].alternateVersions[t]].maxBuild = maxb;
			if (maxv == packages[packages[i].alternateVersions[t]].get_version() && maxb == packages[packages[i].alternateVersions[t]].get_build()) 
				packages[packages[i].alternateVersions[t]].hasMaxVersion = true;
		}
	}
	// Seems that here is all. Compact code, isn't it? :)
	versioningInitialized = true;
	return;
}

bool meetVersion(const versionData &condition, const string& packageVersion)
{
	int iCondition=atoi(condition.condition.c_str());
	const string& version1=packageVersion;
	const string& version2=condition.version;
	switch (iCondition)
	{
		case VER_MORE:
			if (strverscmp2(version1,version2)>0)  return true;
			else return false;
			break;
		case VER_LESS:
			if (strverscmp2(version1,version2)<0) return true;
			else return false;
			break;
		case VER_EQUAL:
			if (strverscmp2(version1,version2)==0) return true;
			else return false;
			break;
		case VER_NOTEQUAL:
			if (strverscmp2(version1,version2)!=0) return true;
			else return false;
			break;
		case VER_XMORE:
			if (strverscmp2(version1,version2)>=0) return true;
			else return false;
			break;
		case VER_XLESS:
			if (strverscmp2(version1,version2)<=0) return true;
			else return false;
			break;
		case VER_ANY:
			return true;
			break;
		default:
			mError((string) __func__ + ": unknown condition " + IntToStr(iCondition));
			return true;
	}
	return true;
}

vector<unsigned int> checkedPackages;
bool notTested(unsigned int num)
{
	for (unsigned int i=0; i<checkedPackages.size(); i++)
	{
		if (checkedPackages[i]==num) return false;
	}
	return true;
}
int PACKAGE_LIST::getPackageNumberByName(const string& name) const {
	for (unsigned int i=0; i<packages.size(); ++i) {
		if (packages[i].get_name()==name) return i;
	}
	mDebug("No such package " + name);
	return -1;
}
void PACKAGE_LIST::buildDependencyOrder()
{
	int pkgSize = this->size();
	vector<int> callList;
	for (int i=0; i<pkgSize; i++)
	{
		callList.clear();
		callList.push_back(i);
		get_max_dtree_length(*this, i, callList);
	}
	priorityInitialized=true;
}
int get_max_dtree_length(PACKAGE_LIST& pkgList, const int& package_id, vector<int>& callList)
{
	bool loop=false;

	
	for (unsigned int i=0; i<callList.size(); i++)
	{
		for (unsigned int t=0; t<callList.size(); t++)
		{
			if (i!=t && callList[i]==callList[t])
			{
				loop=true;
				loopCount++;
			}
		}
	}
	PACKAGE *_p = pkgList.get_package_ptr(package_id);
	int ret=0;
	int max_ret=-1;
	int pkgNum;
	for (unsigned int i=0; i<_p->get_dependencies().size(); i++)
	{
		pkgNum = pkgList.getPackageNumberByName(_p->get_dependencies().at(i).get_package_name());
		if (pkgNum>=0)
		{
			callList.push_back(pkgNum);
			if (!loop) ret = 1 + get_max_dtree_length(pkgList, pkgNum,callList);
			if (max_ret < ret) max_ret = ret;
		}
	}
	_p->priority=ret;
	return ret;
}

DeltaSource::DeltaSource(const string& _url, const string& _md5, const string& _orig_filename, const string& _orig_md5, const string& _size) {
	dup_url=_url;
	dup_md5=_md5;
	orig_filename=_orig_filename;
	orig_md5=_orig_md5;
	dup_size = _size;
}
DeltaSource::~DeltaSource() {
}

SPKG::SPKG() {
}

SPKG::~SPKG() {
}
