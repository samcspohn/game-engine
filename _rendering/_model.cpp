#include "_model.h"
#include <imgui/imgui.h>
atomic<int> uniqueMeshIdGenerator{1};

namespace YAML
{
	Node convert<_model>::encode(const _model &rhs)
	{
		Node node;
		node["id"] = rhs.id;
		return node;
	}

	bool convert<_model>::decode(const Node &node, _model &rhs)
	{
		rhs.id = node["id"].as<int>();
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


	void modelManager::destroy()
	{
		meta.clear();
	}

	void modelManager::_new()
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
		meta[_mm->genID()] = _mm;
		meta[_mm->id]->name = "model_" + to_string(_mm->id);
		// 	m = _mm->id;
		// }
	}

	void modelManager::init()
	{
		{
			auto _mm = make_shared<_modelMeta>("res/models/cube/cube.obj");
			_mm->id = 0;
			meta[_mm->id] = _mm;
			path["res/models/cube/cube.obj"] = 0;
			_mm->name = "cube";
		}
		{
			auto _mm = make_shared<_modelMeta>("res/models/sphere/sphere.obj");
			_mm->id = 1;
			meta[_mm->id] = _mm;
			path["res/models/sphere/sphere.obj"] = 1;
			_mm->name = "sphere";
		}
	}

modelManager model_manager;

_model::_model(){
	id = 0;
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
		toDestroy_models.emplace(this->id);
	}
}
_model::_model(string fileName)
{
	string key = fileName;
	auto mm = model_manager.path.find(key);
	if (mm == model_manager.path.end())
	{
		auto _mm = make_shared<_modelMeta>(fileName);
		model_manager.meta[_mm->genID()] = _mm;
		model_manager.path[key] = _mm->id;
		mm = model_manager.path.find(key);
	}
	id = mm->second;
}

vector<Mesh *> &_model::meshes()
{
	return model_manager.meta[id]->model->meshes;
}
Mesh &_model::mesh()
{
	return *model_manager.meta[id]->model->meshes[0];
}
void _model::makeUnique()
{
	id = -uniqueMeshIdGenerator.fetch_add(1);
	string idStr = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id, 0};
	model_manager.meta[id] = make_shared<_modelMeta>();
	// model_manager.meta[id]->name = idStr;
	model_manager.meta[id]->unique = true;
	std::hash<string> x;
	size_t key = x(idStr);
}
void _model::makeProcedural()
{
	id = -uniqueMeshIdGenerator.fetch_add(1);
	string idStr = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id, 0};
	model_manager.meta[id] = make_shared<_modelMeta>();
	// model_manager.meta[id]->name = idStr;
}
_modelMeta *_model::meta() const
{
	return model_manager.meta[id].get();
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
	model_manager.meta[id]->getBounds();
}
