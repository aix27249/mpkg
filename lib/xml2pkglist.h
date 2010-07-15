#ifndef XML2PKGLIST_H__
#define XML2PKGLIST_H__
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "dataunits.h"

// Receives XML doc, root node, link to package list and server url
int xml2pkglist(xmlDocPtr doc, PACKAGE_LIST &pkgList, const string& server_url);

#endif
