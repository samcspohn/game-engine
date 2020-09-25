#include <glm/glm.hpp>
namespace physics
{

struct sphere
{
	float radius;
};
struct box
{
	box operator=(glm::vec3 v)
	{
		this->dimensions = v;
		return *this;
	}
	glm::vec3 dimensions;
};
struct plane
{
	glm::vec4 _plane;
};
} // namespace physics

struct AABB
{
	glm::vec3 c;
	glm::vec3 r;
	AABB(glm::vec3 _c, glm::vec3 _r) : c(_c), r(_r){};
	AABB(){};
    bool fits(const AABB& a){
        return (this->c.x - this->r.x < a.c.x - a.r.x && this->c.x + this->r.x > a.c.x + a.r.x)
        && (this->c.y - this->r.y < a.c.y - a.r.y && this->c.y + this->r.y > a.c.y + a.r.y)
        && (this->c.z - this->r.z < a.c.z - a.r.z && this->c.z + this->r.z > a.c.z + a.r.z);
    }
};

struct AABB2
{
	glm::vec3 min;
	glm::vec3 max;
	AABB2(glm::vec3 _min, glm::vec3 _max) : min(_min), max(_max){};
	AABB2(){};
    AABB2(float x){
        max = vec3(x);
        min = -max;
    }
    bool fits(const AABB2& a) const {
        return min.x < a.min.x && max.x > a.max.x 
        && min.y < a.min.y && max.y > a.max.y
        && min.z < a.min.z && max.z > a.max.z;
    }

    bool sits(const vec3& v) const {
        return min.x < v.x && max.x > v.x
        || min.y < v.y && max.y > v.y
        || min.z < v.z && max.z > v.z;
    }
    vec3 getCenter() const {
        return (this->min + this->max) / 2.f;
    }
    void setPosition(vec3 t){
        vec3 curPos = this->getCenter();
        vec3 translation = t - curPos;
        this->min += translation; this->max += translation;
    }
};
struct OBB {
    glm::vec3 c;     // OBB center point
    glm::vec3 u[3]; // Local x-, y-, and z-axes
    glm::vec3 e;    // Positive halfwidth extents of OBB along each axis
};
bool _testAABB(const AABB &a, const AABB &b)
{
	bool x = abs(a.c[0] - b.c[0]) < (a.r[0] + b.r[0]);
	bool y = abs(a.c[1] - b.c[1]) < (a.r[1] + b.r[1]);
	bool z = abs(a.c[2] - b.c[2]) < (a.r[2] + b.r[2]);

	return x && y && z;
}

bool testAABB(const AABB &a, const AABB &b)
{
	if (abs(a.c[0] - b.c[0]) >= (a.r[0] + b.r[0]))
		return false;
	if (abs(a.c[1] - b.c[1]) >= (a.r[1] + b.r[1]))
		return false;
	if (abs(a.c[2] - b.c[2]) >= (a.r[2] + b.r[2]))
		return false;

	return true;
}
bool testAABB(const AABB2 &a, const AABB2 &b)
{
    if(a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if(a.max.y < b.min.y || a.min.y > b.max.y) return false;
    if(a.max.z < b.min.z || a.min.z > b.max.z) return false;
    return true;
}
bool testOBBHeightMap(OBB& a, vector<float>& h){
    return a.c.y < 0.f;
}

#define EPSILON 0.00001
int TestOBBOBB(OBB &a, OBB &b)
{
    float ra, rb;
    glm::mat3 R, AbsR;

    // Compute rotation matrix expressing b in a's coordinate frame
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            R[i][j] = glm::dot(a.u[i], b.u[j]);

    // Compute translation vector t
    glm::vec3 t = b.c - a.c;
    // Bring translation into a's coordinate frame
    t = glm::vec3(glm::dot(t, a.u[0]), glm::dot(t, a.u[1]), glm::dot(t, a.u[2]));

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            AbsR[i][j] = glm::abs(R[i][j]) + EPSILON;

    // Test axes L = A0, L = A1, L = A2
    for (int i = 0; i < 3; i++) {
        ra = a.e[i];
        rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
        if (glm::abs(t[i]) > ra + rb) return 0;
    }

    // Test axes L = B0, L = B1, L = B2
    for (int i = 0; i < 3; i++) {
        ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
        rb = b.e[i];
        if (glm::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return 0;
    }

    // Test axis L = A0 x B0
    ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
    rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
    if (glm::abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return 0;

    // Test axis L = A0 x B1
    ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
    rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
    if (glm::abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return 0;

    // Test axis L = A0 x B2
    ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
    rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
    if (glm::abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return 0;

    // Test axis L = A1 x B0
    ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
    rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
    if (glm::abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return 0;

    // Test axis L = A1 x B1
    ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
    rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
    if (glm::abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return 0;

    // Test axis L = A1 x B2
    ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
    rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
    if (glm::abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return 0;

    // Test axis L = A2 x B0
    ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
    rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
    if (glm::abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return 0;

    // Test axis L = A2 x B1
    ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
    rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
    if (glm::abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return 0;

    // Test axis L = A2 x B2
    ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
    rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
    if (glm::abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return 0;

    // Since no separating axis found, the OBBs must be intersecting
    return 1;
}
