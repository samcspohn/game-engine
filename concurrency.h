#pragma once
#include "thread"
#include <tbb/tbb.h>

namespace concurrency {
	extern int numThreads;
	// class pinning_observer: public tbb::task_scheduler_observer {
	// 	cpu_set_t *mask;

	// 	const int pinning_step;
	// 	tbb::atomic<int> thread_index;
	// public:
	// 	int ncpus;
	// 	pinning_observer( int pinning_step=1 );
	// /*override*/ void on_scheduler_entry( bool );

	// 	~pinning_observer();
	// };
	// extern pinning_observer pinningObserver;
}
