#version 430 core
 #extension GL_ARB_conservative_depth : enable

layout(depth_less) out float gl_FragDepth;

struct Material{
	sampler2D texture_diffuse0;
	sampler2D texture_diffuse1;
	sampler2D texture_diffuse2;
	sampler2D texture_specular0;
	sampler2D texture_normal0;
	float shininess;
};



in vec2 TexCoord;
in vec2 TexCoord2;
in vec3 Normal;
in vec3 FragPos;
in vec4 DirLightSpacePos;
in float logz;

uniform vec3 viewPos;
uniform Material material;
// uniform sampler2D directionalShadowMap;
//uniform float farPlane;
// uniform OmniShadowMap omniShadowMap;
//uniform PointLight light;



// out vec4 color;
layout(location = 0) out vec4 gAlbedoSpec;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;




uniform float FC;
void main()
{
// debug / fun shaders
//	color.rgb = vec3(texture(directionalShadowMap,DirLightSpacePos.xy * 0.5 + 0.5).rgb);
//	color.rgb = vec3(DirLightSpacePos.z*.5 + 0.5,0,0);
//	color.rgb = vec3(gl_FragCoord.z);
//	color.rgb = (normalize(Normal) + vec3(1)) / 2;

	vec3 diffuseColor0 = texture(material.texture_diffuse0,TexCoord).rgb;
	vec3 diffuseColor1 = texture(material.texture_diffuse1,TexCoord).rgb;
	vec3 diffuseColor2 = texture(material.texture_diffuse2,TexCoord2).rgb;
// 	vec3 specColor = texture(material.texture_specular0,TexCoord).rgb;
	if(diffuseColor0 == vec3(0,0,0)){
		 diffuseColor0 = vec3(1,1,1);
		//  specColor = vec3(1,1,1);
	}
	float r = dot(normalize(Normal), vec3(0,1,0));
	r = clamp(r * 10 - 8.2,0,1);

	// color.a = 1.0f;
    gPosition = vec4(FragPos,1);
    gNormal = vec4(normalize(Normal),1);
    gAlbedoSpec.rgb = diffuseColor0 * r + (diffuseColor1 * (1 - r) + diffuseColor2 * (1 - r)) * 0.5f;
    gAlbedoSpec.a = 0;//texture(material.texture_specular0,TexCoord).r;
	gl_FragDepth = log2(logz) * 0.5 * FC;
}
