#include "batchManager.h"



namespace batchManager
{
	mutex m;
	// shader id, textureArray hash, mesh id
	queue< shared_ptr<map<_shader, map<texArray, map<renderingMeta *, Mesh *>>>>> batches;
	// map<_shader,map<texArray,batch>> batches2;
	shared_ptr<map<_shader, map<texArray, map<renderingMeta *, Mesh *>>>> updateBatches()
	{
		// cout << "batching" << endl;
		m.lock();
		auto& batch = batchManager::batches.emplace(make_shared<map<_shader, map<texArray, map<renderingMeta *, Mesh *>>>>());
		m.unlock();
		for (auto &i : renderingManager::shader_model_vector)
		{
			for (auto &j : i.second)
			{
				for (auto &k : j.second->m.meshes())
				{
					(*batch)[j.second->s][k->textures][j.second.get()] = k;

				}
			}
		}
		return batch;
	}
}; // namespace batchManager
// list<renderingMeta*> updatedRenderMetas;
