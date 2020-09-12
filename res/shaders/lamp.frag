#version 430 core
 #extension GL_ARB_conservative_depth : enable

layout(depth_less) out float gl_FragDepth;

in vec3 Normal;
in vec3 FragPos;

in float logz;

layout(location = 0) out vec4 gAlbedoSpec;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;


uniform float farPlane;
const float FC = 2.0 / log2(farPlane + 1);

void main()
{
	gPosition = vec4(FragPos,1);
    // also store the per-fragment normals into the gbuffer
    gNormal = vec4(normalize(-Normal),1);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = vec3(1,0.95,0.7);
    // gAlbedoSpec.a = 0.5;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = 1;
	gl_FragDepth = log2(logz) * 0.5 * FC;
}