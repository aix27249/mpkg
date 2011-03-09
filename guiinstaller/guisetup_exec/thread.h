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
		void setProgressCall(int progress);
		void sendReportError(const string& text);

	public slots:
		void skipMD5();

		MpkgErrorReturn errorHandler(ErrorDescription err, const string& details);
		void receiveErrorResponce(MpkgErrorReturn);

		void updateData(const ItemState& a);

	private:
		MpkgErrorReturn errCode;
		AgiliaSetup agiliaSetup;
		void parseConfig(map<string, string> *_strSettings, vector<TagPair> *_users, vector<PartConfig> *_partConfigs, vector<string> *_additional_repositories);


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



};

#endif
