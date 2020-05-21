
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
    float l;
};
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

    vec3 scale;
    int billboard;

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


struct d{
    // vec4 rot;
	uint xy;
    float z;
    uint qxy;
    uint qzw;
	// uint qwx;
    // uint qyz;
	uint scale_xy;
	uint protoID_life;
	// uint key_life;
};


uint getX(inout d item){
    return getHighBits(item.xy);
}
void setX(inout d item, uint x1){
    setHighBits(item.xy, x1);
}

uint getY(inout d item){ // 0x0000ffff
    return getLowBits(item.xy);
}
void setY(inout d item, uint y2){
    setLowBits(item.xy,y2);
}

void setZ(inout d item, float z){
    item.z = z;
    // item.z1 = (item.z & 0x0000ffff) | (floatBitsToUint(z) & 0xffff0000);
}
float getZ(inout d item){
    return item.z;
    // return uintBitsToFloat(item.z & 0xffff0000);
}
void setScale(inout d item, vec2 scale){
    item.scale_xy = (floatBitsToUint(scale.x) & LEFT) | (floatBitsToUint(scale.y) >> 16);
}
vec2 getScale(inout d item){
    return vec2(uintBitsToFloat(item.scale_xy & LEFT), uintBitsToFloat(getLowBits(item.scale_xy) << 16));
}

void setRotation(inout d item, vec4 quat){
    quat = normalize(quat);
    // setHighBits(item.qwx,uint(quat.w * 32768 + 32768));
    // setLowBits(item.qwx,uint(quat.x * 32768 + 32768));
    // setHighBits(item.qyz,uint(quat.y * 32768 + 32768));
    // setLowBits(item.qyz,uint(quat.z * 32768 + 32768));


    setHighBits(item.qxy,uint(quat.x * 32768 + 32768));
    setLowBits(item.qxy,uint(quat.y * 32768 + 32768));
    setHighBits(item.qzw,uint(quat.z * 32768 + 32768));
    setLowBits(item.qzw,uint(quat.w * 32768 + 32768));
    // item.rot = quat;
}

vec4 getRotation(inout d item){
    // return item.rot;
    return normalize(vec4((float(getHighBits(item.qxy)) - 32768) / 32768,
                (float(getLowBits(item.qxy)) - 32768) / 32768,
                (float(getHighBits(item.qzw)) - 32768) / 32768,
                (float(getLowBits(item.qzw)) - 32768) / 32768));
    // return normalize(vec4((float(getHighBits(item.qwx)) - 32768) / 32768,
    //         (float(getLowBits(item.qwx)) - 32768) / 32768,
    //         (float(getHighBits(item.qyz)) - 32768) / 32768,
    //         (float(getLowBits(item.qyz)) - 32768) / 32768));
}

uint protoID(inout d item){
    return item.protoID_life >> 16;
}
void protoID(inout d item, uint id){
    item.protoID_life = (item.protoID_life & 0x0000ffff) | (id << 16);
}
// uint key(inout d item){
//     return item.key_life >> 16;
// }
// void key(inout d item, uint id){
//     item.key_life = (item.key_life & 0x0000ffff) | (id << 16);
// }
float life(inout d item){
    return uintBitsToFloat((item.protoID_life & 0x0000ffff) << 16);
}
void life(inout d item, float l){
    item.protoID_life = (item.protoID_life & 0xffff0000) | (floatBitsToUint(l) >> 16);
}
