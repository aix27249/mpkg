#include "dbstruct.h"
string getDBStructure()
{
	return "----------------------------------------------------------------------\n\
--\n\
--	MPKG package system\n\
--	Database creation script\n\
--	$Id: dbstruct.cpp,v 1.3 2007/11/02 20:19:45 i27249 Exp $\n\
--\n\
----------------------------------------------------------------------\n\
\n\
create table packages (\n\
	package_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
	package_name TEXT NOT NULL,\n\
	package_version TEXT NOT NULL,\n\
	package_arch TEXT NOT NULL,\n\
	package_build TEXT NULL,\n\
	package_compressed_size TEXT NOT NULL,\n\
	package_installed_size TEXT NOT NULL,\n\
	package_short_description TEXT NULL,\n\
	package_description TEXT NULL, \n\
	package_changelog TEXT NULL,\n\
	package_packager TEXT NULL,\n\
	package_packager_email TEXT NULL,\n\
	package_installed INTEGER NOT NULL,\n\
	package_configexist INTEGER NOT NULL,\n\
	package_action INTEGER NOT NULL,\n\
	package_md5 TEXT NOT NULL,\n\
	package_filename TEXT NOT NULL,\n\
	package_betarelease TEXT NOT NULL,\n\
	package_installed_by_dependency INTEGER NOT NULL DEFAULT '0',\n\
	package_type INTEGER NOT NULL DEFAULT '0',\n\
	package_add_date INTEGER NOT NULL DEFAULT '0',\n\
	package_build_date INTEGER NOT NULL DEFAULT '0',\n\
	package_repository_tags TEXT NULL, \n\
	package_distro_version TEXT NULL, \n\
	package_provides TEXT NULL, \n\
	package_conflicts TEXT NULL \n\
);\n\
create index ppname on packages (package_id, package_name, package_version, package_action, package_installed, package_md5);\n\
\n\
create table files (\n\
	file_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
	file_name TEXT NOT NULL,\n\
	file_type INTEGER NOT NULL,\n\
	packages_package_id INTEGER NOT NULL\n\
);\n\
create index pname on files (file_name, packages_package_id);\n\
\n\
create table conflicts (\n\
	conflict_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
	conflict_file_name TEXT NOT NULL,\n\
	backup_file TEXT NOT NULL,\n\
	conflicted_package_id INTEGER NOT NULL\n\
);\n\
\n\
create table locations (\n\
	location_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
	packages_package_id INTEGER NOT NULL,\n\
	server_url TEXT NOT NULL,\n\
	location_path TEXT NOT NULL\n\
);\n\
create index locpid on locations(packages_package_id, location_path, server_url);\n\
create table tags (\n\
	tags_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
	tags_name TEXT NOT NULL\n\
);\n\
create index ptag on tags (tags_id, tags_name);\n\
\n\
create table tags_links (\n\
	tags_link_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
	packages_package_id INTEGER NOT NULL,\n\
	tags_tag_id INTEGER NOT NULL\n\
);\n\
create index ptaglink on tags_links (packages_package_id, tags_tag_id);\n\
\n\
create table dependencies (\n\
	dependency_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
	packages_package_id INTEGER NOT NULL,\n\
	dependency_condition INTEGER NOT NULL DEFAULT '1',\n\
	dependency_type INTEGER NOT NULL DEFAULT '1',\n\
	dependency_package_name TEXT NOT NULL,\n\
	dependency_package_version TEXT NULL,\n\
	dependency_build_only INTEGER NOT NULL DEFAULT '0' \
);\n\
\n\
create index pdeps on dependencies (packages_package_id, dependency_id, dependency_package_name, dependency_package_version, dependency_condition);\n\
\n\
create table history (\n\
	history_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
	history_event INTEGER NOT NULL,\n\
	history_data TEXT NULL\n\
);\n\
create table deltas (\n\
	delta_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
	packages_package_id INTEGER NOT NULL,\n\
	delta_url TEXT NOT NULL,\n\
	delta_md5 TEXT NOT NULL,\n\
	delta_orig_filename TEXT NOT NULL,\n\
	delta_orig_md5 TEXT NOT NULL,\n\
	delta_size TEXT NULL\n\
);\n\
-- INTERNATIONAL SUPPORT\n\
\n\
--create table descriptions (\n\
--	description_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
--	packages_package_id INTEGER NOT NULL,\n\
--	description_language TEXT NOT NULL,\n\
--	description_text TEXT NOT NULL,\n\
--	short_description_text TEXT NOT NULL\n\
--);\n\
\n\
--create table changelogs (\n\
--	changelog_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
--	packages_package_id INTEGER NOT NULL,\n\
--	changelog_language TEXT NOT NULL,\n\
--	changelog_text TEXT NOT NULL\n\
--);\n\
\n\
-- RATING SYSTEM - SUPPORT FOR FUTURE\n\
--create table ratings (\n\
--	rating_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,\n\
--	rating_value INTEGER NOT NULL,\n\
--	packages_package_name TEXT NOT NULL\n\
--);\n\
";
}
