#pragma once
// #include <omp.h>
#include "concurrency.h"
#include "thread"
namespace concurrency {
	size_t numThreads = std::thread::hardware_concurrency() - 1;
}
