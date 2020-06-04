#pragma once
#include "gpu_vector.h"

atomic<int> gpu_vector_base::idGenerator;
map<int,gpu_vector_base*> gpu_buffers;
