
#include "gpu_vector.h"

atomic<int> gpu_vector_base::idGenerator __attribute__ ((init_priority (2000)));;
map<int,gpu_vector_base*> gpu_buffers __attribute__ ((init_priority (2000)));;
