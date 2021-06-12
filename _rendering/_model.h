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
	void getBounds();
	void inspect();

	string file;
	shared_ptr<Model> model = 0;
	glm::vec3 bounds;
	float radius;
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
	extern map<size_t, int> models;
	extern map<int, shared_ptr<_modelMeta>> models_id;
	void destroy();
	void save(OARCHIVE &oa);
	void load(IARCHIVE &ia);
	void encode(YAML::Node &node);
	void decode(YAML::Node &node);
	void _new();
	void init();
}; // namespace modelManager

class _model
{
public:
	_model();
	_model(string fileName);
	~_model();
	void destroy();
	vector<Mesh *> &meshes();
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

namespace YAML
{
	template <>
	struct convert<_model>
	{
		static Node encode(const _model &rhs);

		static bool decode(const Node &node, _model &rhs);
	};

	template <>
	struct convert<_modelMeta>
	{
		static Node encode(const _modelMeta &rhs);

		static bool decode(const Node &node, _modelMeta &rhs);
	};
}