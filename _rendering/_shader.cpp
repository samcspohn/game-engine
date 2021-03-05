#include "_shader.h"


//shader data

_shaderMeta::_shaderMeta() {}
_shaderMeta::_shaderMeta(string compute)
{
	// name = compute;
	shader = new Shader(compute);
}
_shaderMeta::_shaderMeta(string vertex, string fragment)
{
	// name = vertex + fragment;
	shader = new Shader(vertex, fragment);
}
_shaderMeta::_shaderMeta(string vertex, string geom, string fragment)
{
	// name = vertex + geom + fragment;
	shader = new Shader(vertex, geom, fragment);
}
_shaderMeta::_shaderMeta(string vertex, string tess, string geom, string fragment)
{
	// name = vertex + tess + geom + fragment;
	shader = new Shader(vertex, tess, geom, fragment);
}
_shaderMeta::~_shaderMeta()
{
	delete shader;
}
REGISTER_ASSET(_shaderMeta);

namespace shaderManager
{
	map<size_t, _shaderMeta *> shaders;
	map<int, _shaderMeta *> shaders_ids;
	void destroy()
	{
		while (shaders.size() > 0)
		{
			delete shaders.begin()->second;
			shaders.erase(shaders.begin());
		}
	}
	void save(OARCHIVE &oa)
	{
		oa << shaders << shaders_ids;
	}
	void load(IARCHIVE &ia)
	{
		ia >> shaders >> shaders_ids;
		waitForRenderJob([&]() {
			for (auto &i : shaders)
			{
				i.second->shader->_Shader();
			}
		});
	}
}; // namespace shaderManager

_shader::_shader() {}
#define FIND_SHADER_META(meta)                        \
	auto ms = shaderManager::shaders.find(key);       \
	if (ms == shaderManager::shaders.end())           \
	{                                                 \
		auto sm = new meta;                           \
		shaderManager::shaders[key] = sm;             \
		shaderManager::shaders_ids[sm->genID()] = sm; \
		ms = shaderManager::shaders.find(key);        \
	}                                                 \
	s = ms->second->id;

_shader::_shader(string compute)
{
	std::hash<string> x;
	size_t key = x(compute);
	FIND_SHADER_META(_shaderMeta(compute))
}
_shader::_shader(string vertex, string fragment)
{
	std::hash<string> x;
	size_t key = x(vertex + fragment);
	FIND_SHADER_META(_shaderMeta(vertex, fragment))
}
_shader::_shader(string vertex, string geom, string fragment)
{
	std::hash<string> x;
	size_t key = x(vertex + geom + fragment);
	FIND_SHADER_META(_shaderMeta(vertex, geom, fragment))
}

_shader::_shader(string vertex, string tess, string geom, string fragment)
{
	std::hash<string> x;
	size_t key = x(vertex + tess + geom + fragment);
	FIND_SHADER_META(_shaderMeta(vertex, tess, geom, fragment))
}

Shader *_shader::operator->()
{
	return shaderManager::shaders_ids[s]->shader;
}
Shader &_shader::ref()
{
	return *(shaderManager::shaders_ids[s]->shader);
}
_shaderMeta *_shader::meta() const
{
	return shaderManager::shaders_ids[s];
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
	// static const map<GLenum, string> types{{GL_FRAGMENT_SHADER, ".frag"}, {GL_VERTEX_SHADER, ".vert"}, {GL_GEOMETRY_SHADER, ".geom"}, {GL_TESS_EVALUATION_SHADER, ".tese"}, {GL_TESS_CONTROL_SHADER, ".tesc"}};
	// static const map<GLenum, string> types2{{GL_FRAGMENT_SHADER, "fragment"}, {GL_VERTEX_SHADER, "vertex"}, {GL_GEOMETRY_SHADER, "geometry"}, {GL_TESS_EVALUATION_SHADER, "tesselation evaluation"}, {GL_TESS_CONTROL_SHADER, "tesselation control"}};
	// for (auto &i : this->shader->_shaders)
	// {
	// 	// renderEdit("type",(int&)i.first);

	// 	renderEdit(types2.at(i.first).c_str(), i.second);
	// 	if (ImGui::BeginDragDropTarget())
	// 	{
	// 		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(("FILE_DRAG_AND_DROP" + types.at(i.first)).c_str()))
	// 		{
	// 			// IM_ASSERT(payload->DataSize == sizeof(string*));
	// 			string payload_n = string((const char *)payload->Data);
	// 			cout << "file payload:" << payload_n << endl;
	// 			i.second = payload_n;
	// 		}
	// 		ImGui::EndDragDropTarget();
	// 	}
	// }
	// if (ImGui::Button("reload"))
	// {
	// 	this->shader->_Shader();
	// 	// do something
	// }
}
string _shaderMeta::type()
{
	return "SHADER_DRAG_AND_DROP";
}
bool operator<(const _shader &l, const _shader &r)
{
	return l.s < r.s;
}