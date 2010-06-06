#include "fsystem.h"
#include <pthread.h>
#include <iostream>
PThreadWaiter::PThreadWaiter() {
}

PThreadWaiter::~PThreadWaiter() {
	int ret;
	for (size_t i=0; i<threads.size(); ++i) {
		cout << "Waiting threads to finish: " << i << " of " << threads.size() << endl;
		pthread_join(threads[i], (void **) &ret);
	}
}

void PThreadWaiter::registerThread(pthread_t id) {
	// Let's limit number of opened threads to 7
	
	int ret;
	while (threads.size()>=4) { // 4 will be enough for most systems
		pthread_join(threads[0], (void **) &ret);
		threads.erase(threads.begin());
	}
	threads.push_back(id);
}

PThreadWaiter pthreadWaiter;
void *system_routine(void *arg) {
	string data = (const char *) arg;
	//printf("\n\nFSYSTEM: %s\n\n", data.c_str());
	int ret = system(data.c_str());
	pthread_exit((void *) ret);
}
void system_threaded(const string& cmd) {
	pthread_t thread_id;
	const char *arg = cmd.c_str();
	if (pthread_create(&thread_id, NULL, system_routine, (void *) arg)) {
		//perror("fsystem execution failed, running generic system instead");
		system(cmd.c_str());
	}
	else pthreadWaiter.registerThread(thread_id);
}
