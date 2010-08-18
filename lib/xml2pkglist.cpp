#include "xml2pkglist.h"
#include <mpkgsupport/mpkgsupport.h>
#include "conditions.h"
void parseMaintainer(xmlDocPtr doc, xmlNodePtr cur, PACKAGE &pkg) {
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *) "name")) pkg.set_packager(cutSpaces((const char *) xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
		else if (!xmlStrcmp(cur->name, (const xmlChar *) "email")) pkg.set_packager_email(cutSpaces((const char *) xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
		cur = cur->next;
	}
}

DEPENDENCY parseDependency(xmlDocPtr doc, xmlNodePtr cur) {
	DEPENDENCY dep;
	cur = cur->xmlChildrenNode;
	const char *key;
	while (cur != NULL) {
		key = (const char *) xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (key) {
			if (!xmlStrcmp(cur->name, (const xmlChar *) "name")) dep.set_package_name(cutSpaces(key));
			if (!xmlStrcmp(cur->name, (const xmlChar *) "condition")) dep.set_condition(IntToStr(condition2int(cutSpaces(key))));
			if (!xmlStrcmp(cur->name, (const xmlChar *) "version")) dep.set_package_version(cutSpaces(key));
			if (!xmlStrcmp(cur->name, (const xmlChar *) "build_only")) {
				if (cutSpaces(key)=="true") dep.setBuildOnly(true);
				else dep.setBuildOnly(false);
			}
		}
		cur = cur->next;
	}
	return dep;

}
void parseDependencies(xmlDocPtr doc, xmlNodePtr cur, PACKAGE &pkg) {
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *) "dep")) pkg.get_dependencies_ptr()->push_back(parseDependency(doc, cur));
		cur = cur->next;
	}

}
void parseTags(xmlDocPtr doc, xmlNodePtr cur, PACKAGE &pkg) {
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *) "tag")) pkg.get_tags_ptr()->push_back(cutSpaces((const char *) xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
		cur = cur->next;
	}

}

void parseBDelta(xmlDocPtr doc, xmlNodePtr cur, PACKAGE &pkg) {
	string delta_url, delta_md5, delta_size, orig_filename, orig_md5;
	cur = cur->xmlChildrenNode;
	const char *key;
	while(cur != NULL) {
		delta_url.clear();
	       	delta_md5.clear();
		orig_filename.clear();
		orig_md5.clear();
		key = (const char *) xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (!xmlStrcmp(cur->name, (const xmlChar *) "dup")) delta_url = cutSpaces(key);
		else if (!xmlStrcmp(cur->name, (const xmlChar *) "dup_md5")) delta_md5 = cutSpaces(key);
		else if (!xmlStrcmp(cur->name, (const xmlChar *) "orig_md5")) orig_md5 = cutSpaces(key);
		else if (!xmlStrcmp(cur->name, (const xmlChar *) "orig")) orig_filename = cutSpaces(key);
		else if (!xmlStrcmp(cur->name, (const xmlChar *) "dup_size")) delta_size = cutSpaces(key);
		cur = cur->next;
	}
	pkg.deltaSources.push_back(DeltaSource(delta_url, delta_md5, orig_filename, orig_md5, delta_size));
}

void parseConfigFiles(xmlDocPtr doc, xmlNodePtr cur, PACKAGE &pkg) {
	ConfigFile *cfile;
	cur = cur->xmlChildrenNode;
	const char *key;
	xmlAttrPtr attr;
	while(cur != NULL) {
		key = (const char *) xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (!xmlStrcmp(cur->name, (const xmlChar *) "conf_file")) {
			cfile = new ConfigFile;
			cfile->name = cutSpaces(key);
			attr = cur->properties;
			while (attr != NULL) {
				cfile->addAttribute((const char *) attr->name, (const char *) attr->children->name);
				attr = attr->next;
			}
			pkg.config_files.push_back(*cfile);
			delete cfile;
		}
		cur = cur->next;
	}

}

void parseXmlTag(xmlDocPtr doc, xmlNodePtr cur, PACKAGE &pkg) {
	// Complex tags: tag, dependencies, maintainer etc, are parsed using helper functions.
	// Simple tags parsed inline
	// Some tags will be skipped
	const char *key = (const char *) xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	if (!key) return;
	if (!xmlStrcmp(cur->name, (const xmlChar *) "name")) pkg.set_name(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "version")) pkg.set_version(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "arch")) pkg.set_arch(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "build")) pkg.set_build(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "compressed_size")) pkg.set_compressed_size(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "installed_size")) pkg.set_installed_size(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "short_description")) pkg.set_short_description(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "description")) pkg.set_description(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "changelog")) pkg.set_changelog(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "md5")) pkg.set_md5(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "maintainer")) parseMaintainer(doc, cur, pkg);
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "location")) {
		if (pkg.get_locations().empty()) pkg.get_locations_ptr()->resize(1); // Fail-safe check. Better if it is already filled in with server_url
		pkg.get_locations_ptr()->at(0).set_path(cutSpaces(key));
	}
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "filename")) pkg.set_filename(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "dependencies")) parseDependencies(doc, cur, pkg);
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "tags")) parseTags(doc, cur, pkg);
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "repository_tags")) pkg.set_repository_tags(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "distro_version")) pkg.package_distro_version=cutSpaces(key);
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "provides")) pkg.set_provides(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "conflicts")) pkg.set_conflicts(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "need_special_update")) {
		if (strcmp(key, "yes")==0) pkg.needSpecialUpdate = true;
	}
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "betarelease")) pkg.set_betarelease(cutSpaces(key));
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "type")) {
		if (strcmp(key, "source")==0) pkg.set_type(PKGTYPE_SOURCE);
		else pkg.set_type(PKGTYPE_BINARY);
	}
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "bdelta")) parseBDelta(doc, cur, pkg);
	else if (!xmlStrcmp(cur->name, (const xmlChar *) "config_files")) parseConfigFiles(doc, cur, pkg);
}

void parsePackage(xmlDocPtr doc, xmlNodePtr cur, PACKAGE &pkg) {
	cur = cur->xmlChildrenNode;
	while(cur != NULL) {
		parseXmlTag(doc, cur, pkg);
		cur = cur->next;
	}
}

int getPackageCount(xmlNodePtr cur) {
	cur = cur->xmlChildrenNode;
	int counter = 0;
	while(cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *) "package")) {
			counter++;
		}
		cur = cur->next;
	}
	return counter;
}

int xml2pkglist(xmlDocPtr doc, PACKAGE_LIST &pkgList, const string& server_url) {
	xmlNodePtr cur = xmlDocGetRootElement(doc);

	int package_count = getPackageCount(cur);
	pkgList.clear();
	pkgList.set_size(package_count);
	for (size_t i=0; i<pkgList.size(); ++i) {
		pkgList.get_package_ptr(i)->get_locations_ptr()->resize(1);
		pkgList.get_package_ptr(i)->get_locations_ptr()->at(0).set_server_url(server_url);

	}
	
	cur = cur->xmlChildrenNode;
	int counter = 0;
	while(cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *) "package")) {
			parsePackage(doc, cur, *pkgList.get_package_ptr(counter));
			counter++;
		}
		cur = cur->next;
	}

	return package_count;
}

int xml2pkg(xmlDocPtr doc, xmlNodePtr cur, PACKAGE &pkg) {
	if (!xmlStrcmp(cur->name, (const xmlChar *) "package")) {
		parsePackage(doc, cur, pkg);
		return 0;
	}
	else return 1;

}
