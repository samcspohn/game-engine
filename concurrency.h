#pragma once

#include <thread>
namespace concurrency {
	size_t numThreads = ([](){size_t nthreads = thread::hardware_concurrency(); 
	if(nthreads <= 8)
		return nthreads - 1;
	else
		return nthreads - 2;
	})();
	// size_t numThreads = 1;
}
