#include "fsystem.h"
#include <pthread.h>

PThreadWaiter::PThreadWaiter() {
}

PThreadWaiter::~PThreadWaiter() {
	int ret;
	for (size_t i=0; i<threads.size(); ++i) {
		printf("Waiting for %d of %d\n", i, threads.size());
		pthread_join(threads[i], (void **) &ret);
	}
}

void PThreadWaiter::registerThread(pthread_t id) {
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
		perror("fsystem execution failed, running generic system instead");
		system(cmd.c_str());
	}
	else pthreadWaiter.registerThread(thread_id);
}
