//?#version 430



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

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
uniform vec3 viewPos;
uniform Material material;

layout(binding = 0) buffer dlights {
int numDLights;
	DirLight dirlights[];
};
layout(binding = 1) buffer plights {
int numPLights;
	PointLight pointlights[];
};
layout(binding = 2) buffer slights {
int numSLights;
	SpotLight spotlights[];
};


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
	vec3 reflectDir = reflect(-lightDir, normal);
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
	return getAmbient(light.ambient,color) + getDiffuse(light.diffuse,light.direction,normal,color) + getSpecular(light.specular,light.direction,normal,viewDir,color,specColor);
}
vec3 getpointLight(PointLight light, vec3 normal, vec3 viewDir,vec3 color, vec3 specColor){
	vec3 lightDir = normalize(light.position - FragPos);
	return (getAmbient(light.ambient,color) + getDiffuse(light.diffuse,lightDir,normal,color) + getSpecular(light.specular,lightDir,normal,viewDir,color,specColor)) 
	* getAttenuation(light.position,light.constant,light.linear,light.quadratic);
}
vec3 getSpotLight(SpotLight light, vec3 normal,vec3 viewDir,vec3 color, vec3 specColor){
	vec3 lightDir = normalize(light.position - FragPos);
	return (getAmbient(light.ambient,color) + getDiffuse(light.diffuse,lightDir,normal,color) + getSpecular(light.specular,lightDir,normal,viewDir,color,specColor)) 
	* getAttenuation(light.position,light.constant,light.linear,light.quadratic)
	* getIntensity(lightDir,light.direction,light.cutoff,light.outerCutOff);
}

vec3 getLighting(){
	vec3 color;
	vec3 diffuseColor = texture(material.texture_diffuse0,TexCoord).rgb;
	vec3 specColor = texture(material.texture_specular0,TexCoord).rgb;
	if(diffuseColor == vec3(0,0,0)){
		 diffuseColor = vec3(1,1,1);
		 specColor = vec3(1,1,1);
	}
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);
//	color = texture(material.diffuse,TexCoord);
	for(int i = 0; i < numDLights; i++)
		color += vec4(getDirLight(dirlights[i],norm,viewDir,diffuseColor,specColor),0.0f);
	for(int i = 0; i < numPLights; i++)
		color += vec4(getpointLight(pointlights[i],norm,viewDir,diffuseColor,specColor),0.0f);
	for(int i = 0; i < numSLights; i++)
		color += vec4(getSpotLight(spotlights[i],norm,viewDir,diffuseColor,specColor),0.0f);
	return color;
}