#include "_rendering/_renderMeta.h"

renderingMeta::renderingMeta(_shader _s, _model _m)
{
	s = _s;
	m = _m;
	if (m.meta() == 0)
	{
		model_manager.meta[m.m] = make_unique<_modelMeta>();
		string idStr = {(char)(m.m >> 24), (char)(m.m >> 16), (char)(m.m >> 8), (char)m.m, 0};
		model_manager.meta[m.m]->name = idStr;
		model_manager.meta[m.m]->unique = true;
	}
}
renderingMeta::renderingMeta(const renderingMeta &other) {}

namespace renderingManager
{
	mutex m;
	map<int, map<int, unique_ptr<renderingMeta>>> shader_model_vector;
	void destroy()
	{
		shader_model_vector.clear();
		// while (shader_model_vector.size() > 0)
		// {
		// 	while (shader_model_vector.begin()->second.size() > 0)
		// 	{
		// 		// delete shader_model_vector.begin()->second.begin()->second;
		// 		shader_model_vector.begin()->second.erase(shader_model_vector.begin()->second.begin());
		// 	}
		// 	shader_model_vector.erase(shader_model_vector.begin());
		// }
	}
	void lock()
	{
		m.lock();
	}
	void unlock()
	{
		m.unlock();
	}
}; // namespace renderingManager

bool operator<(const texArray &l, const texArray &r)
{
	return l.hash < r.hash;
}
