#ifndef ENGINE_H_
#define ENGINE_H_

#include <mpkg/libmpkg.h>
#include <QThread>
#include <QStringList>

class LoadTags: public QThread {
	Q_OBJECT
	public:
		LoadTags(mpkg* _core);
		~LoadTags();
		void run();
		const QStringList& getTagList();
	signals:
		void done();
	private:
		QStringList tagList;
		mpkg* core;
	
};

class LoadPackages: public QThread {
	Q_OBJECT
	public:
		LoadPackages(mpkg* _core);
		~LoadPackages();
		void run();
		const PACKAGE_LIST& getPackageList();
	signals:
		void done();
	private:
		PACKAGE_LIST packageList;
		mpkg* core;
	
};

class LoadUpdateRepositoryData: public QThread {
	Q_OBJECT
	public:
		LoadUpdateRepositoryData(mpkg* _core);
		~LoadUpdateRepositoryData();
		void run();
	signals:
		void done();
	private:
		mpkg *core;

};

class CommitActions: public QThread {
	Q_OBJECT
	public:
		CommitActions(mpkg* _core);
		~CommitActions();
		void run();
	signals:
		void done();
	private:
		mpkg *core;
};
#endif //ENGINE_H_
