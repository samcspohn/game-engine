#include <glm/glm.hpp>
#include "serialize.h"
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
    bool fits(const AABB &a)
    {
        return (this->c.x - this->r.x < a.c.x - a.r.x && this->c.x + this->r.x > a.c.x + a.r.x) && (this->c.y - this->r.y < a.c.y - a.r.y && this->c.y + this->r.y > a.c.y + a.r.y) && (this->c.z - this->r.z < a.c.z - a.r.z && this->c.z + this->r.z > a.c.z + a.r.z);
    }
};

struct AABB2
{
    glm::vec3 min;
    glm::vec3 max;
    SER_HELPER()
    {
        ar &min &max;
    }
    AABB2(glm::vec3 _min, glm::vec3 _max) : min(_min), max(_max){};
    AABB2(){};
    AABB2(float x)
    {
        max = vec3(x);
        min = -max;
    }
    bool fits(const AABB2 &a) const
    {
        return min.x < a.min.x && max.x > a.max.x && min.y < a.min.y && max.y > a.max.y && min.z < a.min.z && max.z > a.max.z;
    }

    bool sits(const vec3 &v) const
    {
        return min.x < v.x && max.x > v.x || min.y < v.y && max.y > v.y || min.z < v.z && max.z > v.z;
    }
    vec3 getCenter() const
    {
        return (this->min + this->max) / 2.f;
    }
    void setPosition(vec3 t)
    {
        vec3 curPos = this->getCenter();
        vec3 translation = t - curPos;
        this->min += translation;
        this->max += translation;
    }
};
struct OBB
{
    glm::vec3 c; // OBB center point
    // glm::vec3 u[3]; // Local x-, y-, and z-axes
    glm::quat u;
    glm::vec3 e; // Positive halfwidth extents of OBB along each axis
    SER_HELPER()
    {
        ar &c &u &e;
    }
};

struct mesh
{
    vector<glm::vec3> *points;
    vector<uint> *tris;
    SER_HELPER()
    {
        ar &points &tris;
    }
};

class rigidBody;
class collider;

// struct vertex;
struct point
{
    glm::vec3 pos1;
    glm::vec3 pos2;
    SER_HELPER()
    {
        ar &pos1 &pos2;
    }
};

// struct colDat
// {
//     collider *c;
//     bool collider_shape_updated;
//     int type;
//     union
//     {
//         OBB o;
//         mesh m;
//         point p;
//     };
//     bool valid;
//     rigidBody *rb;
//     colDat(){};
//     colDat(collider *_c, AABB2 _a) : c(_c){}
//     void update();
// };

struct octDat
{
    AABB2 a;
    collider *d;
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
    if (a.max.x < b.min.x || a.min.x > b.max.x)
        return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y)
        return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z)
        return false;
    return true;
}
bool testOBBHeightMap(OBB &a, vector<float> &h)
{
    return a.c.y < 0.f;
}

#define EPSILON 0.00001
int TestOBBOBB(OBB &a, OBB &b)
{
    float ra, rb;
    glm::mat3 R, AbsR;

    // Compute rotation matrix expressing b in a's coordinate frame
    // for (int i = 0; i < 3; i++)
    //     for (int j = 0; j < 3; j++)
    //         R[i][j] = glm::dot(a.u[i], b.u[j]);
    R = glm::toMat3(inverse(a.u) * b.u);

    // Compute translation vector t
    glm::vec3 t = b.c - a.c;
    // Bring translation into a's coordinate frame
    t = glm::vec3(glm::dot(t, a.u * glm::vec3(1, 0, 0)), glm::dot(t, a.u * glm::vec3(0, 1, 0)), glm::dot(t, a.u * vec3(0, 0, 1)));

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            AbsR[i][j] = glm::abs(R[i][j]) + EPSILON;

    // Test axes L = A0, L = A1, L = A2
    for (int i = 0; i < 3; i++)
    {
        ra = a.e[i];
        rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
        if (glm::abs(t[i]) > ra + rb)
            return 0;
    }

    // Test axes L = B0, L = B1, L = B2
    for (int i = 0; i < 3; i++)
    {
        ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
        rb = b.e[i];
        if (glm::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb)
            return 0;
    }

    // Test axis L = A0 x B0
    ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
    rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
    if (glm::abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb)
        return 0;

    // Test axis L = A0 x B1
    ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
    rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
    if (glm::abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb)
        return 0;

    // Test axis L = A0 x B2
    ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
    rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
    if (glm::abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb)
        return 0;

    // Test axis L = A1 x B0
    ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
    rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
    if (glm::abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb)
        return 0;

    // Test axis L = A1 x B1
    ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
    rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
    if (glm::abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb)
        return 0;

    // Test axis L = A1 x B2
    ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
    rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
    if (glm::abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb)
        return 0;

    // Test axis L = A2 x B0
    ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
    rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
    if (glm::abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb)
        return 0;

    // Test axis L = A2 x B1
    ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
    rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
    if (glm::abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb)
        return 0;

    // Test axis L = A2 x B2
    ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
    rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
    if (glm::abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb)
        return 0;

    // Since no separating axis found, the OBBs must be intersecting
    return 1;
}

// int TestAABBOBB(AABB2& a, OBB& b){
//      float ra, rb;
//     glm::mat3 R, AbsR;

//     // Compute rotation matrix expressing b in a's coordinate frame
//     for (int i = 0; i < 3; i++)
//         for (int j = 0; j < 3; j++)
//             R[i][j] = glm::dot(a.u[i], b.u[j]);

//     // Compute translation vector t
//     glm::vec3 t = b.c - a.c;
//     // Bring translation into a's coordinate frame
//     t = glm::vec3(glm::dot(t, a.u[0]), glm::dot(t, a.u[1]), glm::dot(t, a.u[2]));

//     // Compute common subexpressions. Add in an epsilon term to
//     // counteract arithmetic errors when two edges are parallel and
//     // their cross product is (near) null (see text for details)
//     for (int i = 0; i < 3; i++)
//         for (int j = 0; j < 3; j++)
//             AbsR[i][j] = glm::abs(R[i][j]) + EPSILON;

//     // Test axes L = A0, L = A1, L = A2
//     for (int i = 0; i < 3; i++) {
//         ra = a.e[i];
//         rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
//         if (glm::abs(t[i]) > ra + rb) return 0;
//     }

//     // Test axes L = B0, L = B1, L = B2
//     for (int i = 0; i < 3; i++) {
//         ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
//         rb = b.e[i];
//         if (glm::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return 0;
//     }

//     // Test axis L = A0 x B0
//     ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
//     rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
//     if (glm::abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return 0;

//     // Test axis L = A0 x B1
//     ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
//     rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
//     if (glm::abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return 0;

//     // Test axis L = A0 x B2
//     ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
//     rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
//     if (glm::abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return 0;

//     // Test axis L = A1 x B0
//     ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
//     rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
//     if (glm::abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return 0;

//     // Test axis L = A1 x B1
//     ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
//     rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
//     if (glm::abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return 0;

//     // Test axis L = A1 x B2
//     ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
//     rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
//     if (glm::abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return 0;

//     // Test axis L = A2 x B0
//     ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
//     rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
//     if (glm::abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return 0;

//     // Test axis L = A2 x B1
//     ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
//     rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
//     if (glm::abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return 0;

//     // Test axis L = A2 x B2
//     ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
//     rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
//     if (glm::abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return 0;

//     // Since no separating axis found, the OBBs must be intersecting
//     return 1;
// }

float ScalarTriple(glm::vec3 &a, glm::vec3 &b, glm::vec3 &c)
{
    return glm::dot(glm::cross(a, b), c);
}

float testPlane(glm::vec3 p, glm::vec4 plane)
{
    return plane.x * p.x + plane.y * p.y + plane.z * p.z + plane.w;
}
int IntersectLineTriangle(glm::vec3 p, glm::vec3 q, glm::vec3 a, glm::vec3 b, glm::vec3 c,
                          float &u, float &v, float &w)
{
    glm::vec3 pq = q - p;
    glm::vec3 pa = a - p;
    glm::vec3 pb = b - p;
    glm::vec3 pc = c - p;
    // Test if pq is inside the edges bc, ca and ab. Done by testing
    // that the signed tetrahedral volumes, computed using scalar triple
    glm::vec3 m = glm::cross(pq, pc);
    // products, are all positive

    // u = ScalarTriple(pq, pc, pb);
    u = glm::dot(pq, glm::cross(c, b)) + glm::dot(m, c - b);
    if (u < 0.0f)
        return 0;
    // v = ScalarTriple(pq, pa, pc);
    v = glm::dot(pq, glm::cross(a, c)) + glm::dot(m, a - c);
    if (v < 0.0f)
        return 0;
    // w = ScalarTriple(pq, pb, pa);
    w = glm::dot(pq, glm::cross(b, a)) + glm::dot(m, b - a);
    if (w < 0.0f)
        return 0;

    // Compute the barycentric coordinates (u, v, w) determining the
    // intersection point r, r = u*a + v*b + w*c
    float denom = 1.0f / (u + v + w);
    u *= denom;
    v *= denom;
    w *= denom; // w = 1.0f - u - v;
    glm::vec3 r;
    r = u * a + v * b + w * c;
    u = r.x;
    v = r.y;
    w = r.z;
    return 1;
}

int IntersectSegmentTriangle(glm::vec3 p, glm::vec3 q, glm::vec3 a, glm::vec3 b, glm::vec3 c,
                             float &u, float &v, float &w, float &t)
{

    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 qp = p - q;

    // Compute triangle normal. Can be precalculated or cached if
    // intersecting multiple segments against the same triangle
    glm::vec3 n = glm::cross(ab, ac);

    // Compute denominator d. If d <= 0, segment is parallel to or points
    // away from triangle, so exit early
    float d = glm::dot(qp, n);
    if (d <= 0.0f)
        return 0;

    // Compute intersection t value of pq with plane of triangle. A ray
    // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
    // dividing by d until intersection has been found to pierce triangle
    glm::vec3 ap = p - a;
    t = glm::dot(ap, n);
    if (t < 0.0f)
        return 0;
    if (t > d)
        return 0; // For segment; exclude this code line for a ray test

    // Compute barycentric coordinate components and test if within bounds
    glm::vec3 e = glm::cross(qp, ap);
    v = glm::dot(ac, e);
    if (v < 0.0f || v > d)
        return 0;
    w = -glm::dot(ab, e);
    if (w < 0.0f || v + w > d)
        return 0;

    // Segment/ray intersects triangle. Perform delayed division and
    // compute the last barycentric coordinate component
    float ood = 1.0f / d;
    t *= ood;
    v *= ood;
    w *= ood;
    u = 1.0f - v - w;

    glm::vec3 r;
    r = u * a + v * b + w * c;
    u = r.x;
    v = r.y;
    w = r.z;

    return 1;
}

int IntersectTriangleTriangle(vector<glm::vec3> &t1, vector<glm::vec3> &t2, glm::vec3 &result)
{
    vec3 p1;
    vec3 p2;
    vec3 p3;

    vec3 p4;
    vec3 p5;
    vec3 p6;
    float t;
    bool intersected1 = IntersectSegmentTriangle(t1[0], t1[1], t2[0], t2[1], t2[2], p1.x, p1.y, p1.z, t);
    bool intersected2 = IntersectSegmentTriangle(t1[1], t1[2], t2[0], t2[1], t2[2], p2.x, p2.y, p2.z, t);
    bool intersected3 = IntersectSegmentTriangle(t1[2], t1[0], t2[0], t2[1], t2[2], p3.x, p3.y, p3.z, t);

    bool intersected4 = IntersectSegmentTriangle(t2[0], t2[1], t1[0], t1[1], t1[2], p4.x, p4.y, p4.z, t);
    bool intersected5 = IntersectSegmentTriangle(t2[1], t2[2], t1[0], t1[1], t1[2], p5.x, p5.y, p5.z, t);
    bool intersected6 = IntersectSegmentTriangle(t2[2], t2[0], t1[0], t1[1], t1[2], p6.x, p6.y, p6.z, t);
    // bool intersected1 = RayIntersectsTriangle(t1[0],t1[1]-t1[0],t2,p1);
    // bool intersected2 = RayIntersectsTriangle(t1[1],t1[2]-t1[1],t2,p2);
    // bool intersected3 = RayIntersectsTriangle(t1[2],t1[0]-t1[2],t2,p3);
    result = glm::vec3(0);
    int intersections = 0;
    if (intersected1 || intersected2 || intersected3 || intersected4 || intersected5 || intersected6)
    {
        if (intersected1)
        {
            result += p1;
            intersections++;
        }
        if (intersected2)
        {
            result += p2;
            intersections++;
        }
        if (intersected3)
        {
            result += p3;
            intersections++;
        }
        if (intersected4)
        {
            result += p4;
            intersections++;
        }
        if (intersected5)
        {
            result += p5;
            intersections++;
        }
        if (intersected6)
        {
            result += p6;
            intersections++;
        }
        result /= intersections;
        return 1;
    }
    return 0;
}

bool testMeshMesh(mesh &m1, // longer tris
                  const mat4 &trans1,
                  mesh &m2, // shorter tris
                  const mat4 &trans2, glm::vec3 &result)
{
    vector<glm::vec3> tri1(3);
    vector<glm::vec3> tri2(3);
    for (int i = 0; i < m1.tris->size(); i += 3)
    {
        tri1[0] = trans1 * vec4((*m1.points)[(*m1.tris)[i]], 1);
        tri1[1] = trans1 * vec4((*m1.points)[(*m1.tris)[i + 1]], 1);
        tri1[2] = trans1 * vec4((*m1.points)[(*m1.tris)[i + 2]], 1);
        for (int j = 0; j < m2.tris->size(); j += 3)
        {
            tri2[0] = trans2 * vec4((*m2.points)[(*m2.tris)[j]], 1);
            tri2[1] = trans2 * vec4((*m2.points)[(*m2.tris)[j + 1]], 1);
            tri2[2] = trans2 * vec4((*m2.points)[(*m2.tris)[j + 2]], 1);
            if (IntersectTriangleTriangle(tri1, tri2, result))
            {
                return true;
            }
        }
    }
    return false;
}

using namespace glm;
vector<glm::vec3> boxPoints{
    vec3(1.f, -1.f, -1.f),
    vec3(1.f, -1.f, 1.f),
    vec3(-1.f, -1.f, 1.f),
    vec3(-1.f, -1.f, -1.f),
    vec3(1.f, 1.f, -1.f),
    vec3(1.f, 1.f, 1.f),
    vec3(-1.f, 1.f, 1.f),
    vec3(-1.f, 1.f, -1.f)};
vector<uint> boxTris{
    0, 1, 2,
    0, 2, 3,
    4, 7, 6,
    4, 7, 5,
    0, 4, 5,
    0, 5, 1,
    1, 5, 6,
    1, 6, 2,
    2, 6, 7,
    2, 7, 3,
    4, 0, 3,
    4, 3, 7};

bool testOBBMesh(OBB &o, const mat4 o_trans, mesh &m, const mat4 m_trans, glm::vec3 &result)
{
    mesh o_m;
    o_m.points = &boxPoints;
    o_m.tris = &boxTris;
    return testMeshMesh(m, m_trans, o_m, o_trans, result);
}

bool testPointMesh(point &p, mesh &m, glm::vec3 mPos, glm::vec3 mScl, glm::quat mRot, glm::vec3 &result)
{
    // glm::vec3 pos1 = (1.f / mScl) * glm::inverse(mRot) * (p.pos1 - mPos);
    // glm::vec3 pos2 = (1.f / mScl) * glm::inverse(mRot) * (p.pos2 - mPos);
    mat4 trans = glm::inverse(glm::translate(mPos) * glm::toMat4(mRot) * glm::scale(mScl));
    glm::vec3 pos1 = trans * vec4(p.pos1, 1);
    glm::vec3 pos2 = trans * vec4(p.pos2, 1);
    glm::vec3 p1;
    vector<glm::vec3> tri1(3);
    vector<glm::vec3> tri2(3);
    float t;
    for (int i = 0; i < m.tris->size(); i += 3)
    {
        // tri1[0] = trans1 * vec4((*m1.points)[ (*m1.tris)[i]     ],1);
        // tri1[1] = trans1 * vec4((*m1.points)[ (*m1.tris)[i + 1] ],1);
        // tri1[2] = trans1 * vec4((*m1.points)[ (*m1.tris)[i + 2] ],1);
        tri1[0] = (*m.points)[(*m.tris)[i]];
        tri1[1] = (*m.points)[(*m.tris)[i + 1]];
        tri1[2] = (*m.points)[(*m.tris)[i + 2]];
        if (IntersectSegmentTriangle(pos2, pos1, tri1[0], tri1[1], tri1[2], p1.x, p1.y, p1.z, t))
        {
            // result = mPos;
            result = inverse(trans) * vec4(p1, 1);
            return true;
        }
    }
    return false;
}

bool testPointOBB(point &p, OBB &o, glm::vec3 &result)
{
    glm::vec3 pos = glm::inverse(o.u) * (p.pos1 - o.c);

    mesh o_m;
    o_m.points = &boxPoints;
    o_m.tris = &boxTris;
    return testPointMesh(p, o_m, o.c, o.e, o.u, result);
    // if(abs(pos.x) < o.e.x &&
    //  abs(pos.y) < o.e.y &&
    //  abs(pos.z) < o.e.z){
    //      return true;
    //  }
    //  return false;
}