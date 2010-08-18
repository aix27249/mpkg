#ifndef XML2PKGLIST_H__
#define XML2PKGLIST_H__
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "dataunits.h"

// Receives XML doc, root node, link to package list and server url
int xml2pkglist(xmlDocPtr doc, PACKAGE_LIST &pkgList, const string& server_url);
// Same thing for single package. Creates doc on the fly
int xml2pkg(xmlDocPtr doc, xmlNodePtr cur, PACKAGE &pkg);

#endif
