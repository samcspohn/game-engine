#pragma once

#include <mutex>
#include "texture.h"
#include "_shader.h"
#include "_renderer.h"


bool operator<(const texArray &l, const texArray &r);
bool operator<(const _shader &l, const _shader &r);
namespace batchManager
{
	extern mutex m;
	// shader id, textureArray hash, mesh id
	extern queue< shared_ptr<map<_shader, map<texArray, map<renderingMeta *, Mesh *>>>>>  batches;
	// extern map<_shader, map<texArray, batch>> batches2;
	// extern map<_shader,map<texArray,batch>> batches2;

	shared_ptr<map<_shader, map<texArray, map<renderingMeta *, Mesh *>>>> updateBatches();
}; // namespace batchManager

