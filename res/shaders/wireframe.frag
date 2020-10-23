#version 430 core
 #extension GL_ARB_conservative_depth : enable

layout(depth_less) out float gl_FragDepth;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in float logz;

uniform vec3 viewPos;

// out vec4 color;
layout(location = 0) out vec4 gAlbedoSpec;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;


uniform float FC;
void main()
{
    gPosition = vec4(FragPos,1);
    gNormal = vec4(normalize(Normal),1);
    gAlbedoSpec.rgb = vec3(1);
    gAlbedoSpec.a = 1;
	gl_FragDepth = log2(logz) * 0.5 * FC;
}
