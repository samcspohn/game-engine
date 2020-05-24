#pragma once
#include<map>
#include "Model.h"
#include "Shader.h"
#include <string>
#include <list>
#include "gpu_vector.h"
#include "concurrency.h"
#include "game_object.h"
#include "Component.h"
#include "fast_list.h"
#include <atomic>
using namespace std;


struct matrix {
	glm::mat4 mvp;
	glm::mat4 model;
	glm::mat4 normal;
};
gpu_vector_proxy<matrix>* GPU_MATRIXES;
//gpu_vector<GLuint> ids;

// array_heap<bool> GPU_MATRIXES_IDS = array_heap<bool>();



class _renderer;
struct __renderer {
	GLuint transform;
	GLuint matrixID;
	__renderer() {	}
	__renderer(GLuint tid, GLuint mid) {
		transform = tid;
		matrixID = mid;
	}
};

mutex rendererLock;
// fast_list<__renderer> gpu_renderers = fast_list<__renderer>();
// gpu_vector<__renderer>* GPU_RENDERERS;

// model data
struct _modelMeta {
	_modelMeta() {}
	_modelMeta(string file) {
		name = file;
		model = new Model(file);
	}
	~_modelMeta() {
		delete model;
	}
	string name;
	Model* model = 0;
};
class {
public:
	map<string, _modelMeta*> models;

}modelManager;

class _model {
public:
	_model() {};
	_model(string fileName) {
		auto mm = modelManager.models.find(fileName);
		if (mm != modelManager.models.end()) {
			m = mm->second;
		}
		else {
			modelManager.models[fileName] = new _modelMeta(fileName);
			m = modelManager.models.at(fileName);
		}
	}
	_modelMeta* m = 0;
};


//shader data
struct _shaderMeta {
	_shaderMeta() {}
	_shaderMeta(string compute) {
		name = compute;
		shader = new Shader(compute);
	}
	_shaderMeta(string vertex, string fragment) {
		name = vertex + fragment;
		shader = new Shader(vertex, fragment);
	}
	_shaderMeta( string vertex, string geom,string fragment) {
		name = vertex + geom + fragment;
		shader = new Shader(vertex, geom, fragment);
	}
	~_shaderMeta() {
		delete shader;
	}
	string name;
	Shader* shader = 0;
};

class {
public:
	map<string, _shaderMeta*> shaders = map<string, _shaderMeta*>();
}shaderManager;

class _shader {
public:
	_shader() {}
	_shader(string compute) {
		auto ms = shaderManager.shaders.find(compute);
		if (ms != shaderManager.shaders.end()) {
			s = ms->second;
		}
		else {
			shaderManager.shaders[compute] = new _shaderMeta(compute);
			s = shaderManager.shaders[compute];
		}
	}
	_shader(string vertex, string fragment) {
		auto ms = shaderManager.shaders.find(vertex + fragment);
		if (ms != shaderManager.shaders.end()) {
			s = ms->second;
		}
		else {
			shaderManager.shaders[vertex + fragment] = new _shaderMeta(vertex, fragment);
			s = shaderManager.shaders[vertex + fragment];
		}
	}
	_shader(string vertex, string geom,  string fragment) {
		auto ms = shaderManager.shaders.find(vertex + geom + fragment);
		if (ms != shaderManager.shaders.end()) {
			s = ms->second;
		}
		else {
			shaderManager.shaders[geom + vertex + fragment] = new _shaderMeta(vertex, geom, fragment);
			s = shaderManager.shaders[geom + vertex + fragment];
		}
	}
	_shaderMeta* s = 0;
};



int renderingId = 0;
struct renderingMeta {
	GLuint id = -1;
	gpu_vector<GLuint>* _transformIds;
	
	// fast_list<GLuint> ids = fast_list<GLuint>();
	// fast_list<GLuint> transformIds = fast_list<GLuint>();
	// vector<GLuint> _transformIds;
	// atomic<int> counter;
	_shader s;
	_model m;
	renderingMeta() {
		//_ids = new gpu_vector<GLuint>();
		//_ids->storage = &ids.data;
	}
	renderingMeta(_shader _s, _model _m) {
		s = _s;
		m = _m;
		// _ids = new gpu_vector<GLuint>();
		// _ids->ownStorage();
	}

	//renderingMeta operator=(const renderingMeta& other) {
	//	return renderingMeta(other);
	//}
	renderingMeta(const renderingMeta& other) {
		s = other.s;
		m = other.m;
		//ids = fast_list<GLuint>(other.ids);
		// _ids = new gpu_vector<GLuint>();
		// _ids->storage = &ids.data;
	}
	~renderingMeta() {

	}

};

class {
public:
	map<string, map<string, renderingMeta*> > shader_model_vector;
	map<GLuint, renderingMeta*> renderingMetas;
	mutex lock;

}renderingManager;

class cullObjects;
class _renderer : public component {
	_shader shader;
	_model model;
	// array_heap<bool>::ref matrixLoc;
	// fast_list<GLuint>::iterator idLoc;
	renderingMeta* meta = 0;
	// fast_list<__renderer>::iterator _Rloc;
	// fast_list<GLuint>::iterator transformIdRef;

	// friend _camera;
	friend cullObjects;
public:
	_model getModel(){
		return model;
	}

	void onStart() {
		if (shader.s == 0 || model.m == 0)
			return;
		if(meta != 0)
			set(meta);
		else
			set(shader,model);
		transform->gameObject->renderer = this;
	}
	_renderer() {}

	void set(_shader s, _model m) {
		rendererLock.lock();
		shader = s;
		model = m;
		// if (!transformIdRef.isNull()) {
		// 	meta->counter.fetch_sub(1);
		// }
		auto r = renderingManager.shader_model_vector.find(s.s->name);
		if (r == renderingManager.shader_model_vector.end()) {
			renderingManager.shader_model_vector[s.s->name][m.m->name] = new renderingMeta(s, m);

			r = renderingManager.shader_model_vector.find(s.s->name);
		}
		else {
			auto rm = r->second.find(m.m->name);
			if (rm == r->second.end())
				r->second[m.m->name] = new renderingMeta(s, m);
		}

		meta = (r->second[m.m->name]);
		// transformIdRef = meta->counter.fetch_add(1);
		rendererLock.unlock();

	}
	void set(renderingMeta* _meta){
		shader = _meta->s;
		model = _meta->m;
		meta = _meta;
		// GLuint t = transform->_T;
		// if (!transformIdRef.isNull()) {
		// 	renderLock.lock();
		// 	meta->counter.fetch_sub(1);
		// }
		// else
		// 	renderLock.lock();
		// meta->counter.fetch_add(1);
		// renderLock.unlock();
	}
	_renderer(const _renderer& other) {
//		if (other.meta != 0) {
			shader = other.shader;
			model = other.model;
			meta = other.meta;
			fast_list<GLuint>::iterator();
//		}
	}
	void onDestroy(){
        // if (meta != 0) {
		// 	rendererLock.lock();
		// 	meta->transformIds.erase(transformIdRef);
		// 	rendererLock.unlock();
		// }
	}
//	~_renderer() {
//	}

	// void updateTransformLoc(int loc) {
	// 	*transformIdRef = loc;
	// }
	COPY(_renderer);
};

class camera : public component
{
	
};