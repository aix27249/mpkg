#include <mpkg/libmpkg.h>
struct PkgData {
	string name;
	string version;
	string arch;
	string build;
	string filename;
	string tag;
	string rawpath;
};

void parseString(string data, vector<PkgData>& pkgs) {
	if (data.empty() || data.find_first_not_of(" \n\t")==std::string::npos) return;
	PkgData tmp;
	size_t p;
       	p=data.find(".txz ");
	if (p==std::string::npos) p=data.find(".tgz ");
	if (p==std::string::npos) p=data.find(".tbz ");
	if (p==std::string::npos) p=data.find(".tlz ");
	if (p==std::string::npos) return;
	if (data.length()<=p+5) return;
	tmp.tag = data.substr(p+5);
	tmp.rawpath = data.substr(0, p+5);
	data = data.substr(0, p);
	data = getFilename(data);
	tmp.filename = data;
	//printf("DATA: %s\n", data.c_str());
	string ptrstr = data.substr(data.find_last_of("-")+1);
	tmp.build=ptrstr;
	data = data.substr(0,data.find_last_of("-"));
	ptrstr = data.substr(data.find_last_of("-")+1);
	tmp.arch=ptrstr;
	data = data.substr(0,data.find_last_of("-"));
	ptrstr=data.substr(data.find_last_of("-")+1);
	tmp.version=ptrstr;
	data = data.substr(0,data.find_last_of("-"));
	tmp.name = data;
	pkgs.push_back(tmp);
}


int main(int argc, char **argv) {
	if (argc<2) {
		fprintf(stderr, "no taglist specified\n");
		return 1;
	}
	vector<string> data = ReadFileStrings(argv[1]);
	string proot = argv[2];
	vector<PkgData> pkgs;
	for (size_t i=0; i<data.size(); ++i) {
		parseString(data[i], pkgs);
	}
	Repository repObj;
	PACKAGE_LIST rep;
	repObj.get_index("file://"+getAbsolutePath(argv[2]), &rep);


	for (size_t i=0; i<pkgs.size(); ++i) {
		printf("%s\n", pkgs[i].name.c_str());
	}
	bool foundThis;
	vector <string> failed;
	string filename;
	for (size_t i=0; i<rep.size(); ++i) {
		foundThis = false;
		filename = string(rep[i].get_locations()[0].get_full_url() + rep[i].get_filename()).substr(7);
		for (size_t t=0; !foundThis && t<pkgs.size(); ++t) {
			if (rep[i].get_name()==pkgs[t].name) {
				foundThis = true;

				printf("%s: %s, path: %s\n", rep[i].get_name().c_str(), pkgs[t].tag.c_str(), filename.c_str());
				system("mpkg-setmeta " + filename + " --add-tag=" + pkgs[t].tag);
			}
		}
		if (!foundThis) {
			printf("%s: tag not found\n", rep[i].get_name().c_str());
			failed.push_back(filename);
			
		}
	}
	WriteFileStrings("untagged.log", failed);

	/*size_t start=0, end=pkgs.size(), half=pkgs.size()/2;
	pid_t pid = fork();
	if (pid) {
		end=half;
		printf("Parent working\n");
	}
	else {
		printf("Child working\n");
		start=half+1;
	}
	for (size_t i=start; i<end; ++i) {
		//printf("[%s]-[%s]-[%s]-[%s]: [%s]\n", pkgs[i].name.c_str(), pkgs[i].version.c_str(), pkgs[i].arch.c_str(), pkgs[i].build.c_str(), pkgs[i].tag.c_str());
		//system("find . -name " + pkgs[i].name + "-*-*-" + pkgs[i].build + ".t?z | sed -e s/txz/txz\\ " + pkgs[i].tag + "/ | sed -e s/tgz/tgz\\ " + pkgs[i].tag + "/");
		system("find . -mmin +200 -name " + pkgs[i].name + "-*-*-*.t?z | xargs mpkg tag " + pkgs[i].tag);
	}*/
	return 0;

}
