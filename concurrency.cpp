#pragma once
// #include <omp.h>
#include "concurrency.h"
#include "thread"
#include <tbb/tbb.h>

namespace concurrency {
	int numThreads = std::thread::hardware_concurrency() - 1;
	tbb::task_scheduler_init tbbinit(concurrency::numThreads);
}
