#include "tests.h"
#include "libmpkg.h"
void run_testing_facility(mpkg *core, string test_type) {
	printf("test_type: %s\n", test_type.c_str());
	SQLTable pkgList;
	SQLTable fileList;
	SQLTable tagLinkList;
	SQLRecord pkgFields, fileFields, tagFields, sqlSearch;
	pkgFields.addField("package_id");
	tagFields.addField("packages_package_id");
	fileFields.addField("packages_package_id");
	core->db->get_sql_vtable(pkgList, pkgFields, "packages", sqlSearch);
	sqlSearch.groupBy="packages_package_id";
	core->db->get_sql_vtable(fileList, fileFields, "files", sqlSearch);
	core->db->get_sql_vtable(tagLinkList, tagFields, "tags_links", sqlSearch);
	printf("Checking filelist integrity...\n");
	bool found;
	for (unsigned int i=0; i<fileList.size(); ++i) {
		found=false;
		for (unsigned int t=0; !found && t<pkgList.size(); ++t) {
			if (pkgList.getValue(t,0)==fileList.getValue(i, 0)) found=true;
		}
		if (!found) printf("Filelist scan: missing package %s\n", fileList.getValue(i, 0).c_str());
	}
	for (unsigned int i=0; i<tagLinkList.size(); ++i) {
		found=false;
		for (unsigned int t=0; !found && t<pkgList.size(); ++t) {
			if (pkgList.getValue(t,0)==tagLinkList.getValue(i, 0)) found=true;
		}
		if (!found) printf("Taglink scan: missing package %s\n", fileList.getValue(i, 0).c_str());
	}


}
