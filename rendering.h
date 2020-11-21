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
#include "texture.h"
// #include <hash_fun.h>
#include <functional>
#include "serialize.h"
using namespace std;


struct matrix {
	glm::mat4 mvp;
	glm::mat4 model;
	glm::mat4 normal;
};
struct __renderer {
	uint transform;
	uint id;
};
struct __renderMeta{
	GLfloat radius;
	GLfloat min;
	GLfloat max;
	uint isBillboard;
};
extern gpu_vector_proxy<matrix>* GPU_MATRIXES;

extern gpu_vector<__renderer>* __RENDERERS_in;
// extern gpu_vector<GLuint>* __RENDERERS_keys_in;
// extern gpu_vector_proxy<__renderer>* __RENDERERS_out;
// extern gpu_vector_proxy<GLuint>* __RENDERERS_keys_out;

extern gpu_vector<GLuint>* __renderer_offsets;
extern gpu_vector<__renderMeta>* __rendererMetas;

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
	SER_HELPER(){
		ar & name & model & bounds & radius & unique;
	}
};
namespace modelManager {
	extern map<size_t, _modelMeta*> models;
	void destroy();
	void save(OARCHIVE& oa);
	void load(IARCHIVE& ia);
};

class _model {
public:
	_model();
	_model(string fileName);
	vector<Mesh>& meshes();
	Mesh& mesh();
	void makeUnique();
	void makeProcedural();
	void recalcBounds();
	_modelMeta* meta() const;
	// _modelMeta* m = 0;
	size_t m = 0;
	SER_HELPER(){
		ar & m;
	}
};


//shader data
struct _shaderMeta {
	_shaderMeta();
	_shaderMeta(string compute);
	_shaderMeta(string vertex, string fragment);
	_shaderMeta( string vertex, string geom, string fragment);
	_shaderMeta( string vertex, string tess, string geom, string fragment);
	~_shaderMeta();
	string name;
	Shader* shader = 0;
	SER_HELPER(){
		ar & name & shader;
	}
};

namespace shaderManager {

	extern map<size_t, _shaderMeta*> shaders;
	void destroy();
	void save(OARCHIVE& oa);
	void load(IARCHIVE& ia);
};

class _shader {
public:
	_shader();
	_shader(string compute);
	_shader(string vertex, string fragment);
	_shader(string vertex, string geom,  string fragment);
	_shader(string vertex, string tess, string geom,  string fragment);
	// _shaderMeta* s = 0;
	size_t s = 0;
	Shader& ref();
	Shader* operator->();
	_shaderMeta* meta() const;
	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */)
	{
		ar & s;
	}
};


extern int renderingId;
struct renderingMeta {
	// gpu_vector<__renderer>* __renderers;
	fast_list_deque<GLuint> ids;
	
	_shader s;
	_model m;
	float minRadius = 0.f;
	float maxRadius = INFINITY;
	uint isBillboard = 0;
	renderingMeta(_shader _s, _model _m);
private:
	renderingMeta(const renderingMeta& other);
};

namespace renderingManager {
	extern mutex m;

	extern map<size_t, map<size_t, renderingMeta *>> shader_model_vector;
	void destroy();
	void lock();
	void unlock();
};
struct batchElement{
	renderingMeta*r;
	Mesh* m;
	// int indexSize;
	// int pointSize;
	double getHash();
};
struct batch{
	list<batchElement> elements;
    GLuint VAO = 0, VBO = 0, EBO = 0;
	double hash = 10;
	int vertexSize = 0;
	int indexSize = 0;
	vector<GLuint> indices;
	void init();
	void addElement(batchElement b);
	void finalize();
};
bool operator<(const texArray& l,const texArray& r);
bool operator<(const _shader& l,const _shader& r);
namespace batchManager{
	extern mutex m;
	// shader id, textureArray hash, mesh id
	extern queue<map<_shader,map<texArray,map<renderingMeta*,Mesh*>>>> batches;
	extern map<_shader,map<texArray,batch>> batches2;
	// extern map<_shader,map<texArray,batch>> batches2;

	map<_shader,map<texArray,map<renderingMeta*,Mesh*>>>& updateBatches();
};

void destroyRendering();

class _renderer final : public component {
	_shader shader;
	_model model;
	renderingMeta* meta = 0;
	typename fast_list_deque<GLuint>::iterator transformIdRef;
	friend void run();
	friend void makeBillboard(_model m, _texture t, _renderer* r);
public:
	void setCullSizes(float min, float max);
	_model getModel();
	_shader getShader();
	void onStart();
	_renderer();
	void set(_shader s, _model m);
	void set_proto(_shader s, _model m);
	void set(renderingMeta* _meta);
	_renderer(const _renderer& other);
	void onDestroy();
	void onEdit();
	COPY(_renderer);
	SER2(shader, model);
};