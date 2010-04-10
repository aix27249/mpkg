// This utility will checkout filelist.tar.xz from all enabled repositories and create import_filelist.sql file
// NOTE: This should be run ***AFTER*** mpkg-update (we need correct package_id)
#include <mpkg/libmpkg.h>

string filestrParser(string str, int id) {
	string ret = "INSERT INTO files VALUES(NULL, ";
	size_t p = str.find_last_of(" ");
	string type = str.substr(p+1);
	str.erase(str.begin()+p, str.end());
	p = str.find_last_of(" ");
	string md5 = str.substr(p+1);
	str.erase(str.begin()+p, str.end());
	ret += "'" + str + "', '" + type + "', '" + IntToStr(id) + "', '" + md5 + "');";
	return ret;


}

int main(int argc, char **argv) {
	string output="filelist.sql";
	if (argc>1) output=argv[1];
	Repository rep;
	PACKAGE_LIST *tmpPackages;
	PACKAGE_LIST currentPkgList;
	mpkg core;
	SQLRecord sqlSearch;
	core.get_packagelist(sqlSearch, &currentPkgList);
	vector<string> query;
	//query.push_back("BEGIN TRANSACTION;");
	query.push_back("DELETE FROM files;");
	query.push_back("ALTER TABLE files ADD file_md5 text;");
	string tmp;
	vector<string> filelist;
	int id;
	for (size_t i=0; i<REPOSITORY_LIST.size(); ++i) {
		tmp = get_tmp_dir();
		tmpPackages = new PACKAGE_LIST;
		rep.get_index(REPOSITORY_LIST[i], tmpPackages);
		CommonGetFile(REPOSITORY_LIST[i]+"/filelist.tar.xz", tmp + "/filelist.tar.xz");
		system("tar xf " + tmp + "/filelist.tar.xz -C " + tmp);
		for (size_t p=0; p<tmpPackages->size(); ++p) {
			//printf("Searching package...\n");
			id = currentPkgList.getPackageNumberByMD5(tmpPackages->at(p).get_md5());
			if (id!=-1) id = currentPkgList[id].get_id();
			else continue;
			printf("Collecting data for %d\n", id);
			filelist = ReadFileStrings(tmp+"/.mpkg_filelists/" + tmpPackages->at(p).get_locations().at(0).get_path() + "/" + tmpPackages->at(p).get_filename() + ".filelist");
			for (size_t f=0; f<filelist.size(); ++f) {
				query.push_back(filestrParser(filelist[f], id));
			}
			printf("Query collect complete\n");
		}
		delete tmpPackages;
		system("rm -rf " + tmp);
	}
	//query.push_back("COMMIT;");
	printf("Writing file\n");
	WriteFileStrings(output, query);
	printf("Done\n");
	return 0;



}
