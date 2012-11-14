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
	// Let's limit number of opened threads to 4
	
	int ret;
	while (threads.size()>=4) { // 4 will be enough for most systems. Greater values often gives more speed, but can cause out-of-memory on some systems.
		pthread_join(threads[0], (void **) &ret);
		threads.erase(threads.begin());
	}
	threads.push_back(id);
}

PThreadWaiter pthreadWaiter;
void *system_routine(void *arg) {
	string data = (const char *) arg;
	size_t ret = system((const char *) arg);
	pthread_exit((void *) ret);
}
void system_threaded(const string& cmd) {
	pthreadWaiter.cmd_history.push_back(cmd);
	pthread_t thread_id;
	const char *arg = pthreadWaiter.cmd_history[pthreadWaiter.cmd_history.size()-1].c_str();
	if (pthread_create(&thread_id, NULL, system_routine, (void *) arg)) {
		//perror("fsystem execution failed, running generic system instead");
		system(cmd.c_str());
	}
	else pthreadWaiter.registerThread(thread_id);
}
