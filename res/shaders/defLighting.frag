#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gAlbedoSpec;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDepth;
uniform vec3 viewPos;

// layout(std430, binding = 0) buffer l{ vec4 lights[];};

void main()
{
    // // // retrieve data from gbuffer
    // vec3 Normal = texture(gNormal, TexCoords).rgb;
    // vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    // float Specular = texture(gAlbedoSpec, TexCoords).a;
    // vec3 FragPos = texture(gPosition, TexCoords).rgb;
    
    // FragColor = vec4(Diffuse * 0.1, 1.0);// + vec4(FragPos,0);
    // // FragColor = vec4(1,0,1,1);
    // retrieve data from gbuffer
    vec3 FragPos=texture(gPosition,TexCoords).rgb;
    if(FragPos==vec3(0)){
        discard;
        // lighting = vec3(0.5,0.6,1.0);
    }
    vec3 Normal=texture(gNormal,TexCoords).rgb;
    vec3 Diffuse=texture(gAlbedoSpec,TexCoords).rgb;
    float Specular=texture(gAlbedoSpec,TexCoords).a;
    float depth = texture(gDepth,TexCoords).r;
    
    // then calculate lighting as usual
    vec3 lighting=Diffuse*.1;// hard-coded ambient component
    vec3 viewDir=normalize(viewPos-FragPos);
    float Linear=1.f;
    float Quadratic=1.f;
    vec3 Color=vec3(1,1,1);
    vec3 light = normalize(vec3(1));
    // for(int i = 0; i < 100; ++i)
    // {
        // diffuse
        vec3 lightDir = light;//normalize(light.xyz - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * Color;
        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
        vec3 specular = Color * spec * Specular;
        // attenuation
        float distance = 1;//length(light.xyz - FragPos);
        float attenuation = 1;//1.0 / (1.0 + Linear * distance + Quadratic * distance * distance);
        diffuse *= attenuation;
        specular *= attenuation;
        lighting = diffuse;// + specular;
    // }
    // if(FragPos==vec3(0)){
    //     discard;
    //     // lighting = vec3(0.5,0.6,1.0);
    // }
    FragColor=vec4(lighting,1.f);
    // FragColor=vec4(depth,depth,depth,1.f);
    
}
