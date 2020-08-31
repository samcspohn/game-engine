#include "game_object.h"

// std::vector<game_object*> toDestroy;
// mutex destroyLock;
tbb::concurrent_vector<game_object*> toDestroy;