#include "Model.h"
#include "rendering.h"
#include "concurrency.h"
#include <map>
#include <glm/glm.hpp>

using namespace glm;

vector<vector<float> > noise;
float getNoise(float x, float y){
    x = fmod(x,(float)noise.size());
    y = fmod(y,(float)noise[x].size());

    float xr = fmod(x,1.f);
    float yr = fmod(y,1.f);
    float a1 = noise[(int)x][(int)y];
    float a2 = noise[((int)x + 1) % noise.size()][(int)y];
    float a3 = noise[(int)x][((int)y + 1) % noise[x].size()];
    float a4 = noise[((int)x + 1) % noise.size()][((int)y + 1) % noise[x].size()];
    float x1 = a1 * (1 - xr) + a2 * (xr);
    float x2 = a3 * (1 - xr) + a4 * (xr);
    float h = x1 * (1 - yr) + x2 * (yr);
    return h;
}
class terrain;
map<int,terrain*> terrains;

struct terrainHit{
    vec3 normal;
    float height;
    terrainHit(){}
    terrainHit(vec3 n, float h) : normal(n), height(h){}
};

class terrain : public component{
public:
    COPY(terrain);
    _renderer* r = 0;
    int width;
    int depth;
    bool generated = false;
    void onStart(){
        terrains.insert(std::pair<int,terrain*>(0, transform->gameObject->getComponent<terrain>()));
        
    }
    int xz(int x, int z){
        return x * width + z;
    }
    vector<vector<float> >heightMap;
    terrainHit getHeight(float x, float z){
        if(!generated)
        return terrainHit(vec3(0,1,0),-INFINITY);

        x -= transform->getPosition().x;
        z -= transform->getPosition().z;
        x /= transform->getScale().x;
        z /= transform->getScale().z;
        x += (width - 1) / 2;
        z += (depth - 1) / 2;
        if(x > width - 2.f || z > width - 2.f || x < 0.f || z < 0.f)
            return terrainHit(vec3(0,1,0),-INFINITY);
        // return 1.f;

        float xr = fmod(x,1.f);
        float yr = fmod(z,1.f);
        float a1 = heightMap[(int)x][(int)z];
        vec3 v1 = vec3(0,a1,0);
        float a2 = heightMap[(int)x + 1][(int)z];
        vec3 v2 = vec3(1,a2,0);
        float a3 = heightMap[(int)x][(int)z + 1];
        vec3 v3 = vec3(0,a3,1);
        float a4 = heightMap[(int)x + 1][((int)z + 1)];
        float x1 = a1 * (1 - xr) + a2 * (xr);
        float x2 = a3 * (1 - xr) + a4 * (xr);
        float h = (x1 * (1 - yr) + x2 * (yr)) * transform->getScale().y + transform->getPosition().y;
        vec3 normal = normalize(cross(v1 - v2, v3 - v2));
        return terrainHit(normal,h);
    }
    void genHeightMap(int _width, int _depth){
        width = _width;
        depth = _depth;

        for(int x = 0; x < width; x++){
            noise.push_back(vector<float>());
            for(int z = 0; z < depth; z++){
                noise[x].push_back(randf());
            }
        }
        for(int smoothing = 0; smoothing < 4; smoothing++){
            for(int x = 0; x < width; x++){
                for(int z = 0; z < depth; z++){
                    noise[x][z] = (noise[x][z]
                    + noise[(x - 1 + width) % width][(z - 1 + depth) % depth]
                    + noise[(x - 1 + width) % width][(z + 1) % depth]
                    + noise[(x + 1) % width][(z - 1 + depth) % depth]
                    + noise[(x + 1) % width][(z + 1) % depth]

                    + noise[(x) % width][(z - 1 + depth) % depth]
                    + noise[(x) % width][(z + 1) % depth]
                    + noise[(x - 1  + width) % width][(z) % depth]
                    + noise[(x + 1) % width][(z) % depth]) / 9;
                }
            }
        }
        for(int x = 0; x < width; x++){
            heightMap.push_back(vector<float>());
            for(int z = 0; z < depth; z++){
                heightMap[x].push_back(noise[x][z]* 10.f + getNoise((float)x / 5.f,(float)z / 5.f) * 50.f + getNoise((float)x / 20.f,(float)z / 20.f) * 300.f);
            }
        }
        generated = true;
        enqueRenderJob([&](){this->generate();});
    }
    void generate(){
        
        if(r == 0)
            return;
        _model model = r->getModel();
        // if(!(model.m->model->ready)){
        //     enqueRenderJob([&](){this->generate();});
        //     return;
        // }
        model.meshes().push_back(Mesh());
            
        model.mesh().vertices = vector<glm::vec3>(width * depth);

        vector<glm::vec3>& verts = model.mesh().vertices;
        vector<glm::vec2>& uvs = model.mesh().uvs;
        uvs = vector<glm::vec2>(width * depth);

        Texture grass;
        grass.type = "texture_diffuse";
        grass.load("res/images/grass.jpg");
        model.mesh().textures.push_back(grass);

        Texture rock;
        rock.type = "texture_diffuse";
        rock.load("res/images/rock.jpg");
        model.mesh().textures.push_back(rock);

        glm::vec2 uv;
        for(int x = 0; x < width; x++){
            uv.y = (float)!(bool)(int)uv.y;
            for(int z = 0; z < depth; z++){
                verts[x * width + z] = glm::vec3(x - (width - 1) / 2.0,heightMap[x][z], z - (depth - 1) / 2.0);
                uvs[x * width + z] = uv;
                uv.x = (float)!(bool)(int)uv.x;
            }
        }

        model.mesh().indices = vector<GLuint>((width - 1) * (depth - 1) * 6);
        int k = 0;
        for(int i = 0; i < width - 1; i++){
            for(int j = 0; j < depth - 1; j++){

            
            model.mesh().indices[k++] = xz(i,j);
            model.mesh().indices[k++] = xz(i,j + 1);
            model.mesh().indices[k++] = xz(i + 1,j + 1);
            model.mesh().indices[k++] = xz(i,j);
            model.mesh().indices[k++] = xz(i + 1,j + 1);
            model.mesh().indices[k++] = xz(i + 1,j);
            }
        }
        model.mesh().normals = vector<glm::vec3>(width * depth);
        for(int x = 1; x < width - 1; x++){
            for(int z = 1; z < depth - 1; z++){
                glm::vec3 p = model.mesh().vertices[xz(x,z)];
                glm::vec3 a1 = glm::cross(p - model.mesh().vertices[xz(x,z - 1)],        p - model.mesh().vertices[xz(x - 1,z - 1)]);
                glm::vec3 a2 = glm::cross(p - model.mesh().vertices[xz(x - 1,z - 1)],    p - model.mesh().vertices[xz(x - 1,z)]);
                glm::vec3 a3 = glm::cross(p - model.mesh().vertices[xz(x + 1,z)],        p - model.mesh().vertices[xz(x,z - 1)]);
                glm::vec3 a4 = glm::cross(p - model.mesh().vertices[xz(x - 1,z)],        p - model.mesh().vertices[xz(x,z + 1)]);
                glm::vec3 a5 = glm::cross(p - model.mesh().vertices[xz(x + 1,z + 1)],    p - model.mesh().vertices[xz(x + 1,z)]);
                glm::vec3 a6 = glm::cross(p - model.mesh().vertices[xz(x + 1,z + 1)],    p - model.mesh().vertices[xz(x + 1,z + 1)]);
                model.mesh().normals[xz(x,z)] = glm::vec3(glm::normalize(a1 + a2 + a3 + a4 + a5 + a6));
            }
        }
        model.mesh().reloadMesh();
        model.recalcBounds();
        generated = true;
    }
};