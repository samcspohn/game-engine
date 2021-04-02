#version 430 core
#extension GL_ARB_conservative_depth : enable
layout(depth_less) out float gl_FragDepth;

struct Material{
	sampler2D texture_diffuse0;
	sampler2D texture_specular0;
	sampler2D texture_normal0;
	float shininess;
};
uniform vec3 viewPos;
uniform Material material;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 DirLightSpacePos;
in float logz;

layout(location = 0) out vec4 gAlbedoSpec;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;

uniform float FC;
void main()
{
	gl_FragDepth = log2(logz) * 0.5 * FC;
	vec4 color;
// debug / fun shaders
//	color.rgb = vec3(texture(directionalShadowMap,DirLightSpacePos.xy * 0.5 + 0.5).rgb);
//	color.rgb = vec3(DirLightSpacePos.z*.5 + 0.5,0,0);
//	color.rgb = vec3(gl_FragCoord.z);
	color.rgb = (normalize(Normal) + vec3(1)) / 2;
	vec4 diffuseColor = texture(material.texture_diffuse0,TexCoord);
    vec4 specColor = texture(material.texture_specular0,TexCoord);
	if(diffuseColor.a == 0 && specColor == vec4(0,0,0,1))
		discard;
	if(diffuseColor == vec4(0,0,0,1)){
		 diffuseColor = vec4(1,1,1,1);
	}
	// color.rgb = diffuseColor.rgb;

    gPosition = vec4(FragPos,1);
    gNormal = vec4(normalize(Normal),1);
    gAlbedoSpec.rgb = color.rgb;
    gAlbedoSpec.a = 1.f;//specColor.r;
}
