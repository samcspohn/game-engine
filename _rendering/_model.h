#pragma once
#include <map>
#include <string>
#include "Model.h"
#include "serialize.h"
#include "editor.h"
#include "Transform.h"
// model data
class _model;
struct _modelMeta : public assets::asset
{
	_modelMeta();
	_modelMeta(string file);
	~_modelMeta();
	bool onEdit();
	string type();
	string file;
	unique_ptr<Model> model = 0;
	glm::vec3 bounds;
	float radius;
	void getBounds();
	void inspect();
	bool unique = false;
	friend class _model;
	SER_HELPER()
	{
		SER_BASE_ASSET
		ar &name &model &bounds &radius &unique;
	}
};
namespace modelManager
{
	extern map<size_t, shared_ptr<_modelMeta>> models;
	extern map<int, shared_ptr<_modelMeta>> models_id;
	void destroy();
	void save(OARCHIVE &oa);
	void load(IARCHIVE &ia);
	void _new();
}; // namespace modelManager

class _model
{
public:
	_model();
	_model(string fileName);
	~_model();
	void destroy();
	vector<Mesh*> &meshes();
	Mesh &mesh();
	void makeUnique();
	void makeProcedural();
	void recalcBounds();
	_modelMeta *meta() const;
	// _modelMeta* m = 0;
	int m = 0;
	SER_HELPER()
	{
		ar &m;
	}
};

extern atomic<int> uniqueMeshIdGenerator;
transform2 renderRaycast(glm::vec3 p, glm::vec3 dir);