#ifndef FSYSTEM_H__
#define FSYSTEM_H__
#include "file_routines.h" 
#include <pthread.h>
void system_threaded(const string& cmd);

class PThreadWaiter {
	public:
		PThreadWaiter();
		~PThreadWaiter();
		vector<pthread_t> threads;
		void registerThread(pthread_t id);
		vector<string> cmd_history;
};

#endif
