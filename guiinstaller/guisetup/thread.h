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


signals:
		void finished(bool);
};
