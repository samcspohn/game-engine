#include "_shader.h"

//shader data

_shaderMeta::_shaderMeta() {}
_shaderMeta::_shaderMeta(string compute)
{
	// name = compute;
	shader = make_unique<Shader>(compute);
}
_shaderMeta::_shaderMeta(string vertex, string fragment)
{
	// name = vertex + fragment;
	shader = make_unique<Shader>(vertex, fragment);
}
_shaderMeta::_shaderMeta(string vertex, string geom, string fragment)
{
	// name = vertex + geom + fragment;
	shader = make_unique<Shader>(vertex, geom, fragment);
}
_shaderMeta::_shaderMeta(string vertex, string tess, string geom, string fragment)
{
	// name = vertex + tess + geom + fragment;
	shader = make_unique<Shader>(vertex, tess, geom, fragment);
}
_shaderMeta::~_shaderMeta()
{
	// delete shader.get();
}
REGISTER_ASSET(_shaderMeta);

namespace shaderManager
{
	map<size_t, shared_ptr<_shaderMeta>> shaders;
	map<int, shared_ptr<_shaderMeta>> shaders_ids;
	void _new()
	{
		// auto ms = shaderManager::shaders.find(key);
		// if (ms == shaderManager::shaders.end())
		// {
		// auto sm = meta;
		// shaderManager::shaders[key] = sm;
		auto sm = make_shared<_shaderMeta>();
		sm->shader = make_unique<Shader>();
		shaderManager::shaders_ids[sm->genID()] = sm;
		sm->name = "shader_" + to_string(sm->id);
		// shaderManager::shaders.find(key);
		// }
	}
	void destroy()
	{
		shaders.clear();
		shaders_ids.clear();
	}
	void save(OARCHIVE &oa)
	{
		oa << shaders << shaders_ids;
	}
	void load(IARCHIVE &ia)
	{
		shaders.clear();
		shaders_ids.clear();
		ia >> shaders >> shaders_ids;
		// waitForRenderJob([&]() {
		for (auto &i : shaders_ids)
		{
			i.second->shader->_Shader();
		}
		// });
	}

	void init()
	{
		// default shader
		std::hash<string> x;
		string vertex, fragment;
		vertex = "res/shaders/model.vert";
		fragment = "res/shaders/model.frag";
		size_t key = 0;
		auto ms = shaderManager::shaders.find(key);
		if (ms == shaderManager::shaders.end())
		{
			auto sm = make_shared<_shaderMeta>(vertex, fragment);
			shaderManager::shaders[key] = sm;
			sm->id = 0;
			shaderManager::shaders_ids[0] = sm;
			sm->name = "default shader";
		}
	}
}; // namespace shaderManager

_shader::_shader() {}
#define FIND_SHADER_META(meta)                        \
	auto ms = shaderManager::shaders.find(key);       \
	if (ms == shaderManager::shaders.end())           \
	{                                                 \
		auto sm = meta;                               \
		shaderManager::shaders[key] = sm;             \
		shaderManager::shaders_ids[sm->genID()] = sm; \
		ms = shaderManager::shaders.find(key);        \
	}                                                 \
	s = ms->second->id;

_shader::_shader(string compute)
{
	std::hash<string> x;
	size_t key = x(compute);
	FIND_SHADER_META(make_shared<_shaderMeta>(compute))
}
_shader::_shader(string vertex, string fragment)
{
	std::hash<string> x;
	size_t key = x(vertex + fragment);
	FIND_SHADER_META(make_shared<_shaderMeta>(vertex, fragment))
}
_shader::_shader(string vertex, string geom, string fragment)
{
	std::hash<string> x;
	size_t key = x(vertex + geom + fragment);
	FIND_SHADER_META(make_shared<_shaderMeta>(vertex, geom, fragment))
}

_shader::_shader(string vertex, string tess, string geom, string fragment)
{
	std::hash<string> x;
	size_t key = x(vertex + tess + geom + fragment);
	FIND_SHADER_META(make_shared<_shaderMeta>(vertex, tess, geom, fragment))
}

Shader *_shader::operator->()
{
	return shaderManager::shaders_ids[s]->shader.get();
}
Shader &_shader::ref()
{
	return *(shaderManager::shaders_ids[s]->shader);
}
_shaderMeta *_shader::meta() const
{
	return shaderManager::shaders_ids[s].get();
}
bool _shaderMeta::onEdit()
{
	// if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	// {
	// 	// Set payload to carry the index of our item (could be anything)
	// 	ImGui::SetDragDropPayload("SHADER_DRAG_AND_DROP", &id, sizeof(int));
	// 	ImGui::EndDragDropSource();
	// }
	// return ret;
}
void _shaderMeta::inspect()
{
	static const map<GLenum, string> types{{GL_FRAGMENT_SHADER, ".frag"}, {GL_VERTEX_SHADER, ".vert"}, {GL_GEOMETRY_SHADER, ".geom"}, {GL_TESS_EVALUATION_SHADER, ".tese"}, {GL_TESS_CONTROL_SHADER, ".tesc"}, {GL_COMPUTE_SHADER, ".comp"}};
	static const map<GLenum, string> types2{{GL_FRAGMENT_SHADER, "fragment"}, {GL_VERTEX_SHADER, "vertex"}, {GL_GEOMETRY_SHADER, "geometry"}, {GL_TESS_EVALUATION_SHADER, "tesselation evaluation"}, {GL_TESS_CONTROL_SHADER, "tesselation control"}, {GL_COMPUTE_SHADER, "compute"}};

	if (ImGui::Button("+"))
	{
		ImGui::OpenPopup("add_shader_file");
	}
	if (ImGui::BeginPopup("add_shader_file"))
	{
		ImGui::Separator();
		for (auto &x : types2)
		{
			// this->shader->_shaders.find(x.first) == types2.end() &&
			if (ImGui::Selectable(x.second.c_str()))
			{
				this->shader->_shaders.emplace(x.first, "");
			}
		}
		ImGui::EndPopup();
	}
	for (auto &i : this->shader->_shaders)
	{
		// renderEdit("type",(int&)i.first);

		renderEdit(types2.at(i.first).c_str(), i.second);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(("FILE_DRAG_AND_DROP" + types.at(i.first)).c_str()))
			{
				// IM_ASSERT(payload->DataSize == sizeof(string*));
				string payload_n = string((const char *)payload->Data);
				cout << "file payload:" << payload_n << endl;
				i.second = payload_n;
			}
			ImGui::EndDragDropTarget();
		}
	}
	if (ImGui::Button("reload"))
	{
		this->shader->_Shader();
		// do something
	}
}
string _shaderMeta::type()
{
	return "SHADER_DRAG_AND_DROP";
}
bool operator<(const _shader &l, const _shader &r)
{
	return l.s < r.s;
}
