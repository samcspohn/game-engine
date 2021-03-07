#include "_rendering/_model.h"
#include "_rendering/_shader.h"
#include "../fast_list.h"
#include "../array_heap.h"
#include <mutex>
#include <vector>
#include <list>
#include <limits>
struct renderingMeta
{
	// gpu_vector<__renderer>* __renderers;
	// deque_heap<GLuint> ids;
	fast_list_deque<GLuint> ids;
	// list<GLuint> ids;
	// deque<GLuint> ids_cache;

	_shader s;
	_model m;
	float minRadius = 0.f;
	float maxRadius = std::numeric_limits<float>::max();
	uint isBillboard = 0;
	renderingMeta(_shader _s, _model _m);

private:
	renderingMeta(const renderingMeta &other);
};


namespace renderingManager
{
	extern mutex m;

	extern map<int, map<int, renderingMeta *>> shader_model_vector;
	void destroy();
	void lock();
	void unlock();
}; // namespace renderingManager