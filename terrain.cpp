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

#define octant array<array<array<oct_node,2>,2>,2>
namespace terr{
struct oct_node{
    AABB2 a;
    unique_ptr<octant> children;
};
};

struct chunk{
    unique_ptr<Mesh> mesh;
    array<array<float,100>,100> heightMap;
    terr::oct_node octree;

};

class terrain : public component {
    map<int,map<int,unique_ptr<Model>>> chunks;
public:
    _shader shader;
    terrain() = default;
    terrain(const terrain& t){
        for(auto& x : t.chunks){
            for(auto& z : x.second){
                this->chunks[x.first][z.first] = make_unique<Model>(*z.second.get());
            }
        }
    }
    void render(camera& c){
		glm::mat4 vp = c.proj * c.rot;
        glm::mat4 model = glm::translate(-c.pos);
        glm::mat4 normalMat = model;
        glm::mat4 mvp = vp * model;

        if(shader.s == 0)
            return;

        auto currShader = shader.meta()->shader.get();
        currShader->use();
		currShader->setFloat("FC", 2.0 / log2(c.farPlane + 1));
		currShader->setVec3("viewPos", c.pos);
		currShader->setVec3("viewDir", c.dir);
		currShader->setFloat("screenHeight", (float)c.height);
		currShader->setFloat("screenWidth", (float)c.width);
		currShader->setMat4("vp", vp);
        currShader->setMat4("model",model);
        currShader->setMat4("normalMat",normalMat);
        currShader->setMat4("mvp",mvp);

        auto mesh = chunks[0][0]->meshes[0];
        glBindVertexArray(mesh->VAO);
        glDrawElements(currShader->primitiveType, mesh->indices.size(), GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

    }
    int xz(int x, int z){
        return x * 100 + z;
    }
    float makeHeight(float x, float z){
        // return getNoise((float)x / 20.f, (float)z / 20.f) * 300.f + getNoise(x, z) * 10.f + getNoise((float)x / 5.f, (float)z / 5.f) * 50.f + getNoise(x * 5.f, z * 5.f) * 2.f;
        return glm::simplex(glm::vec2{x,z} / 50.f) * 5.f + glm::simplex(glm::vec2{x,z} / 10.f) + glm::simplex(glm::vec2{x,z} / 5.f) / 5.f;
    }
    
    glm::vec3 makeVert(float x, float z)
    {
        return glm::vec3(x - 100 / 2, makeHeight(x + 0, z + 0), z - 100 / 2) * 10.f;
    }
    void genHeight(){

        array<array<float,100>,100> h;
        chunks[0][0] = make_unique<Model>();
        auto m = chunks[0][0].get();
        m->meshes.push_back(new Mesh());
        auto mesh = m->meshes.front();

        mesh->vertices.resize(100 * 100);
        mesh->normals.resize(100 * 100);

        for(int x = 0; x < 100; x++){
            for(int z = 0; z < 100; z++){
                mesh->vertices[x * 100 + z] = makeVert((float)x,(float)z);
                h[x][z] = mesh->vertices[x * 100 + z].y;
            }
        }
        for (int x = 0; x < 100; x++)
        {
            for (int z = 0; z < 100; z++)
            {
                if (x == 0 || x == 100 - 1 || z == 0 || z == 100 - 1)
                {
                    glm::vec3 p = mesh->vertices[xz(x,z)];
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
                    glm::vec3 p = mesh->vertices[xz(x,z)];
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

        mesh->indices.resize((100 - 1) * (100 - 1) * 6);
        int k = 0;
        for (int i = 0; i < 100 - 1; i++)
        {
            for (int j = 0; j < 100 - 1; j++)
            {
                mesh->indices[k++] = xz(i, j);
                mesh->indices[k++] = xz(i, j + 1);
                mesh->indices[k++] = xz(i + 1, j + 1);
                mesh->indices[k++] = xz(i, j);
                mesh->indices[k++] = xz(i + 1, j + 1);
                mesh->indices[k++] = xz(i + 1, j);
            }
        }
        mesh->makePoints();
        enqueRenderJob([&](){
            mesh->reloadMesh();
        });
    }
    void init(int i){
        genHeight();
        renderShit.emplace(1,[&](camera& c){
            this->render(c);
        });
    }
    void deinit(){
        enqueRenderJob([&](){
            chunks.clear();
        });
    }

    struct range{
        glm::ivec2 begin;
        glm::ivec2 end;
    };
    void insertAABB(terr::oct_node& node,array<array<float,100>,100>& h, float width, range r){
        AABB2 a = node.a;
        glm::vec3 middle = (a.max + a.min) / 2.f;
        glm::vec2 r_middle = (r.begin + r.end) / 2;
        node.children = make_unique<octant>();

        for(int x = 0; x < 2; x++){
            for(int y = 0; y < 2; y++){
                for(int z = 0; z < 2; z++){
                    range _r;
                    _r.begin.x = (x == 0 ? r.begin.x : r_middle.x);
                    _r.begin.y = (y == 0 ? r.begin.y : r_middle.y);
                    _r.end.x = (x == 0 ? r_middle.x : r.end.x);
                    _r.end.y = (y == 0 ? r_middle.y : r.end.y);
                    insertAABB(node.children[x][y][z],h,width,_r);

                }
            }
        }
        c.
    }

    void genOctree(chunk& c){
        float min, max;
        min = 1000000.f;
        max = -min;
        for(int i = 0; i < 100; i++){
            for(int j = 0; j < 100; j++){
                if(c.heightMap[i][j] >  max){
                    max = c.heightMap[i][j];
                }
                if(c.heightMap[i][j] < min){
                    min = c.heightMap[i][j];
                }
            }
        }

        AABB2 aabb;
        aabb.min.x = c.mesh->vertices[0].x;
        aabb.min.z = c.mesh->vertices[0].z;
        aabb.max = -aabb.min;
        aabb.min.y = min;
        aabb.max.y = max;

        terr::oct_node node;
        node.a = aabb;
        insertAABB(node,aabb);
    }


    SER_FUNC()
        SER(shader)
    SER_END;

    COPY(terrain);
};

REGISTER_COMPONENT(terrain);