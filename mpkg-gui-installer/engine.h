#ifndef ENGINE_H_
#define ENGINE_H_
#include "ui_engine.h"
#include <QtGui>
#include <mpkg/libmpkg.h>

#include <mpkg/errorhandler.h>
#include "progress.h"
#include "thread.h"
class EngineWindow: public QWidget {
	Q_OBJECT
	public:
		EngineWindow();
		~EngineWindow();
		Ui::engineWindow ui;
	public slots:
		void commit();
		void cancel();
		void prepareData(int argc, char **argv);
		void finish(bool);
		MpkgErrorReturn errorHandler(ErrorDescription err, const string& details);
	private:
		mpkg *db;
		Thread *threadObject;
		ProgressWindow *progressWindow;

};
#endif
