#include "rendering.h"

using namespace std;

gpu_vector_proxy<matrix>* GPU_MATRIXES;

class _renderer;

// model data

atomic<int> uniqueMeshIdGenerator;

_modelMeta::_modelMeta() {
	model = new Model();
	getBounds();
}
_modelMeta::_modelMeta(string file) {
	name = file;
	model = new Model(file);
	getBounds();
}
_modelMeta::~_modelMeta() {
	delete model;
}

namespace modelManager{

	map<string, _modelMeta*> models;
	void destroy(){
		while (models.size() > 0)
		{
			delete models.begin()->second;
			models.erase(models.begin());
		}
	}
};


_model::_model() {
	// this->makeUnique();
};
_model::_model(string fileName) {
	auto mm = modelManager::models.find(fileName);
	if (mm != modelManager::models.end()) {
		m = mm->second;
	}
	else {
		modelManager::models[fileName] = new _modelMeta(fileName);
		m = modelManager::models.at(fileName);
	}
}

vector<Mesh>& _model::meshes(){
	return m->model->meshes;
}
Mesh& _model::mesh(){
	return m->model->meshes[0];
}
void _model::makeUnique(){
	int id = uniqueMeshIdGenerator.fetch_add(1);
	string idStr = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id, 0 };
	modelManager::models[idStr] = new _modelMeta();
	m = modelManager::models.at(idStr);
	m->unique = true;
}


//shader data

_shaderMeta::_shaderMeta() {}
_shaderMeta::_shaderMeta(string compute) {
	name = compute;
	shader = new Shader(compute);
}
_shaderMeta::_shaderMeta(string vertex, string fragment) {
	name = vertex + fragment;
	shader = new Shader(vertex, fragment);
}
_shaderMeta::_shaderMeta( string vertex, string geom,string fragment) {
	name = vertex + geom + fragment;
	shader = new Shader(vertex, geom, fragment);
}
_shaderMeta::~_shaderMeta() {
	delete shader;
}


namespace shaderManager {
	map<string, _shaderMeta*> shaders = map<string, _shaderMeta*>();
	void destroy(){
		while (shaders.size() > 0)
		{
			delete shaders.begin()->second;
			shaders.erase(shaders.begin());
		}
	}
};


_shader::_shader() {}
_shader::_shader(string compute) {
	auto ms = shaderManager::shaders.find(compute);
	if (ms != shaderManager::shaders.end()) {
		s = ms->second;
	}
	else {
		shaderManager::shaders[compute] = new _shaderMeta(compute);
		s = shaderManager::shaders[compute];
	}
}
_shader::_shader(string vertex, string fragment) {
	auto ms = shaderManager::shaders.find(vertex + fragment);
	if (ms != shaderManager::shaders.end()) {
		s = ms->second;
	}
	else {
		shaderManager::shaders[vertex + fragment] = new _shaderMeta(vertex, fragment);
		s = shaderManager::shaders[vertex + fragment];
	}
}
_shader::_shader(string vertex, string geom,  string fragment) {
	auto ms = shaderManager::shaders.find(vertex + geom + fragment);
	if (ms != shaderManager::shaders.end()) {
		s = ms->second;
	}
	else {
		shaderManager::shaders[geom + vertex + fragment] = new _shaderMeta(vertex, geom, fragment);
		s = shaderManager::shaders[geom + vertex + fragment];
	}
}


int renderingId = 0;
void _modelMeta::getBounds(){
	if(this->model->ready()){
		bounds = glm::vec3(0);
		for(auto& i : this->model->meshes){
			for(auto &j : i.vertices){
				bounds = glm::vec3(glm::max(abs(bounds.x),abs(j.x)),
				glm::max(abs(bounds.y),abs(j.y)),
				glm::max(abs(bounds.z),abs(j.z)));
			}
		}
		radius = length(bounds);
	}else{
		enqueRenderJob([&]() { getBounds(); });
	}
}
renderingMeta::renderingMeta(_shader _s, _model _m) {
	s = _s;
	m = _m;
	_transformIds = new gpu_vector<GLuint>();
	_transformIds->ownStorage();
	// if(m.m->model->ready())
		// m.m->getBounds();
	// else{
	// 	enqueRenderJob([&]() { getBounds(); });
	// }
}
renderingMeta::renderingMeta(const renderingMeta& other) {}

namespace renderingManager {
	mutex m;
	map<string, map<string, renderingMeta*> > shader_model_vector;
	void destroy(){
		while (shader_model_vector.size() > 0){
			while(shader_model_vector.begin()->second.size() > 0)
			{
				delete shader_model_vector.begin()->second.begin()->second;
				shader_model_vector.begin()->second.erase(shader_model_vector.begin()->second.begin());
			}
			shader_model_vector.erase(shader_model_vector.begin());
		}
	}
	void lock(){
		m.lock();
	}
	void unlock(){
		m.unlock();
	}
};

void destroyRendering(){
	shaderManager::destroy();
	modelManager::destroy();
	renderingManager::destroy();
}

void _model::recalcBounds(){
	if(m != 0){
		m->getBounds();
	}	
}
_model _renderer::getModel(){
	return model;
}

void _renderer::onStart() {
	if (shader.s == 0 || model.m == 0)
		return;
	if(meta != 0)
		set(meta);
	else
		set(shader,model);
	transform->gameObject->renderer = this;
}
_renderer::_renderer() {}

void _renderer::set(_shader s, _model m) {
	shader = s;
	model = m;
	if(!transformIdRef.isNull()){
		meta->ids.erase(transformIdRef);
	}
	renderingManager::lock();
	auto r = renderingManager::shader_model_vector.find(s.s->name);
	if (r == renderingManager::shader_model_vector.end()) {
		renderingManager::shader_model_vector[s.s->name][m.m->name] = new renderingMeta(s, m);

		r = renderingManager::shader_model_vector.find(s.s->name);
	}
	else {
		auto rm = r->second.find(m.m->name);
		if (rm == r->second.end())
			r->second[m.m->name] = new renderingMeta(s, m);
	}
	renderingManager::unlock();

	meta = (r->second[m.m->name]);
	transformIdRef = meta->ids.push_back(transform->_T);
}

void _renderer::set_proto(_shader s, _model m) {
	shader = s;
	model = m;
	renderingManager::lock();
	auto r = renderingManager::shader_model_vector.find(s.s->name);
	if (r == renderingManager::shader_model_vector.end()) {
		renderingManager::shader_model_vector[s.s->name][m.m->name] = new renderingMeta(s, m);

		r = renderingManager::shader_model_vector.find(s.s->name);
	}
	else {
		auto rm = r->second.find(m.m->name);
		if (rm == r->second.end())
			r->second[m.m->name] = new renderingMeta(s, m);
	}
	renderingManager::unlock();

	meta = (r->second[m.m->name]);
}

void _renderer::set(renderingMeta* _meta){
	shader = _meta->s;
	model = _meta->m;
	if(model.m->unique){
		model.makeUnique();
		this->set(shader,model);
	}
	else{
		meta = _meta;
		if(!transformIdRef.isNull()){
			meta->ids.erase(transformIdRef);
		}
		transformIdRef = meta->ids.push_back(transform->_T);
	}
}
_renderer::_renderer(const _renderer& other) {
	shader = other.shader;
	model = other.model;
	meta = other.meta;
	transformIdRef = fast_list_deque<GLuint>::iterator();
}
void _renderer::onDestroy(){
	if(meta != 0){
		meta->ids.erase(transformIdRef);
	}
}

// COPY(_renderer);


// class camera : public component
// {
	
// };