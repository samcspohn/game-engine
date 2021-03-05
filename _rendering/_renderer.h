#include "_rendering/_renderMeta.h"
#include "component.h"

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
	// COPY(_renderer);
	SER2(shader, model);
};
void renderEdit(const char *name, _model &m);
void renderEdit(const char *name, _shader &s);