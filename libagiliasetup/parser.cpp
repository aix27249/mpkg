#include "agiliasetup.h"

int saveSettings(const string& filename, map<string, string> settings, vector<string> repositories, map<string, map<string, string> > partitions) {
	// Settings will be saved to ~/.config/textinstaller.conf
	vector<string> conf;

	map<string, string>::iterator it, mit;
	map<string, map<string, string> >::iterator mapit;

	for (it=settings.begin(); it!=settings.end(); it++) {
		conf.push_back(it->first + "=" + it->second);
	}
	conf.push_back("");

	conf.push_back("[repositories]");
	for (size_t i=0; i<repositories.size(); ++i) {
		conf.push_back(repositories[i]);
	}
	conf.push_back("");

	conf.push_back("# Partitioning layout");
	for (mapit=partitions.begin(); mapit!=partitions.end(); mapit++) {
		conf.push_back("");
		conf.push_back("[" + mapit->first + "]");
		for (mit=partitions[mapit->first].begin(); mit!=partitions[mapit->first].end(); mit++) {
			conf.push_back(mit->first + "=" + mit->second);
		}
	}

	WriteFileStrings(filename, conf);
	return 0;
}
bool parseConfString(const string& conf, string &name, string &value) {
	size_t pos;
	pos = conf.find_first_of("=");
	if (pos==std::string::npos) return false;
	name = cutSpaces(conf.substr(0, pos));
	if (pos>=conf.size()-1) value.clear();
	else value = cutSpaces(conf.substr(pos+1));
	return true;

}
int loadSettings(const string& filename, map<string, string> &settings, vector<string> &repositories, map<string, map<string, string> > &partitions) {
	// Load settings until we found brackets
	vector<string> conf = ReadFileStrings(filename);
	string name, value;
	for (size_t i=0; i<conf.size(); ++i) {
		if (conf[i].empty()) continue;
		if (cutSpaces(conf[i])[0]=='#') continue;
		if (cutSpaces(conf[i])[0]=='[') break;
		if (parseConfString(conf[i], name, value)) settings[name]=value;
	}

	// Now, find repos
	bool found = false;
	for (size_t i=0; i<conf.size(); ++i) {
		if (conf[i].empty()) continue;
		if (cutSpaces(conf[i])[0]=='#') continue;
		if (!found) {
			if (cutSpaces(conf[i])=="[repositories]") found = true;
			continue;
		}
		if (cutSpaces(conf[i])[0]=='[') break;
		repositories.push_back(cutSpaces(conf[i]));
	}

	// Partitions
	found = false;
	name.clear();
	value.clear();
	string part;
	for (size_t i=0; i<conf.size(); ++i) {
		if (conf[i].empty()) continue;
		if (cutSpaces(conf[i])[0]=='#') continue;
		if (cutSpaces(conf[i]).find("[/")==0 && cutSpaces(conf[i]).size()>2) {
			found = true;	
			part = cutSpaces(conf[i]).substr(1, cutSpaces(conf[i]).size()-2);
			continue;
		}
		if (found && parseConfString(conf[i], name, value)) partitions[part][name]=value;
	}
	return 0;
}


void parseConfig(map<string, string> *_strSettings, vector<TagPair> *_users, vector<PartConfig> *_partConfigs, vector<string> *_additional_repositories) {
	map<string, string> strSettings;
	vector<PartConfig> partConfigs;
	vector<TagPair> users;
	vector<string> additional_repositories;

	map<string, map<string, string> > partitions;
	vector<string> repositories;
	string home = getenv("HOME");
	loadSettings(home + "/.config/agilia_installer.conf", strSettings, repositories, partitions);

	
	users.push_back(TagPair(strSettings["username"], strSettings["password"]));

	PartConfig *pConfig;
	map<string, map<string, string> >::iterator mapit;
	for (mapit=partitions.begin(); mapit!=partitions.end(); mapit++) {
		pConfig = new PartConfig;
		pConfig->partition = mapit->first;
		pConfig->mountpoint = partitions[mapit->first]["mountpoint"];
		pConfig->format = atoi(partitions[mapit->first]["format"].c_str());
		pConfig->fs = partitions[mapit->first]["fs"];
		pConfig->mount_options = partitions[mapit->first]["options"];
		if (!pConfig->mountpoint.empty()) partConfigs.push_back(*pConfig); // Skip subvolume groups from list
		delete pConfig;
	}

	// Save
	*_strSettings = strSettings;
	*_users = users;
	*_partConfigs = partConfigs;
	*_additional_repositories = additional_repositories;

}
