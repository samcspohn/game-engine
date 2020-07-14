#version 430 core
#extension GL_ARB_conservative_depth : enable

out vec4 FragColor;
  
// in vec2 TexCoords;

struct pointLight{
    vec3 color;
    float constant;
    float linear;
    float quadratic;
    int transformId;
    float radius;
};

uniform float FC;

// uniform vec3 lightColor;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gDepth;
uniform vec2 WindowSize;

uniform vec3 viewPos;
in float logz;
flat in vec3 lightPos;
flat in pointLight pl;


float getFloat(vec4 v){
    return uintBitsToFloat( floatBitsToUint(v.r) << 12
    | floatBitsToUint(v.g) << 8
    | floatBitsToUint(v.b) << 4
    | floatBitsToUint(v.a)
    );

}

float length2(vec3 v){
    return dot(v,v);
}

void main()
{
    vec2 TexCoords = gl_FragCoord.xy / WindowSize;
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 v = FragPos - lightPos;
    if(abs(v.x) > pl.radius 
    || abs(v.y) > pl.radius 
    || abs(v.z) > pl.radius 
    || length2(FragPos - lightPos) > pl.radius * pl.radius)
        discard;


    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    // then calculate lighting as usual
    vec3 lighting = Diffuse * 0.0; // hard-coded ambient component
    vec3 viewDir = normalize(viewPos - FragPos);

    // diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * pl.color;
    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
    vec3 specular = pl.color * spec * Specular;
    // attenuation
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (pl.constant + pl.linear * distance + pl.quadratic * distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;
    lighting += diffuse + specular;       
    
    
    FragColor = vec4(lighting, 1.0);// + vec4(0.03,0,0,0);
    // if(d * 0.5 > depth)
    // FragColor = FragColor + vec4(0.03,0,0,0) * int(d > depth);
    // FragColor = vec4(1,0,0,1);

}  