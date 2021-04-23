#include "terrain.h"
#include <imgui/imgui.h>
#include "lineRenderer.h"
#include "particles/gradient.h"

using namespace std;

struct range
{
    glm::ivec2 begin;
    glm::ivec2 end;
    range() = default;
    range(float bx, float ex, float by, float ey) : begin{bx, by}, end{ex, ey} {}
};
void genOctree(chunk &c, vector<glm::vec3>&);
void insertAABB(terr::quad_node &node, array<array<float, terrainSize>, terrainSize> &h, float width, range r);

chunk::chunk()
{
    this->mesh = make_unique<Mesh>();
}

chunk::~chunk()
{
    // waitForRenderJob([&]() {
    // this->mesh.release();
    // });
}

terrain::terrain() = default;
terrain::terrain(const terrain &t)
{
    // for (auto &x : t.chunks)
    // {
    //     for (auto &z : x.second)
    //     {
    //         this->chunks[x.first][z.first] = make_unique<Mesh>(*z.second.get());
    //     }
    // }
}

void DrawLine(glm::vec3 p1, glm::vec3 p2, glm::vec4 color)
{
    addLine(p1, p2, color);
}

void DrawAABB(AABB2 a, glm::vec4 color)
{
    addBox(a.min, a.max - a.min, color);
}

void DrawQuadTree(terr::quad_node &node, int depth, int maxDepth, array<glm::vec4, 100> &colors)
{
    float d = (float)depth / maxDepth;
    glm::vec4 color = colors[d * 100];

    DrawAABB(node.a, color);
    if (node.children)
    {
        for (auto &x : *node.children)
        {
            for (auto &y : x)
            {
                DrawQuadTree(y, depth + 1, maxDepth, colors);
            }
        }
    }
}
void terrain::render(camera &c)
{
    glm::mat4 vp = c.proj * c.rot;
    glm::mat4 model = glm::translate(-c.pos);
    glm::mat4 normalMat = model;
    glm::mat4 mvp = vp * model;

    if (shader.s == 0)
        return;

    auto currShader = shader.meta()->shader.get();
    currShader->use();
    currShader->setFloat("FC", 2.0 / log2(c.farPlane + 1));
    currShader->setVec3("viewPos", c.pos);
    currShader->setVec3("viewDir", c.dir);
    currShader->setFloat("screenHeight", (float)c.height);
    currShader->setFloat("screenWidth", (float)c.width);
    currShader->setMat4("vp", vp);
    currShader->setMat4("model", model);
    currShader->setMat4("normalMat", normalMat);
    currShader->setMat4("mvp", mvp);

    for (auto &x : chunks)
    {
        for (auto &z : x.second)
        {
            if (z.second)
            {
                auto mesh = z.second->mesh.get();
                glBindVertexArray(mesh->VAO);
                glDrawElements(currShader->primitiveType, mesh->indices.size(), GL_UNSIGNED_INT, 0);

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);
            }
        }
    }

    colorArray gradient;
    gradient.addKey(glm::vec4(1, 0, 0, 1), 0)
        .addKey(glm::vec4(1, 1, 0, 1), 1.f / 6.f)
        .addKey(glm::vec4(0, 1, 0, 1), 2.f / 6.f)
        .addKey(glm::vec4(0, 1, 1, 1), 3.f / 6.f)
        .addKey(glm::vec4(0, 0, 1, 1), 4.f / 6.f)
        .addKey(glm::vec4(1, 0, 1, 1), 5.f / 6.f);

    array<glm::vec4, 100> colors;
    gradient.setColorArray(colors.data());

    int max_depth = 0;

    if (chunks.size() > 0 && chunks.begin()->second.size() > 0)
    {
        terr::quad_node *node = &chunks.begin()->second.begin()->second->quadtree;
        while (true)
        {
            if (node->children)
                node = &node->children->at(0)[0];
            else
                break;
            ++max_depth;
        }
    }

    static bool drawOctree = true;

    if (ImGui::GetIO().KeysDownDuration[GLFW_KEY_O] == 0)
    {
        drawOctree = !drawOctree;
    }
    if (drawOctree)
    {
        for (auto &[x, m] : chunks)
        {
            for (auto &[z, c] : m)
            {

                DrawQuadTree(c->quadtree, 0, max_depth + 1, colors);
            }
        }
    }
}

int terrain::xz(int x, int z)
{
    return x * terrainSize + z;
}
int xz(int x, int z)
{
    return x * terrainSize + z;
}
float terrain::makeHeight(float x, float z)
{
    // return getNoise((float)x / 20.f, (float)z / 20.f) * 300.f + getNoise(x, z) * 10.f + getNoise((float)x / 5.f, (float)z / 5.f) * 50.f + getNoise(x * 5.f, z * 5.f) * 2.f;
    float normalized = glm::simplex(glm::vec2{x, z} / 50.f) + glm::simplex(glm::vec2{x, z} / 10.f) * 0.2f + glm::simplex(glm::vec2{x, z} / 5.f) * 0.02f;
    normalized /= 1.22;
    normalized = normalized * 0.5 + 0.5;
    normalized *= normalized * normalized;
    return normalized * 10.f;
}

glm::vec3 terrain::makeVert(float x, float z)
{
    return glm::vec3(x - terrainSize / 2, makeHeight(x + 0, z + 0), z - terrainSize / 2) * 10.f;
}
void terrain::genHeight(int _x, int _z)
{
    chunks[_x][_z] = make_shared<chunk>();
    array<array<float, terrainSize>, terrainSize> &h = chunks[_x][_z]->h;
    // chunks[_x][_z].mesh = make_unique<Mesh>();
    Mesh *mesh = chunks[_x][_z]->mesh.get();

    mesh->vertices.resize(terrainSize * terrainSize);
    mesh->normals.resize(terrainSize * terrainSize);

    for (int x = 0; x < terrainSize; x++)
    {
        for (int z = 0; z < terrainSize; z++)
        {
            mesh->vertices[x * terrainSize + z] = makeVert((float)x + _x * (terrainSize - 1), (float)z + _z * (terrainSize - 1));
            h[x][z] = mesh->vertices[x * terrainSize + z].y;
        }
    }
    auto _makeVert = [&](int x, int z){
        return makeVert((float)x + _x * (terrainSize - 1), (float)z + _z * (terrainSize - 1));
    };
    for (int x = 0; x < terrainSize; x++)
    {
        for (int z = 0; z < terrainSize; z++)
        {
            if (x == 0 || x == terrainSize - 1 || z == 0 || z == terrainSize - 1)
            {
                glm::vec3 p = mesh->vertices[xz(x, z)];
                glm::vec3 a1 = glm::cross(p - _makeVert(x, z - 1), p - _makeVert(x - 1, z - 1));
                glm::vec3 a2 = glm::cross(p - _makeVert(x - 1, z - 1), p - _makeVert(x - 1, z));
                glm::vec3 a3 = glm::cross(p - _makeVert(x + 1, z), p - _makeVert(x, z - 1));
                glm::vec3 a4 = glm::cross(p - _makeVert(x - 1, z), p - _makeVert(x, z + 1));
                glm::vec3 a5 = glm::cross(p - _makeVert(x + 1, z + 1), p - _makeVert(x + 1, z));
                glm::vec3 a6 = glm::cross(p - _makeVert(x + 1, z + 1), p - _makeVert(x + 1, z + 1));
                mesh->normals[xz(x, z)] = glm::vec3(glm::normalize(a1 + a2 + a3 + a4 + a5 + a6));
            }
            else
            {
                glm::vec3 p = mesh->vertices[xz(x, z)];
                glm::vec3 a1 = glm::cross(p - mesh->vertices[xz(x, z - 1)], p - mesh->vertices[xz(x - 1, z - 1)]);
                glm::vec3 a2 = glm::cross(p - mesh->vertices[xz(x - 1, z - 1)], p - mesh->vertices[xz(x - 1, z)]);
                glm::vec3 a3 = glm::cross(p - mesh->vertices[xz(x + 1, z)], p - mesh->vertices[xz(x, z - 1)]);
                glm::vec3 a4 = glm::cross(p - mesh->vertices[xz(x - 1, z)], p - mesh->vertices[xz(x, z + 1)]);
                glm::vec3 a5 = glm::cross(p - mesh->vertices[xz(x + 1, z + 1)], p - mesh->vertices[xz(x + 1, z)]);
                glm::vec3 a6 = glm::cross(p - mesh->vertices[xz(x + 1, z + 1)], p - mesh->vertices[xz(x + 1, z + 1)]);
                mesh->normals[xz(x, z)] = glm::vec3(glm::normalize(a1 + a2 + a3 + a4 + a5 + a6));
            }
        }
    }

    mesh->indices.resize((terrainSize - 1) * (terrainSize - 1) * 6);
    int k = 0;
    for (int i = 0; i < terrainSize - 1; i++)
    {
        for (int j = 0; j < terrainSize - 1; j++)
        {
            mesh->indices[k++] = xz(i, j);
            mesh->indices[k++] = xz(i, j + 1);
            mesh->indices[k++] = xz(i + 1, j + 1);
            mesh->indices[k++] = xz(i, j);
            mesh->indices[k++] = xz(i + 1, j + 1);
            mesh->indices[k++] = xz(i + 1, j);
        }
    }

    genOctree(*chunks[_x][_z],mesh->vertices);
    mesh->makePoints();
    waitForRenderJob([=]() {
        mesh->reloadMesh();
    });
}
void terrain::init(int i)
{
    // genHeight(0, 0);
    renderShit.emplace(1, [&](camera &c) {
        this->render(c);
    });
}

void terrain::onStart()
{
    chunks.clear();
}
glm::vec3 playerPos;

void terrain::update()
{

    int radius = 8;
    glm::vec3 playerPosScaled = playerPos / (float)((terrainSize - 1) * 10);
    vector<int> v;
    for (auto [x, m] : chunks)
    {
        v.push_back(x);
    }
    for (auto &x : v)
    {
        if (chunks[x].size() == 0)
        {
            chunks.erase(x);
        }
    }

    [&] {
        for (auto &[x, m] : chunks)
        {
            vector<int> _z;
            for (auto [z, p] : m)
            {
                _z.push_back(z);
            }
            for (auto &z : _z)
            {
                if (glm::length(glm::vec2(x, z) - glm::vec2(playerPosScaled.x, playerPosScaled.z)) >= radius)
                {
                    // if (chunks.find(x) != chunks.end() && chunks.at(x).find(z) != chunks.at(x).end())
                    chunks.at(x).erase(z);
                    // return;
                }
            }
        }
    }();
    // }));

    bool generatedChunk = false;
    for (int x = playerPosScaled.x - radius; x <= playerPosScaled.x + radius && !generatedChunk; x++)
    {
        for (int z = playerPosScaled.z - radius; z <= playerPosScaled.z + radius && !generatedChunk; z++)
        {
            if (glm::length(glm::vec2(x, z) - glm::vec2(playerPosScaled.x, playerPosScaled.z)) < radius)
            {
                if (chunks.find(x) == chunks.end() || chunks.at(x).find(z) == chunks.at(x).end() || chunks.at(x).at(z) == 0)
                {
                    genHeight(x, z);
                    // generatedChunk = true;
                }
            }
        }
    }
}

void terrain::deinit()
{
    enqueRenderJob([&]() {
        chunks.clear();
    });
    renderShit.erase(1);
}

void terrain::IntersectRayQuadTree(chunk* _chunk,terr::quad_node &node, ray &r, glm::vec3 &result, float &t)
{
    if (!node.children)
    {
        glm::vec3 p1, p2, p3, p4;
        p1 = _chunk->mesh->vertices[xz(node.quad.x, node.quad.y)];
        p2 = _chunk->mesh->vertices[xz(node.quad.x + 1, node.quad.y)];
        p3 = _chunk->mesh->vertices[xz(node.quad.x + 1, node.quad.y + 1)];
        p4 = _chunk->mesh->vertices[xz(node.quad.x, node.quad.y + 1)];

        glm::vec3 res;
        if (_intersectRayTriangle(r.orig, r.dir, p1, p2, p3, res))
        {
            float _t = glm::length(r.orig - res);
            if (_t < t)
            {
                result = res;
                t = _t;
            }
            return;
        }
        else if (_intersectRayTriangle(r.orig, r.dir, p1, p3, p4, res))
        {
            float _t = glm::length(r.orig - res);
            if (_t < t)
            {
                result = res;
                t = _t;
            }
            return;
        }

        else
        {
            // hit = false;
            return;
        }

        // result = p1;
        // return;
    }
    if (IntersectRayAABB3(r, node.a))
    {
        for (auto &x : *node.children)
        {
            for (auto &y : x)
            {
                IntersectRayQuadTree(_chunk, y, r, result, t);
                // if (t > 0)
                //     return;
            }
        }
    }
}

bool terrain::IntersectRayTerrain(glm::vec3 p, glm::vec3 dir, glm::vec3 &result)
{
    bool hit = false;
    ray r(p, dir);
    float t = numeric_limits<float>::max();
    auto terr = COMPONENT_LIST(terrain)->get(0);
    for(auto &x : terr->chunks){
        for(auto &y : x.second){
            terr->IntersectRayQuadTree(y.second.get(), y.second->quadtree,r,result,t);
            if(t != numeric_limits<float>::max())
                return true;
        }
    }
    return false;
}

void insertAABB(terr::quad_node &node, vector<glm::vec3>& verts, float width, range r)
{

    // generate aab for node with range
    float min, max;
    min = 1000000.f;
    max = -min;
    for (int i = r.begin.x; i <= r.end.x; i++)
    {
        for (int j = r.begin.y; j <= r.end.y; j++)
        {
            if (verts[xz(i,j)].y > max)
            {
                max = verts[xz(i,j)].y;
            }
            if (verts[xz(i,j)].y < min)
            {
                min = verts[xz(i,j)].y;
            }
        }
    }

    AABB2 aabb;
    
    aabb.min.x = verts[xz(r.begin.x,r.begin.y)].x;
    aabb.min.z = verts[xz(r.begin.x,r.begin.y)].z;
    aabb.max.x = verts[xz(r.end.x,r.end.y)].x;
    aabb.max.z = verts[xz(r.end.x,r.end.y)].z;
    aabb.min.y = min;
    aabb.max.y = max;
    node.a = aabb;

    if (r.end.x - r.begin.x == 1 && r.end.y - r.begin.y == 1)
    {
        //node.triangle = range
        node.quad = r.begin;
        return;
    }

    // make children and give ranges
    node.children = make_unique<quadrant>();
    glm::vec2 r_middle = (r.begin + r.end) / 2;

    // left forward
    insertAABB(node.children->at(0)[0], verts, width, range(r.begin.x, r_middle.x, r.begin.y, r_middle.y));
    //right forward
    insertAABB(node.children->at(1)[0], verts, width, range(r_middle.x, r.end.x, r.begin.y, r_middle.y));
    // left back
    insertAABB(node.children->at(0)[1], verts, width, range(r.begin.x, r_middle.x, r_middle.y, r.end.y));
    // right back
    insertAABB(node.children->at(1)[1], verts, width, range(r_middle.x, r.end.x, r_middle.y, r.end.y));
}

void genOctree(chunk &c, vector<glm::vec3>& verts)
{
    insertAABB(c.quadtree, verts, 10.f, range(0, terrainSize - 1, 0, terrainSize - 1));
}

REGISTER_COMPONENT(terrain);