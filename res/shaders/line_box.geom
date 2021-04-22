
#version 430 core

struct line{
    vec3 p1;
    vec3 p2;
    vec4 color;
};

//vector(matrixes, matrix, 3);
layout(std430, binding = 0) buffer l{
	line boxes[];
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

void makeLine(vec3 p1, vec3 p2){
    vec3 width = normalize(cross((p1 - p2), camDir)) * 1.f;
    makePoint(p1 + width);
    makePoint(p1 - width);
    makePoint(p2 + width);
    makePoint(p2 - width);
    EndPrimitive();
}

layout (points) in;
layout (triangle_strip, max_vertices=48) out;
void main()
{
    uint id = index_[0];
    col = boxes[id].color;
    vec3 p = boxes[id].p1;
    vec3 dims = boxes[id].p2;

    vec3 x = vec3(dims.x, 0, 0);
    vec3 y = vec3(0, dims.y, 0);
    vec3 z = vec3(0, 0, dims.z);
    vec3 p1, p2, p3, p4, p5, p6, p7, p8, p9;
    p1 = p;
    p2 = p + x;
    p3 = p + x + z;
    p4 = p + z;
    p5 = p + y;
    p6 = p + x + y;
    p7 = p + x + z + y;
    p8 = p + z + y;

    // // bottom square
    makeLine(p1, p2);
    makeLine(p2, p3);
    makeLine(p3, p4);
    makeLine(p4, p1);
    // top square
    makeLine(p5, p6);
    makeLine(p6, p7);
    makeLine(p7, p8);
    makeLine(p8, p5);

    makeLine(p1, p5);
    makeLine(p2, p6);
    makeLine(p3, p7);
    makeLine(p4, p8);    
}
