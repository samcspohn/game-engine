#pragma once
#include <omp.h>
#include "concurrency.h"
#include "thread"
namespace concurrency {
	size_t numThreads = omp_get_max_threads();
	// size_t numThreads = ([](){size_t nthreads = std::thread::hardware_concurrency(); 
	// if(nthreads <= 8)
	// 	return nthreads - 1;
	// else
	// 	return nthreads - 2;
	// })();
	// size_t numThreads = 1;
}
