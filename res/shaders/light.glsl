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

int numDLights;
layout(binding = 0) buffer dlights {
	DirLight dirlights[];
};
int numPLights;
layout(binding = 1) buffer plights {
	PointLight pointlights[];
};
layout(binding = 2) buffer slights {
	SpotLight spotlights[];
};


vec3 getAmbient(vec3 lightColor, vec3 ambientColor){
	return lightColor * ambientColor;
}

vec3 getDiffuse(vec3 lightCol, vec3 lightDir,vec3 normal,vec3 diffuseColor){
	// diffuse
	float diff = max(dot(normal,lightDir),0.0);
	return lightCol * diff * diffuseColor;
}
vec3 getSpecular(vec3 lightCol, vec3 lightDir,vec3 normal,vec3 viewDir, vec3 specularColor){
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	return  lightCol * spec * specularColor;
}

float getAttenuation(vec3 lightPos, float constant, float linear, float quadratic){
// attenuation
	float dist = length(lightPos - FragPos);
	return 1.0f / (constant + linear * dist + quadratic * (dist * dist));
}

float getIntensity(vec3 lightDir,vec3 lightsDir, float cutoff, float outerCutOff){
	// spotlight
	float theta = dot(lightDir, normalize(-lightsDir));
	float epsilon = (cutoff - outerCutOff);
	return clamp((theta - outerCutOff) / epsilon, 0.0,1.0);
}


vec3 getDirLight(DirLight light, vec3 normal,vec3 viewDir, vec3 ambientColor, vec3 diffuseColor, vec3 SpecularColor){
	return getAmbient(light.ambient,ambientColor) + getDiffuse(light.diffuse,light.direction,normal,diffuseColor) + getSpecular(light.specular,light.direction,normal,viewDir,SpecularColor);
}
vec3 getpointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 ambientColor, vec3 diffuseColor, vec3 SpecularColor){
	vec3 lightDir = normalize(light.position - FragPos);
	return (getAmbient(light.ambient,ambientColor) + getDiffuse(light.diffuse,lightDir,normal,diffuseColor) + getSpecular(light.specular,lightDir,normal,viewDir,SpecularColor)) 
	* getAttenuation(light.position,light.constant,light.linear,light.quadratic);
}
vec3 getSpotLight(SpotLight light, vec3 normal,vec3 viewDir, vec3 ambientColor, vec3 diffuseColor, vec3 SpecularColor){
	vec3 lightDir = normalize(light.position - FragPos);
	return (getAmbient(light.ambient,ambientColor) + getDiffuse(light.diffuse,lightDir,normal,ambientColor) + getSpecular(light.specular,lightDir,normal,viewDir,SpecularColor)) 
	* getAttenuation(light.position,light.constant,light.linear,light.quadratic)
	* getIntensity(lightDir,light.direction,light.cutoff,light.outerCutOff);
}

vec4 getLights(vec3 norm,vec3 viewDir,vec3 ambientColor, vec3 diffuseColor, vec3 SpecularColor){
	vec4 color;
	for(int i = 0; i < 1; i++)
		color += vec4(getDirLight(dirlights[i],norm,viewDir,ambientColor, diffuseColor, SpecularColor),0.0f);
	for(int i = 0; i < 2; i++)
		color += vec4(getpointLight(pointlights[i],norm,viewDir,ambientColor, diffuseColor, SpecularColor),0.0f);
	for(int i = 0; i < 1; i++)
		color += vec4(getSpotLight(spotlights[i],norm,viewDir,ambientColor, diffuseColor, SpecularColor),0.0f);
	return color;
}