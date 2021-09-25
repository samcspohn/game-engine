#pragma once
#include "components/Component.h"
#include "components/game_object.h"
#include "_rendering/Model.h"
#include <map>
#include <glm/gtc/noise.hpp>
#include "_rendering/_renderer.h"
#include "_rendering/camera.h"
#include <array>
#include "physics/collision.h"
using namespace std;

#define quadrant array<array<terr::quad_node, 2>, 2>
#define terrainSize 17
namespace terr
{
    struct quad_node
    {
        AABB2 a;
        unique_ptr<quadrant> children;
        glm::ivec2 quad;
    };
};

struct chunk
{
    unique_ptr<Mesh> mesh;
    array<array<float, terrainSize>, terrainSize> h;
    terr::quad_node quadtree;
    chunk();
    ~chunk();
};
struct terrainHit
{
    glm::vec3 normal;
    float height;
    terrainHit() {}
    terrainHit(glm::vec3 n, float h) : normal(n), height(h) {}
};

extern glm::vec3 playerPos;
class terrain : public component
{
    map<int, map<int, shared_ptr<chunk>>> chunks;

public:
    _shader shader;
    terrain();
    terrain(const terrain &t);
    void render(camera &c);
    static int xz(int x, int z);
    float makeHeight(float x, float z);

    glm::vec3 makeVert(float x, float z);
    void onStart();
    void update();
    void genHeight(int x, int z);
    static terrainHit getHeight(float x, float z);
    void init(int i);
    void deinit();

    void IntersectRayQuadTree(chunk* _chunk, terr::quad_node &node, ray &r, glm::vec3 &result, float& t);
    static bool IntersectRayTerrain(glm::vec3 p, glm::vec3 dir, glm::vec3 &result);

    SER_FUNC(){
        SER(shader)
    }
};
