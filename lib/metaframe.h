#ifndef METAFRAME_H__
#define METAFRAME_H__
#include "local_package.h"
class SourcePackage;
class MetaPackage {
	public:
		MetaPackage(const string& _f);
		~MetaPackage();
		PACKAGE* data;
		bool saveToXML(const string& xmlFilename);
		bool savePackage();
		string pkgFilename;
	private:
		LocalPackage *lp;
};

class MetaSrcPackage {
	public:
		MetaSrcPackage(string _f="");
		~MetaSrcPackage();
		SPKG *data;
		bool saveToXML(const string& xmlFilename);
		bool savePackage(string saveTo="");
		string pkgFilename;
		SourcePackage *sp;
};
#endif
