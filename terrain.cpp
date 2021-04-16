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
void genOctree(chunk &c);
void insertAABB(terr::quad_node &node, array<array<float, terrainSize>, terrainSize> &h, float width, range r);

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

    glm::vec3 dims = a.max - a.min;
    glm::vec3 x = glm::vec3(dims.x, 0, 0);
    glm::vec3 y = glm::vec3(0, dims.y, 0);
    glm::vec3 z = glm::vec3(0, 0, dims.z);
    glm::vec3 p1, p2, p3, p4, p5, p6, p7, p8, p9;
    p1 = a.min;
    p2 = a.min + x;
    p3 = a.min + x + z;
    p4 = a.min + z;
    p5 = a.min + y;
    p6 = a.min + x + y;
    p7 = a.min + x + z + y;
    p8 = a.min + z + y;

    // // bottom square
    DrawLine(p1, p2, color);
    DrawLine(p2, p3, color);
    DrawLine(p3, p4, color);
    DrawLine(p4, p1, color);

    // top square
    DrawLine(p5, p6, color);
    DrawLine(p6, p7, color);
    DrawLine(p7, p8, color);
    DrawLine(p8, p5, color);

    DrawLine(p1, p5, color);
    DrawLine(p2, p6, color);
    DrawLine(p3, p7, color);
    DrawLine(p4, p8, color);
}

void DrawQuadTree(terr::quad_node &node, int depth, int maxDepth, array<glm::vec4, 100> &colors)
{
    float d = (float)depth / maxDepth;
    glm::vec4 color = colors[d * 100];

    DrawAABB(node.a,color);
    if(node.children){
        for(auto& x : *node.children){
            for(auto &y : x){
                DrawQuadTree(y,depth + 1, maxDepth,colors);
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

    auto mesh = chunks[0][0].mesh.get();
    glBindVertexArray(mesh->VAO);
    glDrawElements(currShader->primitiveType, mesh->indices.size(), GL_UNSIGNED_INT, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

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
    terr::quad_node *node = &chunks[0][0].quadtree;
    while (true)
    {
        if (node->children)
            node = &node->children->at(0)[0];
        else
            break;
        ++max_depth;
    }

    static bool drawOctree = true;

    if(ImGui::GetIO().KeysDownDuration[GLFW_KEY_O] == 0){
        drawOctree = !drawOctree;
    }
    if(drawOctree)
        DrawQuadTree(chunks[0][0].quadtree, 0, max_depth + 1, colors);
}

int terrain::xz(int x, int z)
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
void terrain::genHeight()
{

    array<array<float, terrainSize>, terrainSize> &h = chunks[0][0].h;
    chunks[0][0].mesh = make_unique<Mesh>();
    Mesh *mesh = chunks[0][0].mesh.get();

    mesh->vertices.resize(terrainSize * terrainSize);
    mesh->normals.resize(terrainSize * terrainSize);

    for (int x = 0; x < terrainSize; x++)
    {
        for (int z = 0; z < terrainSize; z++)
        {
            mesh->vertices[x * terrainSize + z] = makeVert((float)x, (float)z);
            h[x][z] = mesh->vertices[x * terrainSize + z].y;
        }
    }
    for (int x = 0; x < terrainSize; x++)
    {
        for (int z = 0; z < terrainSize; z++)
        {
            if (x == 0 || x == terrainSize - 1 || z == 0 || z == terrainSize - 1)
            {
                glm::vec3 p = mesh->vertices[xz(x, z)];
                glm::vec3 a1 = glm::cross(p - makeVert(x, z - 1), p - makeVert(x - 1, z - 1));
                glm::vec3 a2 = glm::cross(p - makeVert(x - 1, z - 1), p - makeVert(x - 1, z));
                glm::vec3 a3 = glm::cross(p - makeVert(x + 1, z), p - makeVert(x, z - 1));
                glm::vec3 a4 = glm::cross(p - makeVert(x - 1, z), p - makeVert(x, z + 1));
                glm::vec3 a5 = glm::cross(p - makeVert(x + 1, z + 1), p - makeVert(x + 1, z));
                glm::vec3 a6 = glm::cross(p - makeVert(x + 1, z + 1), p - makeVert(x + 1, z + 1));
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

    genOctree(chunks[0][0]);
    mesh->makePoints();
    enqueRenderJob([&]() {
        chunks[0][0].mesh->reloadMesh();
    });
}
void terrain::init(int i)
{
    genHeight();
    renderShit.emplace(1, [&](camera &c) {
        this->render(c);
    });
}
void terrain::deinit()
{
    enqueRenderJob([&]() {
        chunks.clear();
    });
}

void terrain::IntersectRayQuadTree(terr::quad_node &node, ray &r, glm::vec3 &result, float& t)
{
    if (!node.children)
    {
        glm::vec3 p1, p2, p3, p4;
        p1 = chunks[0][0].mesh->vertices[xz(node.quad.x, node.quad.y)];
        p2 = chunks[0][0].mesh->vertices[xz(node.quad.x + 1, node.quad.y)];
        p3 = chunks[0][0].mesh->vertices[xz(node.quad.x + 1, node.quad.y + 1)];
        p4 = chunks[0][0].mesh->vertices[xz(node.quad.x, node.quad.y + 1)];

        glm::vec3 res;
        if (_intersectRayTriangle(r.orig, r.dir, p1, p2, p3, res))
        {
            float _t = glm::length(r.orig - res);
            if(_t < t){
                result = res;
                t = _t;
            }
            return;
        }
        else if (_intersectRayTriangle(r.orig, r.dir, p1, p3, p4, res))
        {
            float _t = glm::length(r.orig - res);
            if(_t < t){
                result = res;
                t = _t;
            }
            return;
        }

        else{
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
                IntersectRayQuadTree(y, r, result, t);
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
    COMPONENT_LIST(terrain)->get(0)->IntersectRayQuadTree(COMPONENT_LIST(terrain)->get(0)->chunks[0][0].quadtree, r, result, t);
    return t != numeric_limits<float>::max();
}

void insertAABB(terr::quad_node &node, array<array<float, terrainSize>, terrainSize> &h, float width, range r)
{

    // generate aab for node with range
    float min, max;
    min = 1000000.f;
    max = -min;
    for (int i = r.begin.x; i <= r.end.x; i++)
    {
        for (int j = r.begin.y; j <= r.end.y; j++)
        {
            if (h[i][j] > max)
            {
                max = h[i][j];
            }
            if (h[i][j] < min)
            {
                min = h[i][j];
            }
        }
    }

    AABB2 aabb;
    aabb.min.x = (r.begin.x - terrainSize / 2) * width;
    aabb.min.z = (r.begin.y - terrainSize / 2) * width;
    aabb.max.x = (r.end.x - terrainSize / 2) * width;
    aabb.max.z = (r.end.y - terrainSize / 2) * width;
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
    insertAABB(node.children->at(0)[0], h, width, range(r.begin.x, r_middle.x, r.begin.y, r_middle.y));
    //right forward
    insertAABB(node.children->at(1)[0], h, width, range(r_middle.x, r.end.x, r.begin.y, r_middle.y));
    // left back
    insertAABB(node.children->at(0)[1], h, width, range(r.begin.x, r_middle.x, r_middle.y, r.end.y));
    // right back
    insertAABB(node.children->at(1)[1], h, width, range(r_middle.x, r.end.x, r_middle.y, r.end.y));
}

void genOctree(chunk &c)
{
    insertAABB(c.quadtree, c.h, 10.f, range(0, terrainSize - 1, 0, terrainSize - 1));
}

REGISTER_COMPONENT(terrain);