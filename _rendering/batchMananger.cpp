#include "batchManager.h"



namespace batchManager
{
	mutex m;
	// shader id, textureArray hash, mesh id
	queue<map<_shader, map<texArray, map<renderingMeta *, Mesh *>>>> batches;
	// map<_shader,map<texArray,batch>> batches2;
	map<_shader, map<texArray, map<renderingMeta *, Mesh *>>> &updateBatches()
	{
		// cout << "batching" << endl;
		m.lock();
		batchManager::batches.emplace();
		m.unlock();

		// for(auto &m : toDestroy_models){
		// 	delete modelManager::models_id.at(m);
        //     modelManager::models_id.at(m) = 0;
		// }
		// toDestroy_models.clear();
		// for (auto i : renderingManager::shader_model_vector)
		// {
		// 	for (auto j : i.second)
		// 	{
		// 		if(j.second->m.meta() == 0){
		// 			delete renderingManager::shader_model_vector[i.first][j.first];
		// 			renderingManager::shader_model_vector[i.first].erase(j.first);
		// 		}
		// 	}
		// }
		for (auto &i : renderingManager::shader_model_vector)
		{
			for (auto &j : i.second)
			{
				for (auto &k : j.second->m.meshes())
				{
					batchManager::batches.back()[j.second->s][k->textures][j.second.get()] = k;

				}
			}
		}
		return batchManager::batches.back();

	}
}; // namespace batchManager
// list<renderingMeta*> updatedRenderMetas;
