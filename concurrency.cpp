#pragma once
// #include <omp.h>
#include "concurrency.h"
#include <iostream>
#include <sched.h>

namespace concurrency {
#define not_threads 0
	int numThreads = std::thread::hardware_concurrency() - not_threads;
	tbb::task_scheduler_init tbbinit(concurrency::numThreads);
	pinning_observer pinningObserver(1);

#define USE_TASK_ARENA_CURRENT_SLOT 1
	pinning_observer::pinning_observer( int pinning_step=1 ) : pinning_step(pinning_step), thread_index() {
		for ( ncpus = sizeof(cpu_set_t)/CHAR_BIT; ncpus < 16*1024 /* some reasonable limit */; ncpus <<= 1 ) {
			mask = CPU_ALLOC( ncpus );
			if ( !mask ) break;
			const size_t size = CPU_ALLOC_SIZE( ncpus );
			CPU_ZERO_S( size, mask );
			const int err = sched_getaffinity( 0, size, mask );
			if ( !err ) break;

			CPU_FREE( mask );
			mask = NULL;
			if ( errno != EINVAL )  break;
		}
		if ( !mask )
			std::cout << "Warning: Failed to obtain process affinity mask. Thread affinitization is disabled." << std::endl;
	}

/*override*/ void pinning_observer::on_scheduler_entry( bool ) {
	if ( !mask ) return;

	const size_t size = CPU_ALLOC_SIZE( ncpus );
	const int num_cpus = CPU_COUNT_S( size, mask );
	int thr_idx =
#if USE_TASK_ARENA_CURRENT_SLOT
		tbb::task_arena::current_thread_index();
		// tbb::task_arena::current_slot();
#else
		thread_index++;
#endif
#if __MIC__
	thr_idx += 1; // To avoid logical thread zero for the master thread on Intel(R) Xeon Phi(tm)
#endif
	// thr_idx %= num_cpus; // To limit unique number in [0; num_cpus-1] range

	// 	// Place threads with specified step
	// 	int cpu_idx = 0;
	// 	for ( int i = 0, offset = 0; i<thr_idx; ++i ) {
	// 		cpu_idx += pinning_step;
	// 		if ( cpu_idx >= num_cpus )
	// 			cpu_idx = ++offset;
	// 	}

	// 	// Find index of 'cpu_idx'-th bit equal to 1
	// 	int mapped_idx = -1;
	// 	while ( cpu_idx >= 0 ) {
	// 		if ( CPU_ISSET_S( ++mapped_idx, size, mask ) )
	// 			--cpu_idx;
	// 	}

		cpu_set_t *target_mask = CPU_ALLOC( ncpus );
		CPU_ZERO_S( size, target_mask );
		for(int i = not_threads; i < numThreads + not_threads; i++){
			CPU_SET_S( i, size, target_mask );
		}
		const int err = sched_setaffinity( 0, size, target_mask );

		if ( err ) {
			std::cout << "Failed to set thread affinity!n";
			exit( EXIT_FAILURE );
		}
		pthread_setschedprio(pthread_self(),99);
		std::cout << "setting affinity for thread: " << thr_idx << std::endl;
#if LOG_PINNING
		else {
			std::stringstream ss;
			ss << "Set thread affinity: Thread " << thr_idx << ": CPU " << mapped_idx << std::endl;
			std::cerr << ss.str();
		}
#endif
		CPU_FREE( target_mask );
	}

	pinning_observer::~pinning_observer() {
		if ( mask )
			CPU_FREE( mask );
	}

}
