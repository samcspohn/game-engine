#pragma once
#include <map>
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
#include "editor.h"
using namespace std;

struct matrix
{
	glm::mat4 mvp;
	glm::mat4 model;
	glm::mat4 normal;
};
struct __renderer
{
	uint transform;
	uint id;
};
struct __renderMeta
{
	GLfloat radius;
	GLfloat min;
	GLfloat max;
	uint isBillboard;
};
extern gpu_vector_proxy<matrix> *GPU_MATRIXES;

extern gpu_vector<__renderer> *__RENDERERS_in;
// extern gpu_vector<GLuint>* __RENDERERS_keys_in;
// extern gpu_vector_proxy<__renderer>* __RENDERERS_out;
// extern gpu_vector_proxy<GLuint>* __RENDERERS_keys_out;

extern gpu_vector<GLuint> *__renderer_offsets;
extern gpu_vector<__renderMeta> *__rendererMetas;

class _renderer;

// model data
class _model;
struct _modelMeta : public assets::asset
{
	_modelMeta();
	_modelMeta(string file);
	~_modelMeta();
	bool onEdit();
	string type();
	string file;
	Model *model = 0;
	glm::vec3 bounds;
	float radius;
	void getBounds();
	void inspect();
	bool unique = false;
	friend class _model;
	SER_HELPER()
	{
		SER_BASE_ASSET
		ar &name &model &bounds &radius &unique;
	}
};
namespace modelManager
{
	extern map<size_t, _modelMeta *> models;
	extern map<int, _modelMeta *> models_id;
	void destroy();
	void save(OARCHIVE &oa);
	void load(IARCHIVE &ia);
}; // namespace modelManager

class _model
{
public:
	_model();
	_model(string fileName);
	vector<Mesh> &meshes();
	Mesh &mesh();
	void makeUnique();
	void makeProcedural();
	void recalcBounds();
	_modelMeta *meta() const;
	// _modelMeta* m = 0;
	int m = 0;
	SER_HELPER()
	{
		ar &m;
	}
};

//shader data
struct _shader;
struct _shaderMeta : public assets::asset
{
	_shaderMeta();
	_shaderMeta(string compute);
	_shaderMeta(string vertex, string fragment);
	_shaderMeta(string vertex, string geom, string fragment);
	_shaderMeta(string vertex, string tess, string geom, string fragment);
	~_shaderMeta();
	bool onEdit();
	void inspect();
	string type();
	Shader *shader = 0;
	friend struct _shader;
	SER_HELPER()
	{
		SER_BASE_ASSET
		ar &shader;
	}
};

namespace shaderManager
{

	extern map<size_t, _shaderMeta *> shaders;
	extern map<int, _shaderMeta *> shaders_ids;
	void destroy();
	void save(OARCHIVE &oa);
	void load(IARCHIVE &ia);
}; // namespace shaderManager

class _shader
{
public:
	_shader();
	_shader(string compute);
	_shader(string vertex, string fragment);
	_shader(string vertex, string geom, string fragment);
	_shader(string vertex, string tess, string geom, string fragment);
	// _shaderMeta* s = 0;
	int s = 0;
	Shader &ref();
	Shader *operator->();
	_shaderMeta *meta() const;
	SER_HELPER()
	{
		ar &s;
	}
};

extern int renderingId;
struct renderingMeta
{
	// gpu_vector<__renderer>* __renderers;
	fast_list_deque<GLuint> ids;

	_shader s;
	_model m;
	float minRadius = 0.f;
	float maxRadius = INFINITY;
	uint isBillboard = 0;
	renderingMeta(_shader _s, _model _m);

private:
	renderingMeta(const renderingMeta &other);
};

namespace renderingManager
{
	extern mutex m;

	extern map<int, map<int, renderingMeta *>> shader_model_vector;
	void destroy();
	void lock();
	void unlock();
}; // namespace renderingManager
struct batchElement
{
	renderingMeta *r;
	Mesh *m;
	// int indexSize;
	// int pointSize;
	double getHash();
};
struct batch
{
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
bool operator<(const texArray &l, const texArray &r);
bool operator<(const _shader &l, const _shader &r);
namespace batchManager
{
	extern mutex m;
	// shader id, textureArray hash, mesh id
	extern queue<map<_shader, map<texArray, map<renderingMeta *, Mesh *>>>> batches;
	extern map<_shader, map<texArray, batch>> batches2;
	// extern map<_shader,map<texArray,batch>> batches2;

	map<_shader, map<texArray, map<renderingMeta *, Mesh *>>> &updateBatches();
}; // namespace batchManager

void destroyRendering();

class _renderer final : public component
{
	_shader shader;
	_model model;
	renderingMeta *meta = 0;
	typename fast_list_deque<GLuint>::iterator transformIdRef;
	friend void run();
	friend void makeBillboard(_model m, _texture t, _renderer *r);

public:
	void setCullSizes(float min, float max);
	_model getModel();
	_shader getShader();
	void onEdit();
	void onStart();
	_renderer();
	void set(_shader s, _model m);
	void set_proto(_shader s, _model m);
	void set(renderingMeta *_meta);
	_renderer(const _renderer &other);
	void onDestroy();
	COPY(_renderer);
	SER2(shader, model);
};

void renderEdit(const char *name, _model &m);
void renderEdit(const char *name, _shader &s);
transform2 renderRaycast(glm::vec3 p, glm::vec3 dir);