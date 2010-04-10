/******************************************************
 * Converter for legacy Slackware packages
 * $Id: converter.h,v 1.4 2007/11/28 02:24:25 i27249 Exp $
 * ****************************************************/
#ifndef CONVERTER_H_
#define CONVERTER_H_
#include "debug.h"
#include "mpkg.h"
#include "repository.h"
int slack_convert(const string& filename, const string& xml_output);
int convert_package(const string& filename, const string& output_dir);
int tag_package(const string& filename, const string& tag, bool clear_other);
int buildup_package(const string& filename);
int setver_package(const string& filename, const string& version);
#endif
