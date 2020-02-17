
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
    float life2;
    vec3 position2;
    int visible;
    vec3 velocity2;
    float p2;
    vec4 p3;
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


struct d{
	uint x;
	uint y;
	uint z;
	uint protoID_scale;

	// uint emitterID;
	uint key_life;

};


uint getDX1(inout d item){
    return getHighBits(item.x);
}
void setDX1(inout d item, uint x1){
    setHighBits(item.x, x1);
}
uint getDY1(inout d item){ // 0xffff0000
    return getHighBits(item.y);
}
void setDY1(inout d item, uint y1){
    setHighBits(item.y, y1);
}
void setDZ1(inout d item, float z){
    item.z = (item.z & 0x0000ffff) | (floatBitsToUint(z) & 0xffff0000);
}
float getDZ1(inout d item){
    return uintBitsToFloat(item.z & 0xffff0000);
}

////////////////////////////////////////////////////
uint getDX2(inout d item){ // 0x0000ffff
    return getLowBits(item.x);
}
void setDX2(inout d item, uint x2){
    setLowBits(item.x, x2);
}
uint getDY2(inout d item){ // 0x0000ffff
    return getLowBits(item.y);
}
void setDY2(inout d item, uint y2){
    setLowBits(item.y,y2);
}
void setDZ2(inout d item, float z){
    item.z = (item.z & 0xffff0000) | (floatBitsToUint(z) >> 16);
}
float getDZ2(inout d item){
    return uintBitsToFloat((item.z & 0x0000ffff) << 16);
}

uint protoID(inout d item){
    return item.protoID_scale >> 16;
}
void protoID(inout d item, uint id){
    item.protoID_scale = (item.protoID_scale & 0x0000ffff) | (id << 16);
}
float scale(inout d item){
    return uintBitsToFloat((item.protoID_scale & 0x0000ffff) << 16);
}
void scale(inout d item, float sc){
    item.protoID_scale = (item.protoID_scale & 0xffff0000) | (floatBitsToUint(sc) >> 16);
}
// void emitterID(inout d item, uint emitter_id){
//     item.emitterID = emitter_id;
// }
// uint emitterID(inout d item){
//     return item.emitterID;
// }

uint key(inout d item){
    return item.key_life >> 16;
}
void key(inout d item, uint id){
    item.key_life = (item.key_life & 0x0000ffff) | (id << 16);
}
float life(inout d item){
    return uintBitsToFloat((item.key_life & 0x0000ffff) << 16);
}
void life(inout d item, float l){
    item.key_life = (item.key_life & 0xffff0000) | (floatBitsToUint(l) >> 16);
}
