
struct _transform {
	vec3 position; int id;
	vec3 scale; int parent;
	vec4 rotation;
};

struct matrix{
	mat4 mvp;
	mat4 model;
	mat4 normal;
};
