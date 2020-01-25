
struct _transform {
	vec3 position; int id;
	vec3 scale; int parent;
	vec4 rotation;
	int childrenBegin;
	int childrenEnd;
	int prevSibling;
	int nextSibling;
};