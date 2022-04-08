#pragma once
#include <glm/glm.hpp>
#include "serialize.h"
#include <vector>
#include "_model.h"
#include "components/Component.h"

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
    AABB2(glm::vec3 _min, glm::vec3 _max);
    AABB2();
    AABB2(float x);
    bool fits(const AABB2 &a) const;

    bool sits(const glm::vec3 &v) const;
    glm::vec3 getCenter() const;
    void setPosition(glm::vec3 t);
};
struct OBB
{
    glm::vec3 c; // OBB center point
    // glm::vec3 u[3]; // Local x-, y-, and z-axes
    glm::quat u;
    glm::vec3 e; // Positive halfwidth extents of OBB along each axis

};

struct MESH
{
    std::vector<glm::vec3> points;
    std::vector<uint> tris;
    int32_t references;
};

struct mesh
{
    // _model mod;
    int mod;
    MESH *m;
    // constexpr mesh(const mesh&);
};

class rigidBody;
class collider;

// struct vertex;
struct point
{
    glm::vec3 pos1;
    glm::vec3 pos2;
};
struct Sphere{
    glm::vec3 c;
    float r;
};
struct Plane{
    float d;
    glm::vec3 n;
};

struct octDat
{
    AABB2 a;
    collider *d;
};

bool _testAABB(const AABB &a, const AABB &b, collision& col);

bool testAABB(const AABB &a, const AABB &b);
bool testAABB(const AABB2 &a, const AABB2 &b);
bool testOBBHeightMap(OBB &a, std::vector<float> &h);
#define EPSILON 0.00001
int TestOBBOBB(OBB &a, OBB &b, collision& col);

// Intersect ray R(t) = p + t*d against AABB a. When intersecting,
// return intersection distance tmin and point q of intersection
bool IntersectRayAABB(glm::vec3 p, glm::vec3 d, AABB2 a, float &tmin, glm::vec3 &q, collision& col);

struct ray
{
    ray(const glm::vec3 &orig, const glm::vec3 &dir);
    glm::vec3 orig, dir; // ray orig and dir
    glm::vec3 invdir;
    int sign[3];
};

struct sphere
{
    glm::vec3 c;
    float r;
    sphere(glm::vec3 _c, float _r);
};

bool IntersectRayAABB2(const ray &r, AABB2 a);
bool IntersectRayAABB3(const ray &r, AABB2 a);

// bool IntersectRayAABB4(AABB2 b, Ray r);

// int TestAABBOBB(AABB2& a, OBB& b);
bool _intersectRayTriangle(glm::vec3 &p, glm::vec3 &d, glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &q);
float ScalarTriple(glm::vec3 &a, glm::vec3 &b, glm::vec3 &c, collision& col);
float testPlane(glm::vec3 p, glm::vec4 plane, collision& col);
int IntersectLineTriangle(glm::vec3 p, glm::vec3 q, glm::vec3 a, glm::vec3 b, glm::vec3 c,
                          float &u, float &v, float &w);

int IntersectSegmentTriangle(glm::vec3 p, glm::vec3 q, glm::vec3 a, glm::vec3 b, glm::vec3 c,
                             glm::vec3& result, float &t);

int IntersectTriangleTriangle(std::vector<glm::vec3> &t1, std::vector<glm::vec3> &t2, collision& col);
bool testMeshMesh(mesh &m1, // longer tris
                  const glm::mat4 &trans1,
                  mesh &m2, // shorter tris
                  const glm::mat4 &trans2, collision& col);

extern MESH BOX_MESH;

bool testOBBMesh(OBB &o, const glm::mat4 o_trans, mesh &m, const glm::mat4 m_trans, collision& col);

bool testPointMesh(point &p, mesh &m, glm::vec3 mPos, glm::vec3 mScl, glm::quat mRot, collision& col);
bool testPointOBB(point &p, OBB &o, collision& col);

bool TestSphereSphere(Sphere a, Sphere b, collision& col);
bool TestSpherePlane(Sphere s, Plane p, collision& col);

glm::vec3 ClosestPtPointPlane(glm::vec3 q, Plane p);
namespace YAML
{
    template <>
    struct convert<AABB2>
    {
        static Node encode(const AABB2 &rhs)
        {
            Node node;
            node["min"] = rhs.min;
            node["max"] = rhs.max;
            return node;
        }

        static bool decode(const Node &node, AABB2 &rhs)
        {
            rhs.min = node["min"].as<glm::vec3>();
            rhs.max = node["max"].as<glm::vec3>();
            return true;
        }
    };

    template <>
    struct convert<OBB>
    {
        static Node encode(const OBB &rhs)
        {
            Node node;
            node["c"] = rhs.c;
            node["u"] = rhs.u;
            node["e"] = rhs.e;
            return node;
        }

        static bool decode(const Node &node, OBB &rhs)
        {
            rhs.c = node["c"].as<glm::vec3>();
            rhs.u = node["u"].as<glm::vec3>();
            rhs.e = node["e"].as<glm::vec3>();
            return true;
        }
    };

    template <>
    struct convert<point>
    {
        static Node encode(const point &rhs)
        {
            Node node;
            node.push_back(rhs.pos1);
            node.push_back(rhs.pos2);
            return node;
        }

        static bool decode(const Node &node, point &rhs)
        {
            rhs.pos1 = node[0].as<glm::vec3>();
            rhs.pos2 = node[1].as<glm::vec3>();
            return true;
        }
    };

    template <>
    struct convert<Sphere>
    {
        static Node encode(const Sphere &rhs)
        {
            Node node;
            node = rhs.r;
            return node;
        }

        static bool decode(const Node &node, Sphere &rhs)
        {
            rhs.r = node.as<float>();
            return true;
        }
    };

    template <>
    struct convert<Plane>
    {
        static Node encode(const Plane &rhs)
        {
            Node node;
            node["n"] = rhs.n;
            node["d"] = rhs.d;
            return node;
        }

        static bool decode(const Node &node, Plane &rhs)
        {
            rhs.n = node["n"].as<glm::vec3>();
            rhs.d = node["d"].as<float>();
            return true;
        }
    };


    template <>
    struct convert<mesh>
    {
        static Node encode(const mesh &rhs)
        {
            Node node;
            ENCODE_PROTO(mod);
            // node["m"] = rhs.m;
            return node;
        }

        static bool decode(const Node &node, mesh &rhs)
        {
            // rhs.m = node["m"].as<MESH*>();
            rhs.mod = 0;
            DECODE_PROTO(mod);
            return true;
        }
    };

}
