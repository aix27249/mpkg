#include "engine.h"

// -------------------- LoadTags thread --------------------------
LoadTags::LoadTags(mpkg *_core) {
	printf("Creating %s\n", __func__);
	core = _core;
}

LoadTags::~LoadTags() {

	printf("Destroying %s\n", __func__);
}

void LoadTags::run() {
	tagList.clear();
	vector<string> tags;
	core->get_available_tags(&tags);
	sort(tags.begin(), tags.end());
	for (unsigned int i=0; i<tags.size(); ++i) {
		tagList.push_back(QString::fromStdString(tags[i]));
	}
	emit done();
}

const QStringList& LoadTags::getTagList() {
	return tagList;
}

// ------------------- LoadPackages thread ------------------------
LoadPackages::LoadPackages(mpkg *_core) {

	printf("Creating %s\n", __func__);
	core = _core;
}

LoadPackages::~LoadPackages() {
	printf("Destroying %s\n", __func__);
}

void LoadPackages::run() {
	packageList.clear();
	SQLRecord sqlSearch;
	core->get_packagelist(sqlSearch, &packageList);
	//core->db->get_full_locationlist(&packageList);
	packageList.initVersioning();
	emit done();
}

const PACKAGE_LIST& LoadPackages::getPackageList() {
	return packageList;
}

// ----------------- UpdateRepositoryData thread ----------------------
LoadUpdateRepositoryData::LoadUpdateRepositoryData(mpkg *_core) {
	printf("Creating %s\n", __func__);
	core = _core;
}

LoadUpdateRepositoryData::~LoadUpdateRepositoryData() {

	printf("Destroying %s\n", __func__);
}

void LoadUpdateRepositoryData::run() {
	core->update_repository_data();
	delete_tmp_files();
	if (usedCdromMount) system("umount " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
	emit done();
}

// ----------------- CommitActions thread --------------------

CommitActions::CommitActions(mpkg* _core) {

	printf("Creating %s\n", __func__);
	core = _core;
}

CommitActions::~CommitActions() {

	printf("Destroying %s\n", __func__);
}

void CommitActions::run() {
	core->DepTracker->commitToDb();
	core->commit();
	emit done();
}
