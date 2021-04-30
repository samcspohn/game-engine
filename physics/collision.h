#pragma once
#include <glm/glm.hpp>
#include "serialize.h"
#include <vector>
struct AABB
{
    glm::vec3 c;
    glm::vec3 r;
    AABB(glm::vec3 _c, glm::vec3 _r);
    AABB();
    bool fits(const AABB &a);
};

struct AABB2
{
    glm::vec3 min;
    glm::vec3 max;
    SER_HELPER()
    {
        ar &min &max;
    }
    AABB2(glm::vec3 _min, glm::vec3 _max);
    AABB2();
    AABB2(float x);
    bool fits(const AABB2 &a) const;

    bool sits(const glm::vec3 &v)  const;
    glm::vec3 getCenter() const;
    void setPosition(glm::vec3 t);
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

struct MESH
{
    std::vector<glm::vec3> points;
    std::vector<uint> tris;
    int32_t references;
    SER_HELPER()
    {
        ar &points &tris;
    }
};

struct mesh
{
    MESH *m;
    SER_HELPER()
    {
        ar &m;
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


struct octDat
{
    AABB2 a;
    collider *d;
};

bool _testAABB(const AABB &a, const AABB &b);

bool testAABB(const AABB &a, const AABB &b);
bool testAABB(const AABB2 &a, const AABB2 &b);
bool testOBBHeightMap(OBB &a, std::vector<float> &h);
#define EPSILON 0.00001
int TestOBBOBB(OBB &a, OBB &b);

// Intersect ray R(t) = p + t*d against AABB a. When intersecting,
// return intersection distance tmin and point q of intersection
bool IntersectRayAABB(glm::vec3 p, glm::vec3 d, AABB2 a, float &tmin, glm::vec3 &q);

struct ray
{
    ray(const glm::vec3 &orig, const glm::vec3 &dir);
    glm::vec3 orig, dir; // ray orig and dir
    glm::vec3 invdir;
    int sign[3];
};

struct sphere{
    glm::vec3 c;
    float r;
    sphere(glm::vec3 _c, float _r);
};

bool IntersectRayAABB2(const ray &r, AABB2 a);
bool IntersectRayAABB3(const ray &r, AABB2 a);

// bool IntersectRayAABB4(AABB2 b, Ray r);

// int TestAABBOBB(AABB2& a, OBB& b);
bool _intersectRayTriangle(glm::vec3& p, glm::vec3& d, glm::vec3& p0, glm::vec3& p1, glm::vec3& p2, glm::vec3& q);
float ScalarTriple(glm::vec3 &a, glm::vec3 &b, glm::vec3 &c);
float testPlane(glm::vec3 p, glm::vec4 plane);
int IntersectLineTriangle(glm::vec3 p, glm::vec3 q, glm::vec3 a, glm::vec3 b, glm::vec3 c,
                          float &u, float &v, float &w);

int IntersectSegmentTriangle(glm::vec3 p, glm::vec3 q, glm::vec3 a, glm::vec3 b, glm::vec3 c,
                             float &u, float &v, float &w, float &t);

int IntersectTriangleTriangle(std::vector<glm::vec3> &t1, std::vector<glm::vec3> &t2, glm::vec3 &result);
bool testMeshMesh(mesh &m1, // longer tris
                  const glm::mat4 &trans1,
                  mesh &m2, // shorter tris
                  const glm::mat4 &trans2, glm::vec3 &result);

extern MESH BOX_MESH;

bool testOBBMesh(OBB &o, const glm::mat4 o_trans, mesh &m, const glm::mat4 m_trans, glm::vec3 &result);

bool testPointMesh(point &p, mesh &m, glm::vec3 mPos, glm::vec3 mScl, glm::quat mRot, glm::vec3 &result);
bool testPointOBB(point &p, OBB &o, glm::vec3 &result);