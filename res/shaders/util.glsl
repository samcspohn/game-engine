
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


