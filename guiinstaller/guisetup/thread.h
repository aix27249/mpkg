#include <QThread>
#include <mpkg/libmpkg.h>

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

		void getCustomSetupVariants(const vector<string>& rep_list);
	
		string locale;



signals:
		void finished(bool, const vector<CustomPkgSet> &);
		void sendLoadText(const QString &);
		void sendLoadProgress(int);
};
