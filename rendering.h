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
extern gpu_vector_proxy<matrix>* GPU_MATRIXES;

class _renderer;

// model data
struct _modelMeta {
	_modelMeta();
	_modelMeta(string file);
	~_modelMeta();
	string name;
	Model* model = 0;
	glm::vec3 bounds;
	float radius;
	void getBounds();
	bool unique = false;
};
namespace modelManager {
	extern map<string, _modelMeta*> models;
	void destroy();
};

class _model {
public:
	_model();
	_model(string fileName);
	_modelMeta* m = 0;
	vector<Mesh>& meshes();
	Mesh& mesh();
	void makeUnique();
	void recalcBounds();
};


//shader data
struct _shaderMeta {
	_shaderMeta();
	_shaderMeta(string compute);
	_shaderMeta(string vertex, string fragment);
	_shaderMeta( string vertex, string geom, string fragment);
	~_shaderMeta();
	string name;
	Shader* shader = 0;
};

namespace shaderManager {

	extern map<string, _shaderMeta*> shaders;
	void destroy();
};

class _shader {
public:
	_shader();
	_shader(string compute);
	_shader(string vertex, string fragment);
	_shader(string vertex, string geom,  string fragment);
	_shaderMeta* s = 0;
};



extern int renderingId;
struct renderingMeta {
	gpu_vector<GLuint>* _transformIds;
	fast_list_deque<GLuint> ids;
	
	_shader s;
	_model m;

	renderingMeta(_shader _s, _model _m);
private:
	renderingMeta(const renderingMeta& other);
};

namespace renderingManager {
	extern mutex m;

	extern map<string, map<string, renderingMeta*> > shader_model_vector;
	void destroy();
	void lock();
	void unlock();
};

void destroyRendering();

class _renderer : public component {
	_shader shader;
	_model model;
	renderingMeta* meta = 0;
	typename fast_list_deque<GLuint>::iterator transformIdRef;
	friend void run();
public:
	_model getModel();
	void onStart();
	_renderer();
	void set(_shader s, _model m);
	void set_proto(_shader s, _model m);
	void set(renderingMeta* _meta);
	_renderer(const _renderer& other);
	void onDestroy();
	COPY(_renderer);
};

// class camera : public component
// {
	
// };