/*
	MPKG packaging system
	Basic data types descriptions
	Second edition: RISC architecture =)
	$Id: dataunits.h,v 1.54 2007/12/10 03:12:58 i27249 Exp $
*/


#ifndef DATAUNITS_H_
#define DATAUNITS_H_

#include <string>
#include <vector>
#include <map>
#include <stdint.h>
using namespace std;

struct CustomPkgSet {
	string name, desc, full, hw;
	uint64_t csize, isize;
	size_t count;
	bool hasX11;
	bool hasDM;
};



// Server type definitions
// Note: the definition order is important. Set this to comply the sorting order (less value comes first)
#define SRV_LOCAL 0
#define SRV_FILE 1
#define SRV_NETWORK 2
#define SRV_CDROM 3
class ConfigAttribute {
	public: ConfigAttribute();
		~ConfigAttribute();
		string name, value;
};
class ConfigFile {
	public: ConfigFile();
		~ConfigFile();
		string name;
		vector<ConfigAttribute> attr;
		bool hasAttribute(const string &attr_name, string value="") const;
		void addAttribute(const string &attr_name, const string& value);
};


class DeltaSource {
	public:
		DeltaSource(const string& _url, const string& _md5, const string& _orig_filename, const string& _orig_md5, const string& _size);
		~DeltaSource();
		string dup_url;
		string dup_md5;
		string dup_size;
		string orig_filename;
		string orig_md5;
};
// class LOCATION: describes one location unit
class LOCATION
{
    private:
	// INTERNAL DATA //
	int location_id;
	string server_url;
	string location_path;
	bool local;
	
    public:
	// Comparsion
	bool equalTo (const LOCATION &location) const;

	// Data retriveal
	const int& get_id() const;
	int get_type() const;
	const bool& get_local() const;
	const string& get_path() const;
	const string& get_server_url() const;
	const string get_full_url() const;

	// Data writing
	void set_id(const int& id);
	void set_path(const string& path);
	void set_local();
	void unset_local();
	void set_server_url(const string& new_url);


	// Dimensions, empty, etc
	bool IsEmpty() const;
	void clear();

	// Constructor and destructor
	LOCATION();
	~LOCATION();
};

struct versionData
{
	string version;
	string condition;
};
class PACKAGE;
class DEPENDENCY
{
    private:
	// INTERNAL DATA //
	int dependency_id;
	string dependency_type;
	string dependency_package_name;
	int dependency_broken;
	versionData version_data;
	bool buildOnly;

    public:
	// Comparsion
	bool equalTo(const DEPENDENCY& dependency) const;
	bool isResolvableBy(const PACKAGE &) const;

	// Data retriveal
	const int& get_id() const;
	const string& get_condition() const;
	const string& get_type() const;
	const string& get_package_name() const;
	const string& get_package_version() const;
	const int& get_broken() const;
	const versionData& get_version_data() const;
	const bool& isBuildOnly() const;

	// Visual data retriveal (UI purposes)
	const string get_vbroken() const;
	const string get_vcondition() const;
	const string getDepInfo() const;

	// Data writing
	void set_id(const int& id);
	void set_condition(const string& condition);
	void set_type(const string& type);
	void set_package_name(const string& package_name);
	void set_package_version(const string& package_version);
	void set_broken(const int& broken);
	void setBuildOnly(const bool flag=true);

	// Emptyness, empty, etc
	bool IsEmpty() const;
	void clear();

	// Constructor & destructor
	DEPENDENCY();
	~DEPENDENCY();
};

class FILE_EXTENDED_DATA {
    public:
	// INTERNAL DATA //
	int file_id;
	string *filename;
	string backup_file;
    public:
	void clear();

	// Backup id's
	int owner_id;
	int overwriter_id;

	// Constructor && destructor
	FILE_EXTENDED_DATA();
	~FILE_EXTENDED_DATA();
};

void _sortLocations(vector<LOCATION>& locations); // Location sorting

class PACKAGE
{
    private:
	// INTERNAL DATA //
	int package_id;
	string package_name;
	string package_version;
	string package_arch;
	string package_build;
	string package_compressed_size;
	string package_installed_size;
	string package_short_description;
	string package_description; // DEPRECATED: use only short description please
	string package_changelog;
	string package_packager;
	string package_packager_email;
	bool package_installed;
	bool package_configexist;
	int package_action;	
	string package_md5;
	string package_filename;
	string package_betarelease;
	int package_installed_by_dependency;
	int package_type;
	int package_err_type;
	// EXTERNAL DATA //
	vector<string> package_files;
	vector<LOCATION> package_locations;
	vector<DEPENDENCY> package_dependencies;
	vector<string> package_tags;
	string package_repository_tags;
    public:
	string abuild_url;
	vector<ConfigFile> config_files;
	int build_date;
	time_t add_date;
	string getAlternative() const;
	vector<string> getAlternativeVector() const;
	bool checkAlternative(const string& alt) const;

	string package_distro_version;
	string provides;
	const string& get_provides() const;
	void set_provides(const string& _provides);
	const string& get_conflicts() const;
	void set_conflicts(const string& _provides);
	string conflicts;
	const string& get_corename() const;
	vector<DeltaSource> deltaSources;
	bool needSpecialUpdate;
	string usedSource;
	string pgp_signature;
	const int& get_type() const;
	void set_type(const int& type);
	bool isRemoveBlacklisted() const;
	PACKAGE * updatingBy; // Link to another update pair, e.g. for remove_list, it is new package, and for install_list, it is the one from remove_package
	versionData requiredVersion;
	int priority;
	int itemID;	// For status purposes only, means the number in PackageData vector
	bool isBroken;
	bool isRequirement;
	vector<int>alternateVersions;
	bool hasMaxVersion;
	string maxVersion;
	string maxBuild;
	bool newPackage;	// Используется пока что только при обновлении. Есть мысль использовать в GUI (еще не знаю по какому принципу)
	string installedVersion;
	string installedBuild;

	bool deprecated() const;
	// Comparsion
	bool equalTo(const PACKAGE& pkg) const;
	bool locationsEqualTo(const PACKAGE& pkg) const;
	bool tagsEqualTo(const PACKAGE& pkg) const;
	bool depsEqualTo(const PACKAGE& pkg) const;

	bool isTaggedBy(const string& tag) const;
	// Data retrieving	
	bool isItRequired(const PACKAGE& testPackage) const;
	bool isUpdate() const;
	const int& get_id() const;
	const int& get_installed_by_dependency() const;
	const string& get_name() const;
	const string& get_version() const;
	const string& get_arch() const;
	const string& get_build() const;
	const string& get_compressed_size() const;
	const string& get_installed_size() const;
	const string& get_short_description() const;
	const string& get_description() const; // DEPRECATED
	const string& get_changelog() const;
	const string& get_packager() const;
	const string& get_packager_email() const;
	const vector<string> get_repository_tags_vector() const;
	const string& get_repository_tags() const;
	void set_repository_tags(const string& rtags);
	bool available(bool includeLocal=false) const;
	bool installed() const;
	bool configexist() const;
	int action() const;
	bool reachable(bool includeConfigFiles=false) const;	// ==(package_available || package_installed)
	const string& get_md5() const;
	const string& get_filename() const;
	const string& get_betarelease() const;
	bool isBeta() const;
	int get_err_type() const;

	const vector<string>& get_files() const;
	vector<string>* get_files_ptr();
	const vector<LOCATION>& get_locations() const;
	vector<LOCATION>* get_locations_ptr();
	const vector<DEPENDENCY>& get_dependencies() const;
	vector<DEPENDENCY>* get_dependencies_ptr();
	const vector<string>& get_tags() const;
	vector<string>* get_tags_ptr();
	const string get_scriptdir() const;

	// UI functions
	const string get_fullversion() const;
	const string get_vstatus(bool color=false) const;

	// Data writing
	void set_installed_by_dependency(const int& value);
	void set_broken(bool flag=true);
	void set_requiredVersion(const versionData& reqVersion);
	void set_id(const int& id);
	void set_name(const string& name);
	void set_version(const string& version);
	void set_arch(const string& arch);
	void set_build(const string& build);
	void set_compressed_size(const string& compressed_size);
	void set_installed_size(const string& installed_size);
	void set_short_description(const string& short_description);
	void set_description(const string& description); // DEPRECATED
	void set_changelog(const string& changelog);
	void set_packager(const string& packager);
	void set_packager_email(const string& packager_email);
	void set_installed(const bool flag = true);
	void set_configexist(const bool flag = true);
	void set_action(const int& new_action, string reason);
	string package_action_reason;
	void set_md5(const string& md5);
	void set_filename(const string& filename);
	void set_betarelease(const string& betarelease);
	void set_err_type(const int& err);

	void set_files(const vector<string>& files);
	void set_locations(const vector<LOCATION>& locations);
	void set_dependencies(const vector<DEPENDENCY>& dependencies);
	void set_tags(const vector<string>& tags);

	void add_dependency(const string& package_name, const string& dep_condition, const string& package_version);
	void add_file(const string& file_name);
	void add_tag(const string& tag);

	// Internal structure methods
	void sortLocations();
	void clearVersioning();
	
	// Empty, clear, etc
	void clear();
	bool IsEmpty() const;
	
	// Constructor & destructor
	PACKAGE();
	~PACKAGE();
};

class PACKAGE_LIST {
    private:
	vector<PACKAGE> packages;
	map<int, int> tableID;
	map<int, int> reverseTableID;
	PACKAGE __empty;
    public:
	bool priorityInitialized;
	void sortByPriority(const bool& reverse_order=false);
	void sortByTags(const bool& reverse_order = false);
	void sortByPriorityNew(const bool& reverse_order=false);
	void sortByLocations();
	void sortByLocationTypes();
	void buildDependencyOrder();
	int getPackageNumberByName(const string& name) const;
	const PACKAGE& getPackageByID(const int& id) const;
	PACKAGE * getPackageByIDPtr(const int& id, bool quiet=false);
	double totalCompressedSize() const;
	double totalInstalledSize() const;
	double totalInstalledSizeByAction(const int select_action) const;
	double totalCompressedSizeByAction(const int select_action) const;
	bool hasInstalledOnes() const;
	const PACKAGE* getInstalledOne() const;
	PACKAGE* getMaxVersion();
	bool versioningInitialized;
	PACKAGE* get_package_ptr(const int& num);
	const PACKAGE& at(const int& num) const;
	PACKAGE* getPackageByTableID(const unsigned int id);
	void setTableID(const int& pkgNum, const int& id);
	int getTableID(const int& pkgNum);
	int getRealNum(const int& id);
	void set_package(const int& num, const PACKAGE& package);
	bool equalTo(const PACKAGE_LIST& nlist) const;
	//void add(PACKAGE *package);
	void add(const PACKAGE&);
	void push_back(const PACKAGE_LIST& plist);
	void add(const PACKAGE_LIST& pkgList);
	void add_list(const PACKAGE_LIST& pkgList, bool skip_identical=true);
	void clear(const unsigned int new_size = 0);
	bool IsEmpty() const;
	unsigned int size() const;
	void set_size(const unsigned int new_size);
	int getPackageNumberByMD5(const string& md5) const;		// return number (NOT package ID!) of package in vector (if found). Else, returns -1.
	int getMaxVersionID(const string& package_name) const; // Return package ID
	int getMaxVersionNumber(const string& package_name) const;	// Return package number (in array)
	int getInstalledVersionNumber(const string& package_name) const;
	int getInstalledVersionID(const string& package_name) const;

	vector<string> getAlternativeList(bool separate=false) const;
	void switchAlternatives(const vector<string>& alternatives); // Switches install queue to specified alternatives
	vector<PACKAGE *> getAllAlternatives(const string& corename);
	PACKAGE *findMaxVersion();
	const vector<DEPENDENCY>& getDepList(const int& i) const;
	void initVersioning();
	void clearVersioning();
	PACKAGE_LIST();
	~PACKAGE_LIST();
	const PACKAGE& operator [](int num) const;

};

bool meetVersion(const versionData& condition, const string& packageVersion);
int get_max_dtree_length(PACKAGE_LIST& pkgList, const int& package_id, vector<int>& callList);

// This class describes spkg-related things.
class SPKG {
	public:
		SPKG();
		~SPKG();
		PACKAGE pkg;

		string buildsystem;
		string url;
		vector<string> configure_keys;
		vector<string> configure_values;
		vector<string> patches;
		string source_root;
		string max_numjobs;
		bool allow_change_cflags;
		string march, mtune, olevel;
		string custom_gcc_options;
		bool use_cflags;
		string custom_configure, custom_make, custom_makeinstall;
		bool no_subfolder;
		string env_options;
		bool nostrip;
};


#endif //DATAUNITS_H_

