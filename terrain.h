#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include "Model.h"
#include "rendering.h"
#include "concurrency.h"
#include <map>
#include "Component.h"
#include "camera.h"

using namespace glm;

void genNoise(int _width, int _depth, int smoothness);
float getNoise(float x, float y);

struct terrainHit
{
    vec3 normal;
    float height;
    terrainHit() {}
    terrainHit(vec3 n, float h) : normal(n), height(h) {}
};

class terrain : public component
{
public:
    COPY(terrain);
    void onEdit();
    game_object_prototype scatter_obj;
    vector<transform2> scatter;
    vector<glm::vec3> scatterPos;
    _renderer *r = 0;
    static atomic<int> terrId;
    int width = 0;
    int depth = 0;
    int offsetX;
    int offsetZ;
    float middle;
    float max_height;
    bool generated = false;
    vector<vector<float>> heightMap;
    int xz(int x, int z);
    terrainHit getHeight(float x, float z);
    float makeHeight(float x, float z);
    glm::vec3 makeVert(float x, float z);
    void onStart();
    void onDestroy();
    void update();
    void init();
    void deinit();
    void genHeightMap(int _width, int _depth, int _offsetX, int _offsetZ);
    void generate();
    ~terrain();
    SER6(width,depth,offsetX,offsetZ, scatter_obj, scatter);

};
extern int terrainWidth;
extern map<int, map<int, terrain *>> terrains;

terrain *getTerrain(float x, float z);