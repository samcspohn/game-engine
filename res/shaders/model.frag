#version 430 core
 #extension GL_ARB_conservative_depth : enable

layout(depth_less) out float gl_FragDepth;

struct Material{
	sampler2D texture_diffuse0;
	sampler2D texture_specular0;
	sampler2D texture_normal0;
	float shininess;
};



in vec2 TexCoord;
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
	vec4 diffuseColor = texture(material.texture_diffuse0,TexCoord);
	if(diffuseColor.a == 0)
		discard;
// 	vec3 specColor = texture(material.texture_specular0,TexCoord).rgb;
	if(diffuseColor == vec4(0,0,0,1)){
		 diffuseColor = vec4(1,1,1,1);
		//  specColor = vec3(1,1,1);
	}

	// color.a = 1.0f;
    // store the fragment position vector in the first gbuffer texture
    gPosition = vec4(FragPos,1);
    // also store the per-fragment normals into the gbuffer
    gNormal = vec4(normalize(Normal),1);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = diffuseColor.rgb;//texture(material.texture_diffuse0,TexCoord).rgb;
    // gAlbedoSpec.a = 0.5;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(material.texture_specular0,TexCoord).r;
	gl_FragDepth = log2(logz) * 0.5 * FC;
}
