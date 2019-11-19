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
using namespace std;


struct matrix {
	glm::mat4 mvp;
	glm::mat4 model;
	glm::mat4 normal;
};
gpu_vector_proxy<matrix>* GPU_MATRIXES;
//gpu_vector<GLuint> ids;

array_heap<bool> GPU_MATRIXES_IDS = array_heap<bool>();



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
fast_list<__renderer> gpu_renderers = fast_list<__renderer>();
gpu_vector<__renderer>* GPU_RENDERERS;

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
	_shaderMeta(string vertex, string fragment) {
		name = vertex + fragment;
		shader = new Shader(vertex, fragment);
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
	_shaderMeta* s = 0;
};



int renderingId = 0;
struct renderingMeta {
	GLuint id = -1;
	gpu_vector<GLuint>* _ids;
	fast_list<GLuint> ids = fast_list<GLuint>();
	_shader s;
	_model m;
	renderingMeta() {
		//_ids = new gpu_vector<GLuint>();
		//_ids->storage = &ids.data;
	}
	renderingMeta(_shader _s, _model _m) {
		s = _s;
		m = _m;
		_ids = new gpu_vector<GLuint>();
		_ids->ownStorage();
	}

	//renderingMeta operator=(const renderingMeta& other) {
	//	return renderingMeta(other);
	//}
	renderingMeta(const renderingMeta& other) {
		s = other.s;
		m = other.m;
		//ids = fast_list<GLuint>(other.ids);
		_ids = new gpu_vector<GLuint>();
		_ids->storage = &ids.data;
	}
	~renderingMeta() {
		if (_ids != 0) {
			delete _ids;
		}
	}

};

class {
public:
	map<string, map<string, renderingMeta*> > shader_model_vector;
	map<GLuint, renderingMeta*> renderingMetas;
	mutex lock;

}renderingManager;

class _renderer : public component {
	_shader shader;
	_model model;
	array_heap<bool>::ref matrixLoc;
	fast_list<GLuint>::iterator idLoc;
	renderingMeta* meta = 0;
	fast_list<__renderer>::iterator _Rloc;



public:
	void onStart() {
		if (shader.s == 0 || model.m == 0)
			return;
		set(shader, model);
		transform->gameObject->renderer = this;
	}
	_renderer() {}

	void set(_shader s, _model m) {
		rendererLock.lock();
		shader = s;
		model = m;
		if (meta != 0) {
			meta->ids.erase(idLoc);
			GPU_MATRIXES_IDS._delete(matrixLoc);
			gpu_renderers.erase(_Rloc);
		}
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

		matrixLoc = GPU_MATRIXES_IDS._new();
		idLoc = r->second[m.m->name]->ids.push_back(matrixLoc);
		meta = (r->second[m.m->name]);
		_Rloc = gpu_renderers.push_back(__renderer(transform->_T, matrixLoc));
		rendererLock.unlock();

	}
	_renderer(const _renderer& other) {
//		if (other.meta != 0) {
			shader = other.shader;
			model = other.model;
//		}
	}
	~_renderer() {
		if (meta != 0) {
			rendererLock.lock();
			meta->ids.erase(idLoc);
			GPU_MATRIXES_IDS._delete(matrixLoc);
			gpu_renderers.erase(_Rloc);
			rendererLock.unlock();
		}
	}

	void updateTransformLoc(int loc) {
		_Rloc->transform = loc;
	}
	COPY(_renderer);
};

