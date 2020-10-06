
struct particle
{
    vec3 position;
    // uint emitter;

    vec2 scale;
    uint emitter_prototype;
    float life;

    smquat rotation; // 2ints
    smvec3 velocity; // 2ints
    
    vec3 position2;
    int next;
    
    smvec3 velocity2; // 2ints
    float p1;    
    float l;
};

// float getLife1(inout particle p){
//     return p.life;
// }
// void setLife1(inout particle p, float l){
//     p.life = l;
// }
struct emitter_prototype{
    float emission_rate;
    float lifetime;
    float rotation_rate;
    float dispersion;

    // vec4 color;

    float minSpeed;
    float maxSpeed;
    float lifetime2;
    int live;

    vec2 scale;
    int billboard;
    int p3;

    int velAlign;
    float radius;
    int p2;
    int trail;
    vec4 colorLife[100];
    float sizeLife[100];
};
struct emitter{
    uint transform;
    uint emitter_prototype;
    float emission;
    int live;

    vec2 p;
    int last;
    int frame;
};
struct emitterInit{
    uint transformID;
    uint emitterProtoID;
    int live;
    int id;
}; 
struct _emission{
    vec3 position;
    uint emitter_prototype;
    vec3 direction;
    int emitterID;
    vec3 scale;
    int last;
};
struct _burst{
    vec3 position;
    uint emitter_prototype;
    vec3 direction;
    uint count;
    vec3 scale;
    int p1;
};

struct d{
    uint xy;
    uint z;
    smquat rot;
	uint scale_xy;
	uint protoID_life;
};

void setScale(inout d item, vec2 scale){
    item.scale_xy = (floatBitsToUint(scale.x) & LEFT) | (floatBitsToUint(scale.y) >> 16);
}
vec2 getScale(inout d item){
    return vec2(uintBitsToFloat(item.scale_xy & LEFT), uintBitsToFloat(getLowBits(item.scale_xy) << 16));
}

uint protoID(inout d item){
    return item.protoID_life >> 16;
}
void protoID(inout d item, uint id){
    item.protoID_life = (item.protoID_life & 0x0000ffff) | (id << 16);
}

float life(inout d item){
    return uintBitsToFloat((item.protoID_life & 0x0000ffff) << 16);
}
void life(inout d item, float l){
    item.protoID_life = (item.protoID_life & 0xffff0000) | (floatBitsToUint(l) >> 16);
}

void setPos(inout d item, smvec3 sv){
    item.xy = sv.xy;
    item.z = ~(floatBitsToUint(sv.z) ^ (1 << 31));
}

smvec3 getPos(inout d item){
    smvec3 sv;
    sv.xy = item.xy;
    sv.z = uintBitsToFloat(~item.z ^ (1 << 31));
    return sv;
}