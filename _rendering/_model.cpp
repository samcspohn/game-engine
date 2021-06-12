#include "_model.h"
#include <imgui/imgui.h>
atomic<int> uniqueMeshIdGenerator{1};

namespace YAML
{
	Node convert<_model>::encode(const _model &rhs)
	{
		Node node;
		node["id"] = rhs.m;
		return node;
	}

	bool convert<_model>::decode(const Node &node, _model &rhs)
	{
		rhs.m = node["id"].as<int>();
		return true;
	}

	Node convert<_modelMeta>::encode(const _modelMeta &rhs)
	{
		// model &bounds &radius &unique;
		Node node;
		YAML_ENCODE_ASSET();
		node["model"] = *rhs.model;
		node["file"] = rhs.file;
		node["bounds"] = rhs.bounds;
		node["radius"] = rhs.radius;
		node["unique"] = rhs.unique;
		return node;
	}

	bool convert<_modelMeta>::decode(const Node &node, _modelMeta &rhs)
	{
		YAML_DECODE_ASSET();
		rhs.model = make_unique<Model>(node["model"].as<Model>());
		rhs.file = node["file"].as<string>();
		rhs.bounds = node["bounds"].as<glm::vec3>();
		rhs.radius = node["radius"].as<float>();
		rhs.unique = node["unique"].as<bool>();
		return true;
	}
}

_modelMeta::_modelMeta()
{
	model = make_unique<Model>();
	getBounds();
}
_modelMeta::_modelMeta(string _file)
{
	file = _file;
	model = make_unique<Model>(file);
	getBounds();
}
_modelMeta::~_modelMeta()
{
	// delete model;
}

bool _modelMeta::onEdit()
{
	// if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	// {
	// 	// Set payload to carry the index of our item (could be anything)
	// 	ImGui::SetDragDropPayload("MODEL_DRAG_AND_DROP", &id, sizeof(int));
	// 	ImGui::EndDragDropSource();
	// }
	return false;
}
string _modelMeta::type()
{
	return "MODEL_DRAG_AND_DROP";
}
REGISTER_ASSET(_modelMeta);

namespace modelManager
{
	map<size_t, int> models;
	map<int, shared_ptr<_modelMeta>> models_id;
	// map<int, _modelMeta *> unique_models;
	void destroy()
	{
		models.clear();
		models_id.clear();
	}

	void save(OARCHIVE &oa)
	{
		oa << models << models_id;
	}
	void load(IARCHIVE &ia)
	{
		ia >> models >> models_id;
		waitForRenderJob([&]()
		{
			for (auto &m : models_id)
			{
				m.second->model->loadModel();
			}
		});
	}
	void encode(YAML::Node &node)
	{
		YAML::Node models_ids_node;
		// YAML::Node models_node;
		for (auto &i : models_id)
		{
			models_ids_node[i.first] = *i.second;
		}
		node["models_id"] = models_ids_node;
		node["models"] = models;
	}

	void decode(YAML::Node &node)
	{

		// models_id = node["models_id"].as<map<int, shared_ptr<_modelMeta>>>();
		for (YAML::const_iterator i = node["models_id"].begin(); i != node["models_id"].end(); ++i)
		{
			models_id[i->first.as<int>()] = make_shared<_modelMeta>(i->second.as<_modelMeta>());
		}
		models = node["models"].as<map<size_t, int>>();

		waitForRenderJob([&]()
		{
			for (auto &m : models_id)
			{
				m.second->model->loadModel();
			}
		});
	}
	void _new()
	{
		// std::hash<string> x;
		// size_t key = x("fileName");
		// auto mm = modelManager::models.find(key);
		// if (mm != modelManager::models.end())
		// {
		// 	m = mm->second->id;
		// }
		// else
		// {
		auto _mm = make_shared<_modelMeta>();
		// modelManager::models[key] = _mm;
		modelManager::models_id[_mm->genID()] = _mm;
		modelManager::models_id[_mm->id]->name = "model_" + to_string(_mm->id);
		// 	m = _mm->id;
		// }
	}

	void init()
	{
		{
			auto _mm = make_shared<_modelMeta>("res/models/cube/cube.obj");
			_mm->id = 0;
			modelManager::models_id[_mm->id] = _mm;
			_mm->name = "cube";
		}
		{
			auto _mm = make_shared<_modelMeta>("res/models/sphere/sphere.obj");
			_mm->id = 1;
			modelManager::models_id[_mm->id] = _mm;
			_mm->name = "sphere";
		}
	}
}; // namespace modelManager

_model::_model(){
	// this->makeUnique();
};

set<int> toDestroy_models;
_model::~_model()
{
}

void _model::destroy()
{
	if (this->meta() && this->meta()->unique)
	{
		toDestroy_models.emplace(this->m);
	}
}
_model::_model(string fileName)
{
	using namespace modelManager;
	std::hash<string> x;
	size_t key = x(fileName);
	auto mm = modelManager::models.find(key);
	if (mm != modelManager::models.end())
	{
		m = models_id[mm->second]->id;
	}
	else
	{
		auto _mm = make_shared<_modelMeta>(fileName);
		modelManager::models_id[_mm->genID()] = _mm;
		modelManager::models[key] = _mm->id;
		m = _mm->id;
	}
}

vector<Mesh *> &_model::meshes()
{
	return modelManager::models_id[m]->model->meshes;
}
Mesh &_model::mesh()
{
	return *modelManager::models_id[m]->model->meshes[0];
}
void _model::makeUnique()
{
	int id = uniqueMeshIdGenerator.fetch_add(1);
	m = -id;
	string idStr = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id, 0};
	modelManager::models_id[m] = make_shared<_modelMeta>();
	modelManager::models_id[m]->name = idStr;
	modelManager::models_id[m]->unique = true;
	std::hash<string> x;
	size_t key = x(idStr);
}
void _model::makeProcedural()
{
	int id = uniqueMeshIdGenerator.fetch_add(1);
	m = -id;
	string idStr = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id, 0};
	modelManager::models_id[m] = make_shared<_modelMeta>();
	modelManager::models_id[m]->name = idStr;
	std::hash<string> x;
	size_t key = x(idStr);
}
_modelMeta *_model::meta() const
{
	return modelManager::models_id[m].get();
}
void _modelMeta::getBounds()
{
	radius = 0;
	if (this->model->ready())
	{
		bounds = glm::vec3(0);
		for (auto &i : this->model->meshes)
		{
			for (auto &j : i->vertices)
			{
				bounds = glm::vec3(glm::max(abs(bounds.x), abs(j.x)),
								   glm::max(abs(bounds.y), abs(j.y)),
								   glm::max(abs(bounds.z), abs(j.z)));
				float r = length(j);
				if (r > radius)
					radius = r;
			}
		}
		// radius = length(bounds);
	}
	else
	{
		enqueRenderJob([&]()
					   { getBounds(); });
	}
}
void _modelMeta::inspect()
{
	renderEdit("path", this->model->modelPath);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("FILE_DRAG_AND_DROP.obj"))
		{
			// IM_ASSERT(payload->DataSize == sizeof(string*));
			string payload_n = string((const char *)payload->Data);
			cout << "file payload:" << payload_n << endl;
			this->model->modelPath = payload_n;
			this->file = payload_n;
			this->model->meshes.clear();
			this->model->loadModel();
			this->getBounds();
		}
		ImGui::EndDragDropTarget();
	}
	// if (ImGui::Button("reload"))
	// {
	// 	// do something
	// }
}

void _model::recalcBounds()
{
	if (m != 0)
	{
		modelManager::models_id[m]->getBounds();
	}
}
