#include <QThread>
#include <mpkg/libmpkg.h>
#include "custompkgset.h"

class LoadSetupVariantsThread: public QThread {
	Q_OBJECT
	public:
		void run();
		QString detectDVDDevice(QString isofile="");
		QString dvdDevice;
		string volname, rep_location;
		QString pkgsource;
		bool mountDVD(QString device="", QString mountOptions="", bool iso=false);
		bool umountDVD();
		vector<CustomPkgSet> customPkgSetList;

		void calculatePkgSetSize(CustomPkgSet &set);
		void getCustomSetupVariants(const vector<string>& rep_list);

		CustomPkgSet getCustomPkgSet(const string& name);
		string locale;



signals:
		void finished(bool, const vector<CustomPkgSet> &);
		void sendLoadText(const QString &);
		void sendLoadProgress(int);
};
