#pragma once

#include <thread>
namespace concurrency {
	size_t numThreads = thread::hardware_concurrency() - 2;
//	size_t numThreads = 1;
}
