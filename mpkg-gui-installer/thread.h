#ifndef ENGINE_THREAD_H_
#define ENGINE_THREAD_H_
#include <QThread>
#include <mpkg/libmpkg.h>

class Thread: public QThread {
	Q_OBJECT
	public:
		Thread(mpkg *);
		~Thread();
	private:
		mpkg *db;
	public slots:
		void run();
	signals:
		void workFinished(bool);

};


#endif

