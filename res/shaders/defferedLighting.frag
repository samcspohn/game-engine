#version 430 core
#extension GL_ARB_conservative_depth : enable

out vec4 FragColor;
  
// in vec2 TexCoords;

uniform float FC;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform vec2 WindowSize;

uniform vec3 viewPos;
in float logz;

void main()
{             
    vec2 TexCoords = gl_FragCoord.xy / WindowSize;
    // retrieve data from G-buffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    // then calculate lighting as usual
    vec3 lighting = Diffuse * 0.0; // hard-coded ambient component
    vec3 viewDir = normalize(viewPos - FragPos);

    // diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lightColor;
    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
    vec3 specular = lightColor * spec * Specular;
    // attenuation
    float Linear = 1.f;
    float Quadratic = 1.f;
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + Linear * distance + Quadratic * distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;
    lighting += diffuse + specular;       
    
    
    FragColor = vec4(lighting, 1.0);// + vec4(0.03,0,0,0);
    // FragColor = vec4(1,0,0,1);
	// gl_FragDepth = log2(logz) * 0.5 * FC;

}  