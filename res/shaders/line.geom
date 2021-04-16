
#version 430 core

struct line{
    vec3 p1;
    vec3 p2;
    vec4 color;
};

//vector(matrixes, matrix, 3);
layout(std430, binding = 0) buffer l{
	line lines[];
};

uniform mat4 vp;
uniform float FC;
uniform vec3 camDir;

in uint index_[];

out float logz;
out vec4 col;

void makePoint(vec3 p){
	gl_Position = vp * vec4(p,1);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
    EmitVertex();
}

layout (points) in;
layout (triangle_strip, max_vertices=4) out;
void main()
{
    uint id = index_[0];
    vec3 width = normalize(cross((lines[id].p1 - lines[id].p2), camDir)) * 1.f;
    col = lines[id].color;

    makePoint(lines[id].p1 + width);
    makePoint(lines[id].p1 - width);
    makePoint(lines[id].p2 + width);
    makePoint(lines[id].p2 - width);
}
