/* MPKG package system
 * Dependency tracking mechanism
 *
 * This code is licensed under terms of GPL version 3 or later.
 *
 * (c) 2010 Bogdan "aix27249" Kokorev, <i27249@gmail.com>
 */

#ifndef DEPENDENCIES_V2_H_
#define DEPENDENCIES_V2_H_
#include "dataunits.h"
#include "conditions.h"

/* DepReactor: builds dependency tree and renders final action queue
 *
 * Input: 
 * 	full package DB
 * 	pointers to packages which are requested to install
 * 	pointers to packages which are requested to remove
 *
 * Output:
 * 	pointers to packages that should be installed
 * 	pointers to packages that should be removed
 * 	list of errors
 *
 * Input data rules:
 * 	All pointers should point to packages within packagesDB,
 * 	All packages in install queue should be unique within core name and not already installed,
 * 	All packages in remove queue should be unique and already installed.
 * 	
 *
 * Note that DepReactor should receive **clean** queue, prepared by something else.
 * It means that version selection in install queue and dupe cleanup should be performed by another class.
 * WARNING: altrough there will be some sanity checks, do not rely on this: you should ensure that queues are clean BEFORE calling DepReactor
 *
 * If it appears that that combination of packages cannot be installed together, calling class may choose another versions of packages to install and retry.
 *
 * 
 */


class DepReactor {
	public:
		void setInstallQueue(const vector<PACKAGE *> &); // Set user-requested install queue
		void setRemoveQueue(const vector<PACKAGE *> &); // Set user-requested remove queue

		void setPackageDB(const PACKAGE_LIST &); // Set package space to work with. Should include all packages from installQueue and RemoveQueue.
		bool process(); // Start processing. Returns false if errors occured.

		vector<PACKAGE *> getFinalInstallList() const; // Returns final list of packages to install
		vector<PACKAGE *> getFinalRemoveList() const; // Returns final list of packages to remove

		vector<string> getErrorList() const; // Returns human-readable error list

		map<PACKAGE *, PACKAGE *> getDepInstallPaths() const;
		map<PACKAGE *, PACKAGE *> getDepRemovePaths() const;


		void cleanup(); // Cleans everything for future use.

	private:
		vector<PACKAGE *> installQueue, removeQueue;
		PACKAGE_LIST packageDB;

		vector<PACKAGE *> finalInstallList, finalRemoveList;

		vector<string> errorList; // Human-readable error list

		bool sanityCheck(); // Checks input data: should conform all rules

		bool expandInstallLayer(vector<PACKAGE *> masterQueue, vector<PACKAGE *> &slaveInstallQueue, size_t installQueueSize);
		bool expandRemoveLayer(vector<PACKAGE *> masterRemoveQueue, const vector<PACKAGE *>& masterQueue, vector<PACKAGE *> &slaveRemoveQueue, size_t removeQueueSize);
		vector<PACKAGE *> replaceScanner(vector<PACKAGE *> masterQueue);

		map<PACKAGE *, PACKAGE *> depInstallPaths, depRemovePaths;
};

#endif //DEPENDENCIES_V2_H_

