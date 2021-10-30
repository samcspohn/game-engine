#include "_shader.h"

// shader data

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

void shaderManager::_new()
{
	auto sm = make_shared<_shaderMeta>();
	sm->shader = make_unique<Shader>();
	meta[sm->genID()] = sm;
	sm->name = "shader_" + to_string(sm->id);
	// shaderManager::shaders.find(key);
	// }
}
void shaderManager::destroy()
{
	path.clear();
	meta.clear();
}
void shaderManager::init()
{
	// default shader
	// std::hash<string> x;
	string vertex, fragment;
	vertex = "res/shaders/model.vert";
	fragment = "res/shaders/model.frag";
	string key = vertex + fragment;
	auto ms = path.find(key);
	if (ms == path.end())
	{
		auto sm = make_shared<_shaderMeta>(vertex, fragment);
		sm->id = 0;
		path[key] = sm->id;
		meta[sm->id] = sm;
		sm->name = "default shader";
	}

	vertex = "res/shaders/terrain.vert";
	key = vertex + fragment;
	ms = path.find(key);
	if (ms == path.end())
	{
		auto sm = make_shared<_shaderMeta>(vertex, fragment);
		sm->genID();
		path[key] = sm->id;
		meta[sm->id] = sm;
		sm->name = "no_inst shader";
	}
}

shaderManager shader_manager;

_shader::_shader() {}
#define FIND_SHADER_META(_meta)                 \
	auto ms = shader_manager.path.find(key);   \
	if (ms == shader_manager.path.end())       \
	{                                          \
		auto sm = _meta;                        \
		shader_manager.meta[sm->genID()] = sm; \
		shader_manager.path[key] = sm->id;     \
		ms = shader_manager.path.find(key);    \
	}                                          \
	s = ms->second;

_shader::_shader(string compute)
{
	string key = compute;
	FIND_SHADER_META(make_shared<_shaderMeta>(compute))
}
_shader::_shader(string vertex, string fragment)
{

	string key = vertex + fragment;
	FIND_SHADER_META(make_shared<_shaderMeta>(vertex, fragment))
}
_shader::_shader(string vertex, string geom, string fragment)
{
	string key = vertex + geom + fragment;
	FIND_SHADER_META(make_shared<_shaderMeta>(vertex, geom, fragment))
}

_shader::_shader(string vertex, string tess, string geom, string fragment)
{
	string key = vertex + tess + geom + fragment;
	FIND_SHADER_META(make_shared<_shaderMeta>(vertex, tess, geom, fragment))
}

Shader *_shader::operator->()
{
	return shader_manager.meta[s]->shader.get();
}
Shader &_shader::ref()
{
	return *(shader_manager.meta[s]->shader.get());
}
_shaderMeta *_shader::meta() const
{
	return shader_manager.meta[s].get();
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
	return false;
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
