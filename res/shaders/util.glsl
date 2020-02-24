
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
	mat4 r = r1 * r2;
	r[0][3] = r[1][3] = r[2][3] = r[3][0] = r[3][1] = r[3][2] = 0;
	r[3][3] = 1;
	return m * r;
	// #define qw q.w
	// #define qx q.x
	// #define qy q.y
	// #define qz q.z
	// const float qx2 = qx * qx;
	// const float qy2 = qy * qy;
	// const float qz2 = qz * qz;
	// // mat4 r = {{a,-b,-c,-d},
	// // 		{b,a,-d,c},
	// // 		{c,d,a,-b},
	// // 		{d,-c,b,a}};
	// mat4 r = {{1 - 2*qy2 - 2*qz2, 2*qx*qy - 2*qz*qw, 2*qx*qz + 2*qy*qw, 0},
	// 	{2*qx*qy + 2*qz*qw, 1 - 2*qx2 - 2*qz2, 2*qy*qz - 2*qx*qw, 0},
	// 	{2*qx*qz - 2*qy*qw, 2*qy*qz + 2*qx*qw, 1 - 2*qx2 - 2*qy2, 0},
	// 	{0,0,0,1}};
	// #undef qw
	// #undef qx
	// #undef qy
	// #undef qz
	// return m * r;
	
}

mat4 identity(){
	mat4 i = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
	return i;
}
vec4 lookAt(in vec3 lookAt, in vec3 up) {
vec3 forward = lookAt;
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
// 	vec3 f = (normalize(lookAt));
// 	vec3 s = (normalize(cross(up, f)));
// 	vec3 u = (cross(f, s));

// 	mat4 Result = mat4(1);
// 	Result[0][0] = s.x;
// 	Result[1][0] = s.y;
// 	Result[2][0] = s.z;
// 	Result[0][1] = u.x;
// 	Result[1][1] = u.y;
// 	Result[2][1] = u.z;
// 	Result[0][2] = f.x;
// 	Result[1][2] = f.y;
// 	Result[2][2] = f.z;
// 	Result[3][0] = -dot(s, vec3(0));
// 	Result[3][1] = -dot(u, vec3(0));
// 	Result[3][2] = -dot(f, vec3(0));
// 	mat4 m = Result;

// #define m00 Result[0][0]
// #define m01 Result[0][1]
// #define m02 Result[0][2]
// #define m10 Result[1][0]
// #define m11 Result[1][1]
// #define m12 Result[1][2]
// #define m20 Result[2][0]
// #define m21 Result[2][1]
// #define m22 Result[2][2]

// vec4 q;

// float tr = m00 + m11 + m22;
// if (tr > 0) { 
//   float S = sqrt(tr+1.0) * 2; // S=4*qw 
//   q.w = 0.25 * S;
//   q.x = (m21 - m12) / S;
//   q.y = (m02 - m20) / S; 
//   q.z = (m10 - m01) / S; 
// } else if (bool(int(m00 > m11)&int(m00 > m22))) { 
//   float S = sqrt(1.0 + m00 - m11 - m22) * 2; // S=4*qx 
//   q.w = (m21 - m12) / S;
//   q.x = 0.25 * S;
//   q.y = (m01 + m10) / S; 
//   q.z = (m02 + m20) / S; 
// } else if (m11 > m22) { 
//   float S = sqrt(1.0 + m11 - m00 - m22) * 2; // S=4*qy
//   q.w = (m02 - m20) / S;
//   q.x = (m01 + m10) / S; 
//   q.y = 0.25 * S;
//   q.z = (m12 + m21) / S; 
// } else { 
//   float S = sqrt(1.0 + m22 - m00 - m11) * 2; // S=4*qz
//   q.w = (m10 - m01) / S;
//   q.x = (m02 + m20) / S;
//   q.y = (m12 + m21) / S;
//   q.z = 0.25 * S;
// }

// #undef m00
// #undef m01 
// #undef m02 
// #undef m10
// #undef m11 
// #undef m12
// #undef m20 
// #undef m21 
// #undef m22 

// 	return q;
}

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

vec3 rotate(vec3 axis, float angle, vec3 vec){
	if(angle == 0){
		return vec;
	}
	return (rotationMatrix(axis, angle) * vec4(vec,1)).xyz;
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