#include "rendering.h"
#include "editor.h"
#include "collision.h"
#define GLM_GTX_intersect
#include <glm/gtx/intersect.hpp>
// #include "imgui.h"
using namespace std;

gpu_vector_proxy<matrix> *GPU_MATRIXES;
gpu_vector<__renderer> *__RENDERERS_in;
// gpu_vector<GLuint>* __RENDERERS_keys_in;
// gpu_vector_proxy<__renderer>* __RENDERERS_out;
// gpu_vector_proxy<GLuint>* __RENDERERS_keys_out;

gpu_vector<GLuint> *__renderer_offsets;
gpu_vector<__renderMeta> *__rendererMetas;

class _renderer;

// model data

atomic<int> uniqueMeshIdGenerator{1};

_modelMeta::_modelMeta()
{
	model = new Model();
	getBounds();
}
_modelMeta::_modelMeta(string _file)
{
	file = _file;
	model = new Model(file);
	getBounds();
}
_modelMeta::~_modelMeta()
{
	delete model;
}
bool _modelMeta::onEdit()
{
	// char input[1024];
	// sprintf(input, name.c_str());
	// if (ImGui::InputText("", input, 1024, ImGuiInputTextFlags_None))
	// 	name = {input};
	// ImGui::PopID();
	// ImGui::PopItemWidth();
	// ImGui::Button(name.c_str(), {40, 40});
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		// Set payload to carry the index of our item (could be anything)
		ImGui::SetDragDropPayload("MODEL_DRAG_AND_DROP", &id, sizeof(int));
		ImGui::EndDragDropSource();
	}
	return false;
}
string _modelMeta::type()
{
	return "MODEL_DRAG_AND_DROP";
}

namespace modelManager
{
	map<size_t, _modelMeta *> models;
	map<int, _modelMeta *> models_id;
	// map<int, _modelMeta *> unique_models;
	void destroy()
	{
		while (models.size() > 0)
		{
			delete models.begin()->second;
			models.erase(models.begin());
		}
	}
}; // namespace modelManager

_model::_model() {
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
	std::hash<string> x;
	size_t key = x(fileName);
	auto mm = modelManager::models.find(key);
	if (mm != modelManager::models.end())
	{
		m = mm->second->id;
	}
	else
	{
		_modelMeta *_mm = new _modelMeta(fileName);
		modelManager::models[key] = _mm;
		modelManager::models_id[_mm->genID()] = _mm;
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
	modelManager::models_id[m] = new _modelMeta();
	modelManager::models_id[m]->name = idStr;
	modelManager::models_id[m]->unique = true;
}
void _model::makeProcedural()
{
	int id = uniqueMeshIdGenerator.fetch_add(1);
	m = -id;
	string idStr = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id, 0};
	modelManager::models_id[m] = new _modelMeta();
	modelManager::models_id[m]->name = idStr;
}
_modelMeta *_model::meta() const
{
	try
	{
		return modelManager::models_id[m];
	}
	catch (const std::out_of_range &oor)
	{
		return nullptr;
	}
}

// shader data

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
	// bool ret = false;
	// char input[1024];
	// sprintf(input, name.c_str());
	// if (ImGui::InputText("", input, 1024, ImGuiInputTextFlags_None))
	// 	name = {input};
	// ImGui::PopID();
	// ImGui::PopItemWidth();
	// ret = ImGui::Button(name.c_str(), {40, 40});
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		// Set payload to carry the index of our item (could be anything)
		ImGui::SetDragDropPayload("SHADER_DRAG_AND_DROP", &id, sizeof(int));
		ImGui::EndDragDropSource();
	}
	// return ret;
}
void _shaderMeta::inspect()
{
	static const map<GLenum, string> types{{GL_FRAGMENT_SHADER, ".frag"}, {GL_VERTEX_SHADER, ".vert"}, {GL_GEOMETRY_SHADER, ".geom"}, {GL_TESS_EVALUATION_SHADER, ".tese"}, {GL_TESS_CONTROL_SHADER, ".tesc"}};
	static const map<GLenum, string> types2{{GL_FRAGMENT_SHADER, "fragment"}, {GL_VERTEX_SHADER, "vertex"}, {GL_GEOMETRY_SHADER, "geometry"}, {GL_TESS_EVALUATION_SHADER, "tesselation evaluation"}, {GL_TESS_CONTROL_SHADER, "tesselation control"}};
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
int renderingId = 0;
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
			file = payload_n;
		}
		ImGui::EndDragDropTarget();
	}
	if (ImGui::Button("reload"))
	{
		this->model->meshes.clear();
		this->model->loadModel();
		// do something
	}
}
renderingMeta::renderingMeta(_shader _s, _model _m)
{
	s = _s;
	m = _m;
	if (m.meta() == 0)
	{
		modelManager::models_id[m.m] = new _modelMeta();
		string idStr = {(char)(m.m >> 24), (char)(m.m >> 16), (char)(m.m >> 8), (char)m.m, 0};
		modelManager::models_id[m.m]->name = idStr;
		modelManager::models_id[m.m]->unique = true;
	}
}
renderingMeta::renderingMeta(const renderingMeta &other) {}

namespace renderingManager
{
	mutex m;
	map<int, map<int, renderingMeta *>> shader_model_vector;
	void destroy()
	{
		while (shader_model_vector.size() > 0)
		{
			while (shader_model_vector.begin()->second.size() > 0)
			{
				delete shader_model_vector.begin()->second.begin()->second;
				shader_model_vector.begin()->second.erase(shader_model_vector.begin()->second.begin());
			}
			shader_model_vector.erase(shader_model_vector.begin());
		}
	}
	void lock()
	{
		m.lock();
	}
	void unlock()
	{
		m.unlock();
	}
}; // namespace renderingManager

bool operator<(const texArray &l, const texArray &r)
{
	return l.hash < r.hash;
}
bool operator<(const _shader &l, const _shader &r)
{
	return l.s < r.s;
}

double batchElement::getHash()
{
	return double((unsigned long long)r * (unsigned long long)m) / (double)(m->indices.size() * m->points.size());
}

void batch::init()
{
	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &this->VBO);
	glGenBuffers(1, &this->EBO);
}

void batch::addElement(batchElement b)
{
	elements.push_back(b);
}
void batch::finalize()
{
	double h = 10;
	this->vertexSize = 0;
	this->indexSize = 0;
	for (auto &i : elements)
	{
		h /= (i.getHash());
		this->vertexSize += i.m->points.size();
		this->indexSize += i.m->indices.size();
	}
	if (h != hash)
	{
		if (VAO == 0)
		{
			this->init();
		}
		glBindVertexArray(this->VAO);
		glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
		int offset = 0;
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * vertexSize, 0, GL_STATIC_DRAW);
		for (auto &i : elements)
		{
			glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vertex) * i.m->points.size(), i.m->points.data());
			offset += i.m->points.size();
		}
		offset = 0;
		// postion attribute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid *)offset);
		offset += sizeof(glm::vec3);
		// uvs attribute
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid *)offset);
		offset += sizeof(glm::vec2);
		// uvs2 attribute
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid *)offset);
		offset += sizeof(glm::vec2);
		// normals
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid *)offset);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize * sizeof(GLuint), 0, GL_STATIC_DRAW);
		offset = 0;
		for (auto &i : elements)
		{
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, sizeof(GLuint) * i.m->indices.size(), i.m->indices.data());
			offset += i.m->indices.size();
		}
		glBindVertexArray(0);
		this->hash = h;
	}
}

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
		// for(auto &i : batchManager::batches2){
		// 	for(auto &j : i.second){
		// 		j.second.elements.clear();
		// 	}
		// }
		for (auto &m : toDestroy_models)
		{
			delete modelManager::models_id.at(m);
			modelManager::models_id.at(m) = 0;
		}
		toDestroy_models.clear();
		for (auto i : renderingManager::shader_model_vector)
		{
			for (auto j : i.second)
			{
				if (j.second->m.meta() == 0)
				{
					delete renderingManager::shader_model_vector[i.first][j.first];
					renderingManager::shader_model_vector[i.first].erase(j.first);
				}
			}
		}
		for (auto &i : renderingManager::shader_model_vector)
		{
			for (auto &j : i.second)
			{
				for (auto &k : j.second->m.meshes())
				{
					batchManager::batches.back()[j.second->s][k->textures][j.second] = k;
					// batchElement b;
					// b.m = &k;
					// b.r = j.second;
					// batches2[j.second->s][k.textures].addElement(b);
				}
			}
		}
		return batchManager::batches.back();
		// for(auto &i : batchManager::batches2){
		// 	// cout << "" << i.first.s->name << endl;
		// 	for(auto &j : i.second){
		// 		// cout << "-- textures" << endl;
		// 			j.second.finalize();
		// 			// cout << " -- -- vao:" << k.second.VAO << " indices:" << k.second.indexSize << " vertices:" << k.second.vertexSize << endl;
		// 	}
		// }
		// batchManager::batches = tempBatches;
		// for(auto &i : batchManager::batches){
		// 	cout << " - batched shader:" << i.first.s->name << " : textures:" << i.second.size() << endl;
		// 	for(auto &j : i.second){
		// 		cout << " - - meshes:" << j.second.size() << endl;
		// 	}
		// }
	}
}; // namespace batchManager
// list<renderingMeta*> updatedRenderMetas;

void destroyRendering()
{
	shaderManager::destroy();
	modelManager::destroy();
	renderingManager::destroy();
}

void _model::recalcBounds()
{
	if (m != 0)
	{
		modelManager::models_id[m]->getBounds();
	}
}
_model _renderer::getModel()
{
	return model;
}
_shader _renderer::getShader()
{
	return shader;
}

void _renderer::setCullSizes(float min, float max)
{
	meta->maxRadius = max;
	meta->minRadius = min;
}
void _renderer::onStart()
{
	if (shader.s == 0 || model.m == 0)
		return;
	if (meta != 0)
		set(meta);
	else
		set(shader, model);
	transform->gameObject()->renderer = this;
}
_renderer::_renderer() {}

void _renderer::set(_shader s, _model m)
{
	shader = s;
	model = m;
	if (!transformIdRef.isNull())
	{
		meta->ids.erase(transformIdRef);
	}
	if (s.s == 0 || m.m == 0)
		return;
	renderingManager::lock();
	auto r = renderingManager::shader_model_vector.find(s.s);
	if (r == renderingManager::shader_model_vector.end())
	{
		renderingManager::shader_model_vector[s.s][m.m] = new renderingMeta(s, m);

		r = renderingManager::shader_model_vector.find(s.s);
		// updatedRenderMetas.push_back(r->second[m.m->name]);
	}
	else
	{
		auto rm = r->second.find(m.m);
		if (rm == r->second.end())
		{
			r->second[m.m] = new renderingMeta(s, m);
			// updatedRenderMetas.push_back(r->second[m.m->name]);
		}
	}
	renderingManager::unlock();

	meta = (r->second[m.m]);
	transformIdRef = meta->ids.push_back(transform.id);
}

void _renderer::set_proto(_shader s, _model m)
{
	shader = s;
	model = m;
	renderingManager::lock();
	auto r = renderingManager::shader_model_vector.find(s.s);
	if (r == renderingManager::shader_model_vector.end())
	{
		renderingManager::shader_model_vector[s.s][m.m] = new renderingMeta(s, m);

		r = renderingManager::shader_model_vector.find(s.s);
	}
	else
	{
		auto rm = r->second.find(m.m);
		if (rm == r->second.end())
			r->second[m.m] = new renderingMeta(s, m);
	}
	renderingManager::unlock();

	meta = (r->second[m.m]);
}

void _renderer::set(renderingMeta *_meta)
{
	shader = _meta->s;
	model = _meta->m;
	// if (modelManager::models[model.m]->unique)
	// {
	// 	model.makeUnique();
	// 	this->set(shader, model);
	// }
	// else
	// {
	meta = _meta;
	if (!transformIdRef.isNull())
	{
		meta->ids.erase(transformIdRef);
	}
	transformIdRef = meta->ids.push_back(transform.id);
	// }
}
_renderer::_renderer(const _renderer &other)
{
	shader = other.shader;
	model = other.model;
	meta = other.meta;
	transformIdRef = fast_list_deque<GLuint>::iterator();
}
void _renderer::onDestroy()
{
	if (meta != 0)
	{
		meta->ids.erase(transformIdRef);
	}
}

void renderEdit(const char *name, _model &m)
{
	// ImGui::DragInt(name,&i);
	if (m.m == 0) // uninitialized
		ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
	else
		ImGui::InputText(name, (char *)m.meta()->name.c_str(), m.meta()->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("MODEL_DRAG_AND_DROP"))
		{
			IM_ASSERT(payload->DataSize == sizeof(int));
			int payload_n = *(const int *)payload->Data;
			m.m = payload_n;
		}
		ImGui::EndDragDropTarget();
	}
}

void renderEdit(const char *name, _shader &s)
{
	// ImGui::DragInt(name,&i);
	if (s.s == 0) // uninitialized
		ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
	else
		ImGui::InputText(name, (char *)s.meta()->name.c_str(), s.meta()->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SHADER_DRAG_AND_DROP"))
		{
			IM_ASSERT(payload->DataSize == sizeof(int));
			int payload_n = *(const int *)payload->Data;
			s.s = payload_n;
		}
		ImGui::EndDragDropTarget();
	}
}

void _renderer::onEdit()
{
	_shader curr_s = shader;
	_model curr_m = model;
	RENDER(model);
	RENDER(shader);

	if (model.m != curr_m.m || shader.s != curr_s.s)
	{
		this->set(shader, model);
	}
}
REGISTER_COMPONENT(_renderer)

bool intersectRayMesh(glm::vec3 &p, glm::vec3 &d, Mesh *m, glm::vec3 &q, float &dist, glm::mat4 &model)
{

	float closest = numeric_limits<float>().max();
	glm::vec3 r{closest};
	bool ret = false;
	for (int i = 0; i < m->indices.size(); i += 3)
	{
		glm::vec3 p1 = model * glm::vec4(m->vertices[m->indices[i]], 1);
		glm::vec3 p2 = model * glm::vec4(m->vertices[m->indices[i + 1]], 1);
		glm::vec3 p3 = model * glm::vec4(m->vertices[m->indices[i + 2]], 1);
		// if (IntersectSegmentTriangle(p, p + d * closest, p1, p2, p3, q.x,q.y,q.z, t))
		// glm::vec2 bp;
		// float t;
		// if(glm::intersectRayTriangle(p,d,p1,p2,p3, bp, t))
		// if (glm::intersectLineTriangle(p, d, p1, p2, p3, q))
		if (_intersectRayTriangle(p, d, p1, p2, p3, q))
		{
			dist = glm::length(p - q);
			// float dist = t;
			if (dist < closest)
			{
				r = q;
				closest = dist;
			}
			ret = true;
		}
	}
	dist = closest;
	q = r;
	return ret;
}

bool intersectRayModel(glm::vec3 &p, glm::vec3 &d, Model *M, glm::vec3 &q, float &dist, glm::mat4 &model, glm::mat4 &rot)
{
	// glm::mat4 invModel = glm::inverse(model);
	// p = invModel * glm::vec4(p, 1);
	// d = glm::inverse(rot) * glm::vec4(d, 1);
	bool ret = false;
	float closest = numeric_limits<float>().max();
	glm::vec3 r{closest};
	for (Mesh *m : M->meshes)
	{
		if (intersectRayMesh(p, d, m, q, dist, model))
		{

			dist = glm::length(p - q);
			if (dist < closest)
			{
				r = q;
				closest = dist;
			}
			ret = true;
		}
	}
	dist = closest;
	q = r;
	return ret;
}

#include "terrain.h"
transform2 renderRaycast(glm::vec3 p, glm::vec3 dir)
{
	dir = normalize(dir);
	auto renderers = COMPONENT_LIST(_renderer);
	cout << "p: " + to_string(p) + " dir: " + to_string(dir) + '\n';
	ray _ray(p, dir);

	float closest = numeric_limits<float>().max();
	transform2 ret;
	mutex lock;
	parallelfor(renderers->size(),
				// for (uint32_t i = 0; i < renderers->size(); ++i)
				{
					glm::vec3 q;
					_renderer *_rend;
					if (renderers->getv(i) && (_rend = renderers->get(i))->getModel().meta())
					{
						if (_rend->transform->gameObject()->getComponent<terrain>() != 0)
							continue;
						// _renderer *_rend = renderers->get(i);
						float rad = _rend->getModel().meta()->radius;
						float sc = glm::length(_rend->transform->getScale());
						rad *= sc;
						// if (glm::intere(p,dir,sphere(_rend->transform->getPosition(),rad),t,q))
						float t;
						if (glm::intersectRaySphere(p, dir, _rend->transform->getPosition(), rad * rad, t))
						{
							string name = renderers->get(i)->transform->name();
							if (name == "")
							{
								name = "game object " + to_string(_rend->transform.id);
							}
							glm::mat4 model = _rend->transform->getModel();
							glm::mat4 rot = glm::toMat4(_rend->transform->getRotation());
							if (intersectRayModel(p, dir, _rend->getModel().meta()->model, q, t, model, rot))
							{
								cout << name + ": hit -- " + to_string(t) + "\n";
								// float dist = t;
								float dist = glm::length(p - q);
								{
									scoped_lock l(lock);
									if (dist < closest)
									{
										closest = dist;
										ret = _rend->transform;
									}
								}
							}
							else
							{
								cout << name + ": miss\n";
							}
						}
					}
				})
		// }
		return ret;
}