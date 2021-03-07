#include "_rendering/_renderMeta.h"
#include "component.h"

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

class _renderer final : public component
{
	_shader shader;
	_model model;
	renderingMeta *meta = 0;
	// typename list<GLuint>::iterator transformIdRef;
	typename fast_list_deque<GLuint>::iterator transformIdRef;
	// typename deque_heap<GLuint>::ref transformIdRef;
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
	// COPY(_renderer);
	SER2(shader, model);
};
void renderEdit(const char *name, _model &m);
void renderEdit(const char *name, _shader &s);

extern gpu_vector_proxy<matrix> *GPU_MATRIXES;

extern gpu_vector<__renderer> *__RENDERERS_in;
// extern gpu_vector_proxy<__renderer> *__RENDERERS_out;
extern gpu_vector<GLuint> *__renderer_offsets;
extern gpu_vector<__renderMeta> *__rendererMetas;