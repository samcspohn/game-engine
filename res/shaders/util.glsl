
mat4 translate(mat4 m, vec3 translation){
		mat4 t = {{1,0,0,translation.x},
		{0,1,0,translation.y,},
		{0,0,1,translation.z}
		,{0,0,0,1}};
		return m * transpose(t);
}
mat4 scale(mat4 m, vec3 scale){
	mat4 s = {{scale.x,0,0,0}, {0,scale.y,0,0},{0,0,scale.z,0},{0,0,0,1}};
	return m * transpose(s);
}

mat4 rotate(mat4 m, vec4 q){
	mat4 r1 = {{q.w,q.z,-q.y,q.x},{-q.z,q.w,q.x,q.y},{q.y,-q.x,q.w,q.z},{-q.x,-q.y,-q.z,q.w}};
	mat4 r2 = {{q.w,q.z,-q.y,-q.x},{-q.z,q.w,q.x,-q.y},{q.y,-q.x,q.w,-q.z},{q.x,q.y,q.z,q.w}};
	return m * r1 * r2;
}

mat4 identity(){
	mat4 i = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
	return i;
}
vec4 lookAt(in vec3 lookAt, in vec3 upDirection) {
vec3 forward = lookAt; vec3 up = upDirection;
forward = normalize(forward);
vec3 right = normalize(cross(up, forward));
up = normalize(cross(forward,right));

#define m00 right.x
#define m01 up.x
#define m02 forward.x
#define m10 right.y
#define m11 up.y
#define m12 forward.y
#define m20 right.z
#define m21 up.z
#define m22 forward.z

vec4 ret;
ret.w = sqrt(1.0f + m00 + m11 + m22) * 0.5f;
float w4_recip = 1.0f / (4.0f * ret.w);
ret.x = (m21 - m12) * w4_recip;
ret.y = (m02 - m20) * w4_recip;
ret.z = (m10 - m01) * w4_recip;

#undef m00
#undef m01
#undef m02
#undef m10
#undef m11
#undef m12
#undef m20
#undef m21
#undef m22

return ret;
}

struct d{
    // vec3 p;
    // uint key;
    // vec3 pos1;
    // uint emitterID;
    // vec3 pos2;
    // uint emitterProtoID;
    // vec3 scale;
    // float life;


	uint x;
	uint y;
	uint z;
	uint protoID_scale;

	// vec2 p;
	uint emitterID;
	uint key_life;

};
const uint RIGHT = 0x0000ffff;
const uint LEFT = 0xffff0000;
    uint getLeft(uint field) {
        return field >> 16; // >>> operator 0-fills from left
    }
	uint getRight(uint field) {
        return field & RIGHT;
    }

	void setHighBits(inout uint o, uint left){
		o = (left << 16) | (o & RIGHT);
	}
	void setLowBits(inout uint o, uint right){
		o = (o & LEFT) | (right);
	}
	uint getHighBits(inout uint o){
		return getLeft(o);
	}
	uint getLowBits(inout uint o){
		return getRight(o);
	}




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
	// vec3 position1(inout d item){
	// 	// return vec3(0,0,0);
	// 	return vec3(uintBitsToFloat(item.x & 0xffff0000), uintBitsToFloat(item.y & 0xffff0000), uintBitsToFloat(item.z & 0xffff0000));
	// }
	// void position1(inout d item, vec3 set){
	// 	item.x = (item.x & 0x0000ffff) | (floatBitsToUint(set.x) & 0xffff0000);
	// 	item.y = (item.y & 0x0000ffff) | (floatBitsToUint(set.y) & 0xffff0000);
	// 	item.z = (item.z & 0x0000ffff) | (floatBitsToUint(set.z) & 0xffff0000);
	// }
	// vec3 position2(inout d item){
	// 	// return vec3(0,100,0);
	// 	return vec3(uintBitsToFloat((item.x & 0x0000ffff) << 16), uintBitsToFloat((item.y & 0x0000ffff) << 16), uintBitsToFloat((item.z & 0x0000ffff) << 16));
	// }
	// void position2(inout d item, vec3 set){
	// 	item.x = (item.x & 0xffff0000) | (floatBitsToUint(set.x) >> 16);
	// 	item.y = (item.y & 0xffff0000) | (floatBitsToUint(set.y) >> 16);
	// 	item.z = (item.z & 0xffff0000) | (floatBitsToUint(set.z) >> 16);
	// }
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
	void emitterID(inout d item, uint emitter_id){
		item.emitterID = emitter_id;
	}
	uint emitterID(inout d item){
		return item.emitterID;
	}

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

	float intToFloat(int toFloat){
		int p1x = toFloat;
		if(toFloat < 0){
			p1x = p1x << 16 | 0x80000000;
		}else{
			p1x = p1x << 16;
		}
		return intBitsToFloat(p1x);
	}
	int floatToInt(float toInt){
		int x = floatBitsToInt(toInt);
		if(x < 0){
			x = ((x >> 16) & 0x00007fff) | 0x80000000;
		}else{
			x = x >> 16;
		}
		return x;
	}