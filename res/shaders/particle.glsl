
struct particle{
    vec3 position;
    uint emitter;
    vec3 scale;
    uint emitter_prototype;
    vec4 rotation;

    vec3 velocity;
    int live;

    float life;
    int next;
    int prev;
    float p;
};
struct emitter_prototype{
    float emission_rate;
    float lifetime;
    float rotation_rate;
    int id;

    vec4 color;

    vec3 velocity;
    int live;

    vec3 scale;
    int billboard;

    vec3 p;
    int trail;
};
struct emitter{
    uint transform;
    uint emitter_prototype;
    float emission;
    int live;

    vec3 p;
    int frame;
};

struct emitterInit{
    uint transformID;
    uint emitterProtoID;
    int live;
    int id;
}; 