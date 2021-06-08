#pragma once
#include "_rendering/_renderMeta.h"
void renderEdit(const char *name, _model &m);
void renderEdit(const char *name, _shader &s);
#include "_serialize.h"
// #include "components/Component.h"
#include "components/game_object.h"
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
	__renderer();
	__renderer(uint transform, uint id);
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
	// typename fast_list_deque<GLuint>::iterator transformIdRef;
	int transformIdRef = -1;
	friend void run();
	friend void makeBillboard(_model m, _texture t, _renderer *r);

public:
	void setCullSizes(float min, float max);
	_model getModel();
	_shader getShader();
	void onEdit();
	void init(int id);
	_renderer();
	void set(_shader s, _model m);
	void set_proto(_shader s, _model m);
	void set(renderingMeta *_meta);
	_renderer(const _renderer &other);
	void deinit(int id);
	COPY(_renderer);
	SER_FUNC()

	switch (x)
	{
	case ser_mode::edit_mode:
	{
		_shader curr_s = shader;
		_model curr_m = model;
		renderEdit("shader", shader);
		renderEdit("model", model);
		if (model.m != curr_m.m || shader.s != curr_s.s)
		{
			this->set(shader, model);
		}
	}
	break;
	case ser_mode::read_mode:
		shader = node_9738469372465["shader"].as<decltype(shader)>();
		model = node_9738469372465["model"].as<decltype(model)>();
		// this->set(shader, model);
		break;
	case ser_mode::write_mode:
		node_9738469372465["shader"] = shader;
		node_9738469372465["model"] = model;
		break;
	default:
		cout << "no mode provided";
		break;
	}

	SER_END
};

extern gpu_vector_proxy<matrix> *GPU_MATRIXES;
extern int __RENDERERS_in_size;
extern gpu_vector_proxy<__renderer> *__RENDERERS_in;
// extern gpu_vector_proxy<__renderer> *__RENDERERS_out;
extern gpu_vector<GLuint> *__renderer_offsets;
extern gpu_vector<__renderMeta> *__rendererMetas;
