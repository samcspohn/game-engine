
struct particle
{
    vec3 position;
    int p1;
    // uint emitter;

    vec2 scale;
    uint emitter_prototype;
    float life;

    smquat rotation; // 2ints
    smvec3 velocity; // 2ints
    // vec3 position2;
    int next;
    
    // smvec3 velocity2; // 2ints
    float p2;    
    float l;
    int p3;
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
    
    vec2 texCoord;
    vec2 sz;
    

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
    
    vec3 prevPos;
    int p2;
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

smvec3 getPos(d item){
    smvec3 sv;
    sv.xy = item.xy;
    sv.z = uintBitsToFloat(~item.z ^ (1 << 31));
    return sv;
}

void makeParticle(inout particle p, inout emitter_prototype ep, inout rng generator, inout float particle_count, vec3 tpos, vec4 trot, vec3 tscale, uint epId, float dt, uint eId){

    p.position = tpos + normalize(randVec3(generator)) * tscale * ep.radius;
    // p.rotation  = transforms[e.transform].rotation;
    p.scale = tscale.xy * ep.scale;
    p.life = 1.f;
    p.l = max(ep.lifetime2 + gen(generator) * abs(ep.lifetime - ep.lifetime2),0.0001f);
    // p.emitter = eId;
    p.emitter_prototype = epId;
    vec3 random = rotate(vec3(1,0,0),gen(generator) * ep.dispersion,vec3(0,0,ep.minSpeed + gen(generator) * (ep.maxSpeed - ep.minSpeed)));
    random = rotate(vec3(0,0,1),gen(generator) * 2 * M_PI,random);
    vec3 vel = vec3(rotate(identity(), trot) * vec4(random * tscale,1));
    set(p.velocity,vel);
    
    if(ep.velAlign == 1){
        vec3 dir = cross(vel,randVec3(generator));
        set(p.rotation,lookAt(dir,vel));
    }else{
        set(p.rotation,normalize(randVec4(generator)));
    }

    p.position += vel * dt * particle_count;
    // p.position2 = p.position;
    // p.velocity2 = p.velocity;
    p.life -= dt * particle_count++ / p.l;
}
