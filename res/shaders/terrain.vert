#version 430 core
#include "transform.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec2 uvs2;
layout (location = 3) in vec3 normal;


//out float logz;
out vec2 TexCoord;
out vec2 TexCoord2;
out vec3 Normal;
out vec3 FragPos;
out float logz;

uniform mat4 model;
uniform mat4 normalMat;
uniform mat4 mvp;
uniform float FC;

void main()
{
	gl_Position = mvp * vec4(position,1);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;

//	DirLightSpacePos = dirLightTransform * model * vec4(position, 1.0);

	TexCoord = texCoord;
	TexCoord2 = uvs2;
	FragPos = (model * vec4(position,1.0f)).xyz;
	Normal = mat3(normalMat) * normal;
//	}
}
