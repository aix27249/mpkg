#ifndef GUISETUP_THREAD_H__
#define GUISETUP_THREAD_H__
#include <QtGui>
#include <QThread>
#include <mpkg/libmpkg.h>
#include <agiliasetup.h>

#include <mpkg/errorhandler.h>




class SetupThread: public QThread, public StatusNotifier {
	Q_OBJECT
	public:
		void run();
		void setDetailsTextCall(const string& msg);
		void setSummaryTextCall(const string& msg);
		void sendReportError(const string& text);
	private:
		// Core
		mpkg *core;
		QSettings *settings;


		MpkgErrorReturn errCode;
		AgiliaSetup agiliaSetup;


	signals:
		void setSummaryText(const QString &);
		void setDetailsText(const QString &);
		void setProgressMax(int);
		void setProgress(int);
		void reportError(const QString &);
		void reportFinish();
		void minimizeWindow();
		void maximizeWindow();
		void showMD5Button(bool);
		void enableMD5Button(bool);
		void sendErrorHandler(ErrorDescription, const QString &);

	public slots:
		void skipMD5();

		MpkgErrorReturn errorHandler(ErrorDescription err, const string& details);
		void receiveErrorResponce(MpkgErrorReturn);

		bool fillPartConfigs();
		bool validateConfig();
		bool setMpkgConfig();
		bool getRepositoryData();
		bool prepareInstallQueue();
		bool validateQueue();
		bool formatPartitions();
		bool mountPartitions();
		bool moveDatabase();
		bool createBaselayout();
		bool processInstall();
		bool postInstallActions();
		void updateData(const ItemState& a);


};

#endif
