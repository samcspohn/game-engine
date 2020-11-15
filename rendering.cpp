#include "rendering.h"

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
_modelMeta::_modelMeta(string file)
{
	name = file;
	model = new Model(file);
	getBounds();
}
_modelMeta::~_modelMeta()
{
	delete model;
}

namespace modelManager
{
	map<size_t, _modelMeta *> models;
	void destroy()
	{
		while (models.size() > 0)
		{
			delete models.begin()->second;
			models.erase(models.begin());
		}
	}

	void save(OARCHIVE& oa){
		oa << models;
	}
	void load(IARCHIVE& ia){
		ia >> models;
		waitForRenderJob([&](){
			for(auto& m : models){
				if(m.second->model->modelPath != ""){
					m.second->model->loadModel();
				}
			}
		});
	}
}; // namespace modelManager

_model::_model(){
	// this->makeUnique();
};
_model::_model(string fileName)
{
	std::hash<string> x;
	size_t key = x(fileName);
	auto mm = modelManager::models.find(key);
	if (mm != modelManager::models.end())
	{
		m = key;
	}
	else
	{
		modelManager::models[key] = new _modelMeta(fileName);
		m = key;
	}
}

vector<Mesh> &_model::meshes()
{
	return modelManager::models[m]->model->meshes;
}
Mesh &_model::mesh()
{
	return modelManager::models[m]->model->meshes[0];
}
void _model::makeUnique()
{
	int id = uniqueMeshIdGenerator.fetch_add(1);
	m = id;
	string idStr = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id, 0};
	modelManager::models[m] = new _modelMeta();
	// m = modelManager::models.at(m);
	modelManager::models[m]->name = idStr;
	modelManager::models[m]->unique = true;
}
void _model::makeProcedural()
{
	int id = uniqueMeshIdGenerator.fetch_add(1);
	m = id;
	string idStr = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id, 0};
	modelManager::models[m] = new _modelMeta();
	modelManager::models[m]->name = idStr;
}
_modelMeta *_model::meta() const
{
	return modelManager::models[m];
}

//shader data

_shaderMeta::_shaderMeta() {}
_shaderMeta::_shaderMeta(string compute)
{
	name = compute;
	shader = new Shader(compute);
}
_shaderMeta::_shaderMeta(string vertex, string fragment)
{
	name = vertex + fragment;
	shader = new Shader(vertex, fragment);
}
_shaderMeta::_shaderMeta(string vertex, string geom, string fragment)
{
	name = vertex + geom + fragment;
	shader = new Shader(vertex, geom, fragment);
}
_shaderMeta::_shaderMeta(string vertex, string tess, string geom, string fragment)
{
	name = vertex + tess + geom + fragment;
	shader = new Shader(vertex, tess, geom, fragment);
}
_shaderMeta::~_shaderMeta()
{
	delete shader;
}

namespace shaderManager
{
	map<size_t, _shaderMeta *> shaders;
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
_shader::_shader(string compute)
{
	std::hash<string> x;
	size_t key = x(compute);
	auto ms = shaderManager::shaders.find(key);
	if (ms == shaderManager::shaders.end())
	{
		shaderManager::shaders[key] = new _shaderMeta(compute);
	}
	s = key;
}
_shader::_shader(string vertex, string fragment)
{
	std::hash<string> x;
	size_t key = x(vertex + fragment);
	auto ms = shaderManager::shaders.find(key);
	if (ms == shaderManager::shaders.end())
	{
		shaderManager::shaders[key] = new _shaderMeta(vertex, fragment);
	}
	s = key;
}
_shader::_shader(string vertex, string geom, string fragment)
{
	std::hash<string> x;
	size_t key = x(vertex + geom + fragment);
	auto ms = shaderManager::shaders.find(key);
	if (ms == shaderManager::shaders.end())
	{
		shaderManager::shaders[key] = new _shaderMeta(vertex, geom, fragment);
	}
	s = key;
}

_shader::_shader(string vertex, string tess, string geom, string fragment)
{
	std::hash<string> x;
	size_t key = x(vertex + tess + geom + fragment);
	auto ms = shaderManager::shaders.find(key);
	if (ms == shaderManager::shaders.end())
	{
		shaderManager::shaders[key] = new _shaderMeta(vertex, tess, geom, fragment);
	}
	s = key;
}

Shader* _shader::operator->()
{
	return shaderManager::shaders[s]->shader;
}
Shader &_shader::ref()
{
	return *(shaderManager::shaders[s]->shader);
}
_shaderMeta *_shader::meta() const
{
	return shaderManager::shaders[s];
}

int renderingId = 0;
void _modelMeta::getBounds()
{
	if (this->model->ready())
	{
		bounds = glm::vec3(0);
		for (auto &i : this->model->meshes)
		{
			for (auto &j : i.vertices)
			{
				bounds = glm::vec3(glm::max(abs(bounds.x), abs(j.x)),
								   glm::max(abs(bounds.y), abs(j.y)),
								   glm::max(abs(bounds.z), abs(j.z)));
			}
		}
		radius = length(bounds);
	}
	else
	{
		enqueRenderJob([&]() { getBounds(); });
	}
}
renderingMeta::renderingMeta(_shader _s, _model _m)
{
	s = _s;
	m = _m;
	if(m.meta() == 0){
		modelManager::models[m.m] = new _modelMeta();
		// m = modelManager::models.at(m);
		string idStr = {(char)(m.m >> 24), (char)(m.m >> 16), (char)(m.m >> 8), (char)m.m, 0};
		modelManager::models[m.m]->name = idStr;
		modelManager::models[m.m]->unique = true;
	}
	
}
renderingMeta::renderingMeta(const renderingMeta &other) {}

namespace renderingManager
{
	mutex m;
	map<size_t, map<size_t, renderingMeta *>> shader_model_vector;
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
		for (auto &i : renderingManager::shader_model_vector)
		{
			for (auto &j : i.second)
			{
				// int shader = j.second->s.s->shader->Program;
				for (auto &k : j.second->m.meshes())
				{
					batchManager::batches.back()[j.second->s][k.textures][j.second] = &k;
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
		modelManager::models[m]->getBounds();
	}
}
_model _renderer::getModel()
{
	return model;
}
_shader _renderer::getShader(){
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
REGISTER_COMPONENT(_renderer)

// COPY(_renderer);

// class camera : public component
// {

// };