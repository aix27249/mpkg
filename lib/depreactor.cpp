/* MPKG package system
 * Dependency tracking mechanism v.4
 *
 * This code is licensed under terms of GPL version 3 or later.
 *
 * (c) 2010 Bogdan "aix27249" Kokorev, <i27249@gmail.com>
 */


#include "depreactor.h"
#include <mpkgsupport/mpkgsupport.h>


// DepReactor: public API

// Data import

void DepReactor::setInstallQueue(const vector<PACKAGE *> &p) {
	installQueue = p;
}

void DepReactor::setRemoveQueue(const vector<PACKAGE *> &p) {
	removeQueue = p;
}

void DepReactor::setPackageDB(const PACKAGE_LIST &p) {
	packageDB = p;
	// Let's init versioning (we will need it), and do that even if there was such init before: safety costs more than half a second to process.
	packageDB.initVersioning();
}

// Data export

vector<PACKAGE *> DepReactor::getFinalInstallList() const {
	return finalInstallList;
}

vector<PACKAGE *> DepReactor::getFinalRemoveList() const {
	return finalRemoveList;
}

vector<string> DepReactor::getErrorList() const {
	return errorList;
}

// Maintenance routine

void DepReactor::cleanup() {
	installQueue.clear();
	removeQueue.clear();
	packageDB.clear();
	finalInstallList.clear();
	finalRemoveList.clear();
	errorList.clear();
}

// Main function: processing
bool DepReactor::process() {
	// Process install queue.
	errorList.clear(); // Clean error list: fresh run
	vector<PACKAGE *> masterQueue, slaveInstallQueue;
	masterQueue = installQueue;
	bool res = true;

	// Walk thru while no errors and while slaveInstallQueue grows.
	while (res) {
		res = expandInstallLayer(masterQueue, slaveInstallQueue, installQueue.size());
		if (!res) break;
		if (slaveInstallQueue.empty()) break;
		for (size_t i=0; i<slaveInstallQueue.size(); ++i) {
			masterQueue.push_back(slaveInstallQueue[i]);
		}
		slaveInstallQueue.clear();
	}
	// At this point, masterQueue is everything that should be installed.
	
	// Now, let's get with removing.
	// First of all, check what we are replacing.
	vector<PACKAGE *> removeByReplace = replaceScanner(masterQueue);

	// Now let's process remove queue.
	vector<PACKAGE *> masterRemoveQueue, slaveRemoveQueue, stageRemoveQueue;
	masterRemoveQueue = removeQueue;

	// Merge removeByReplace with masterRemoveQueue
	res = false;
	for (size_t i=0; !res && i<removeByReplace.size(); ++i) {
		for (size_t t=0; t<masterRemoveQueue.size(); ++t) {
			if (removeByReplace[i]==masterRemoveQueue[t]) res = true;
		}
		if (!res) masterRemoveQueue.push_back(removeByReplace[i]);
	}

	res = true;
	int cnt = 0;
	size_t prevQueueSize = 0;
	while (res) {
		res = expandRemoveLayer(masterRemoveQueue, masterQueue, slaveRemoveQueue, prevQueueSize);
		if (!res) break;
		if (slaveRemoveQueue.empty()) break;
		prevQueueSize = masterRemoveQueue.size();
		for (size_t i=0; i<slaveRemoveQueue.size(); ++i) {
			masterRemoveQueue.push_back(slaveRemoveQueue[i]);
		}
		slaveRemoveQueue.clear();
		
		cnt++;
		printf("\nLoop %d\n", cnt);
	}

	// Debug section
	finalInstallList = masterQueue;
	finalRemoveList = masterRemoveQueue;
	return true;
}

bool DepReactor::expandInstallLayer(vector<PACKAGE *> masterQueue, vector<PACKAGE *> &slaveInstallQueue, size_t installQueueSize) {
	// Some helper variables
	bool loopStop, loopStop2;
	DEPENDENCY *dep;
	PACKAGE *depPkg;

	// Functional programming wins :)
	//
	// Go walk thru install queue: resolve dependencies and conflicts
	for (size_t i=0; i<masterQueue.size(); ++i) {
		// Looking for dependendy resolutions
		for (size_t d=0; d<masterQueue[i]->get_dependencies().size(); ++d) {
			dep = &masterQueue[i]->get_dependencies_ptr()->at(d);
			// 1: look inside installed packages
			loopStop = false;
			for (size_t t=0; t<packageDB.size(); ++t) {
				if (!packageDB[t].installed()) continue;
				if (dep->isResolvableBy(packageDB[t])) {
					loopStop=true;
					break;
				}
			}
			if (loopStop) continue; // Found in installed - ok
		
			// 2: look inside install queue
			loopStop = false;
			for (size_t t=0; t<masterQueue.size(); ++t) {
				if (dep->isResolvableBy(*masterQueue[t])) {
					loopStop=true;
					break;
				}
			}
			if (loopStop) continue; // Found in install queue - ok

			// 3: look into slave queue. Maybe some package already requested that package.
			loopStop = false;
			for (size_t t=0; t<slaveInstallQueue.size(); ++t) {
				if (dep->isResolvableBy(*slaveInstallQueue[t])) {
					loopStop=true;
					break;
				}
			}
			if (loopStop) continue; // Found in slave install queue - ok

			// 4: look in not installed packages. Considering all packages are reachable.
			loopStop = false;
			for (size_t t=0; t<packageDB.size(); ++t) {
				if (packageDB[t].installed()) continue;
				if (dep->isResolvableBy(packageDB[t])) {
					// Possible situation: package with same name is already in queue, but has different version.
					// Either new candidate also satisfies all previous requirements, or we should look for another one.
					// For this, we need to know which packages requires our candidate.
					loopStop2 = false;
					depPkg = NULL;
					for (size_t d=installQueueSize; !depPkg && d<masterQueue.size(); ++d) {
						if (packageDB[t].get_corename()==masterQueue[d]->get_corename()) {
							depPkg = masterQueue[d];
							break;
						}
					}
					for (size_t d=0; !depPkg && d<slaveInstallQueue.size(); d++) {
						if (packageDB[t].get_corename()==slaveInstallQueue[d]->get_corename()) {
							depPkg = slaveInstallQueue[d];
							break;
						}
					}

					// If depPkg, check if we can replace it.
					// Check if new one satisfies all another ones
					for (size_t s=0; depPkg && !loopStop2 && s<masterQueue.size(); ++s) {
						for (size_t ds=0; !loopStop2 && ds<masterQueue[s]->get_dependencies().size(); ++ds) {
							if (masterQueue[s]->get_dependencies().at(ds).get_package_name()!=depPkg->get_corename()) continue;
							if (!masterQueue[s]->get_dependencies().at(ds).isResolvableBy(packageDB[t])) loopStop2 = true;
						}
					}
					
					if (depPkg && !loopStop2) {
						loopStop=true;
						for (size_t d=0; d<masterQueue.size(); ++d) {
							if (masterQueue[d]==depPkg) masterQueue[d]=packageDB.get_package_ptr(t);
						}
						for (size_t d=0; slaveInstallQueue.size(); ++d) {
							if (slaveInstallQueue[d]==depPkg) slaveInstallQueue[d]=packageDB.get_package_ptr(t);
						}
						break;
					}
					if (!depPkg) {
						loopStop = true;
						slaveInstallQueue.push_back(packageDB.get_package_ptr(t)); 
						break;
					}
				}
			}

			// Bad situation: if nothing resolves our dependencies
			if (!loopStop) {
				errorList.push_back(masterQueue[i]->get_name() + " " + masterQueue[i]->get_fullversion() + \
						_(" has broken dependency: ") + dep->getDepInfo());
			}
		}
	}
	return true;
}


vector<PACKAGE *> DepReactor::replaceScanner(vector<PACKAGE *> masterQueue) {
	vector<PACKAGE *> removeByReplace;
	// Here we should do:
	// 1. Check for upgrades and provides. Do not care about revdeps here, it will be done in next part.
	
	for (size_t i=0; i<masterQueue.size(); ++i) {
		for (size_t t=0; t<packageDB.size(); ++t) {
			if (!packageDB[t].installed()) continue; // We need only installed packages. Really.
			if (masterQueue[i]->get_corename()==packageDB[t].get_corename()) {
				// Sir, let's gtfo
				removeByReplace.push_back(packageDB.get_package_ptr(t));
			}
		}
	}

	vector<PACKAGE *> conflictRemove;
	// 2. Now, check for conflicts. Note that conflicts may be from both sides: masterQueue and packageDB.
	for (size_t i=0; i<masterQueue.size(); ++i) {
		if (masterQueue[i]->conflicts.empty() || masterQueue[i]->conflicts=="0") continue; // No conflicts - no check.
		for (size_t t=0; t<packageDB.size(); ++t) {
			if (masterQueue[i]->conflicts==packageDB[t].get_corename()) {
				conflictRemove.push_back(packageDB.get_package_ptr(t));
			}
		}
	}

	for (size_t i=0; i<packageDB.size(); ++i) {
		if (packageDB[i].conflicts.empty() || packageDB[i].conflicts=="0") continue; // No conflicts - no check.
		for (size_t t=0; t<masterQueue.size(); ++t) {
			if (packageDB[i].conflicts==masterQueue[t]->get_corename()) {
				conflictRemove.push_back(packageDB.get_package_ptr(i)); // Master queue is so master.
			}
		}
	}

	// Merge conflictRemove with removeByReplace and return it.
	bool dupe;
	for (size_t i=0; i<conflictRemove.size(); ++i) {
		dupe = false;
		for (size_t t=0; !dupe && t<removeByReplace.size(); ++t) {
			if (conflictRemove[i]==removeByReplace[t]) dupe = true;
		}
		if (!dupe) removeByReplace.push_back(conflictRemove[i]);
	}

	return removeByReplace;
}



bool DepReactor::expandRemoveLayer(vector<PACKAGE *> masterRemoveQueue, const vector<PACKAGE *>& masterQueue, vector<PACKAGE *> &slaveRemoveQueue, size_t prevQueueSize) {
	bool resolvable, already;
	DEPENDENCY *dep;
	size_t loopCounter = 0;
	string corename;
	// Expand layer. Just do it.
	// Going thru masterQueue and look if any package in packageDB 
	for (size_t i=prevQueueSize; i<masterRemoveQueue.size(); ++i) {
		for (size_t t=0; t<packageDB.size(); ++t) {

			if (!packageDB[t].installed()) continue;

			// Check if package is already in masterRemoveQueue
			already = false;
			for (size_t d=0; !already && d<masterRemoveQueue.size(); ++d) {
				if (masterRemoveQueue[d]==packageDB.get_package_ptr(t)) already = true;
			}
			if (already) continue;
			
			for (size_t d=0; d<packageDB[t].get_dependencies().size(); ++d) {
				dep = &packageDB.get_package_ptr(t)->get_dependencies_ptr()->at(d);
				corename = masterRemoveQueue[i]->get_corename();

				loopCounter++;
				if (dep->get_package_name()==corename) {
					printf("Possible: %s\n", corename.c_str());
					// Possible candidate to remove. Check if it's deps can be fixed with masterQueue
					resolvable = false;
					for (size_t m=0; !resolvable && m<masterQueue.size(); ++m) {
						if (dep->isResolvableBy(*masterQueue[m])) resolvable=true;
					}
					if (!resolvable) {
						//printf("Unres: %s: %s\n", packageDB[t].get_name().c_str(), dep->get_package_name().c_str());
						already = false;
						for (size_t m=0; !already && m<slaveRemoveQueue.size(); ++m) {
							if (slaveRemoveQueue[m]==packageDB.get_package_ptr(t)) already = true;
						}

						for (size_t m=0; !already && m<masterRemoveQueue.size(); ++m) {
							if (masterRemoveQueue[m]==packageDB.get_package_ptr(t)) already = true;
						}

						if (!already) slaveRemoveQueue.push_back(packageDB.get_package_ptr(t));
						break;
					}
				}
			}
		}
	}
	printf("Iterations: %d\n", loopCounter);

	return true;
}
