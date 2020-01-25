#version 430 core
 #extension GL_ARB_conservative_depth : enable

layout(depth_less) out float gl_FragDepth;

struct Material{
	sampler2D texture_diffuse0;
	sampler2D texture_specular0;
	sampler2D texture_normal0;
	float shininess;
};

struct DirLight{
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
struct PointLight{
	vec3 position;
	float constant;

	vec3 ambient;
	float linear;

	vec3 diffuse;
	float quadratic;

	vec3 specular;
};
struct SpotLight{
	vec3 position;
	float cutoff;

	vec3 direction;
	float outerCutOff;

	vec3 ambient;
	float constant;

	vec3 diffuse;
	float linear;

	vec3 specular;
	float quadratic;
};
struct OmniShadowMap{
	samplerCube shadowMap;
	float farPlane;
};

uniform float screenHeight;
uniform float screenWidth;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 DirLightSpacePos;
in float logz;

uniform vec3 viewPos;
uniform Material material;
uniform sampler2D directionalShadowMap;
//uniform float farPlane;
uniform OmniShadowMap omniShadowMap;
//uniform PointLight light;

layout(std430, binding = 0) buffer dlights {
int numDLights;
	DirLight dirlights[];
};
layout(std430, binding = 1) buffer plights {
int numPLights;
	PointLight pointlights[];
};
layout(std430, binding = 2) buffer slights {
int numSLights;
	SpotLight spotlights[];
};


out vec4 color;


float CalcDirectionalShadowFactor(DirLight light)
{
	vec3 projCoords = DirLightSpacePos.xyz / DirLightSpacePos.w;
	projCoords = (projCoords * 0.5) + 0.5;
//	projCoords *= 10;
//projCoords.z = 1 - projCoords.z;
	float current = projCoords.z;
	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(light.direction);
	float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.0001);
//	float bias = 0.00001;

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(directionalShadowMap, 0);
	for(int x = -1; x <= 1; ++x)
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(directionalShadowMap, projCoords.xy + vec2(x,y) * texelSize).r;
			shadow += current - bias > pcfDepth ? 1.0 : 0.0;
		}

	shadow /= 9.0;
	if(projCoords.z > 1.0 || projCoords.z < 0 || projCoords.x > 1 || projCoords.x < 0 || projCoords.y > 1 || projCoords.y < 0)
		shadow = 0.0;
//	else shadow = 1;

//	return projCoords.x;
	return 1 - shadow;
}
float calcOmniShadowFactor(PointLight light){
	vec3 fragToLight = FragPos - light.position;
	float closest = texture(omniShadowMap.shadowMap,fragToLight).r;
	closest *= omniShadowMap.farPlane;
	float current = length(fragToLight);

	float bias = 0.5;
	float shadow = current - bias > closest ? 1.0 : 0.0;
	return 1 - shadow;
}

vec3 getAmbient(vec3 lightColor, vec3 color){
	return lightColor * color;
}

vec3 getDiffuse(vec3 lightCol, vec3 lightDir,vec3 normal,vec3 color){
	// diffuse
	float diff = max(dot(normal,lightDir),0.0);
	return lightCol * diff * color;
}
vec3 getSpecular(vec3 lightCol, vec3 lightDir,vec3 normal,vec3 viewDir,vec3 color, vec3 specColor){
	// specular
	vec3 reflectDir = reflect(lightDir, normal);
	if(length(lightDir - normal) > sqrt(2))
	{
		return vec3(0);
	}
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	return  lightCol * spec * specColor;
}

float getAttenuation(vec3 lightPos, float constant, float linear, float quadratic){
// attenuation
	float dist = length(lightPos - FragPos);
	float a = (constant + linear * dist + quadratic * (dist * dist));
	if(a == 0)
		return 0;
	else
	return 1.0f / a;
}

float getIntensity(vec3 lightDir,vec3 lightsDir, float cutoff, float outerCutOff){
	// spotlight
	float theta = dot(lightDir, normalize(-lightsDir));
	float epsilon = (cutoff - outerCutOff);
	if(epsilon == 0)
		return 0;
	else
		return clamp((theta - outerCutOff) / epsilon, 0.0,1.0);
}


vec3 getDirLight(DirLight light, vec3 normal,vec3 viewDir,vec3 color, vec3 specColor){
	return getAmbient(light.ambient,color) + (getDiffuse(light.diffuse,light.direction,normal,color) + getSpecular(light.specular,light.direction,normal,viewDir,color,specColor));
//	* CalcDirectionalShadowFactor(light);
}
vec3 getpointLight(PointLight light, vec3 normal, vec3 viewDir,vec3 color, vec3 specColor){
	vec3 lightDir = normalize(light.position - FragPos);
	return (getAmbient(light.ambient,color) + getDiffuse(light.diffuse,lightDir,normal,color) + getSpecular(light.specular,lightDir,normal,viewDir,color,specColor))
	* getAttenuation(light.position,light.constant,light.linear,light.quadratic) * calcOmniShadowFactor(light);
}
vec3 getSpotLight(SpotLight light, vec3 normal,vec3 viewDir,vec3 color, vec3 specColor){
	vec3 lightDir = normalize(light.position - FragPos);
	return (getAmbient(light.ambient,color) + getDiffuse(light.diffuse,lightDir,normal,color) + getSpecular(light.specular,lightDir,normal,viewDir,color,specColor))
	* getAttenuation(light.position,light.constant,light.linear,light.quadratic)
	* getIntensity(lightDir,light.direction,light.cutoff,light.outerCutOff);
}



uniform float FC;
void main()
{
// debug / fun shaders
//	color.rgb = vec3(texture(directionalShadowMap,DirLightSpacePos.xy * 0.5 + 0.5).rgb);
//	color.rgb = vec3(DirLightSpacePos.z*.5 + 0.5,0,0);
//	color.rgb = vec3(gl_FragCoord.z);
//	color.rgb = (normalize(Normal) + vec3(1)) / 2;

	vec3 diffuseColor = texture(material.texture_diffuse0,TexCoord).rgb;
	vec3 specColor = texture(material.texture_specular0,TexCoord).rgb;
	if(diffuseColor == vec3(0,0,0)){
		 diffuseColor = vec3(1,1,1);
		 specColor = vec3(1,1,1);
	}
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);
	DirLight dl;
    dl.direction = vec3(1.f);
	dl.ambient = vec3(0.2f);
	dl.diffuse = vec3(0.5f);
	dl.specular = vec3(1.f);

	color = vec4(0);
//	color.rgb += vec3(0,0,getDirLight(dirlights[0],norm,viewDir,diffuseColor,specColor).b);
	color.rgb += getDirLight(dl,norm,viewDir,diffuseColor,specColor);

//	color.rgb = diffuseColor;


//	for(int i = 0; i < numDLights; i++)
//		color += vec4(getDirLight(dirlights[i],norm,viewDir,diffuseColor,specColor),0.0f);
//	for(int i = 0; i < numPLights; i++)
//		color += vec4(getpointLight(pointlights[i],norm,viewDir,diffuseColor,specColor),0.0f);
//	for(int i = 0; i < numSLights; i++)
//		color += vec4(getSpotLight(spotlights[i],norm,viewDir,diffuseColor,specColor),0.0f);
//

	color.a = 1.0f;
	gl_FragDepth = log2(logz) * 0.5 * FC;
}
