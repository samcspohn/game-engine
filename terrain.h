#include "Model.h"
#include "rendering.h"
#include "concurrency.h"
#include <map>
#include <glm/glm.hpp>

using namespace glm;

vector<vector<float> > noise;
void genNoise(int _width, int _depth, int smoothness){
    int width = _width;
    int depth = _depth;

    for(int x = 0; x < width; x++){
        noise.push_back(vector<float>());
        for(int z = 0; z < depth; z++){
            noise[x].push_back(randf());
        }
    }
    for(int smoothing = 0; smoothing < smoothness; smoothing++){
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
}
float getNoise(float x, float y){
    x = fmod(x,(float)noise.size());
    if(x < 0.f){
        x += (float)noise.size();
    }
    y = fmod(y,(float)noise[x].size());
    if(y < 0.f){
        y += (float)noise.size();
    }

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
int terrainWidth = 1;

class terrain;
map<int,map<int,terrain*>> terrains;

terrain* getTerrain(float x, float z){
    float width = terrainWidth;
    float scale = 20.f;
    auto xt = terrains.find((int)(x / scale / width + (x > 0 ? 1 : -1) * 0.5f));
    if(xt != terrains.end()){
        auto zt = xt->second.find((int)(z / scale / width + (z > 0 ? 1 : -1) * 0.5f));
        if(zt != xt->second.end()){
            return zt->second;
        }
    }
    return 0;
}

struct terrainHit{
    vec3 normal;
    float height;
    terrainHit(){}
    terrainHit(vec3 n, float h) : normal(n), height(h){}
};

class terrain : public component{
public:
    COPY(terrain);
    game_object_proto* scatter_obj;
    vector<game_object*> scatter;
    vector<glm::vec3> scatterPos;
    _renderer* r = 0;
    static atomic<int> terrId;
    int width;
    int depth;
    int offsetX;
    int offsetZ;
    float middle;
    float max_height;
    bool generated = false;
    int xz(int x, int z){
        return x * width + z;
    }
    vector<vector<float> >heightMap;
    terrainHit getHeight(float x, float z){
        if(!generated)
        return terrainHit(vec3(0,1,0),-INFINITY);

        vec3 scale = transform->getScale();
        vec3 pos = transform->getPosition();
        (int)(x / scale.x / width + (x > 0 ? 1 : -1) * 0.5f);
        x -= pos.x;// - width / 2 * scale.x;
        z -= pos.z;// - depth / 2 * scale.z;
        x /= scale.x;
        z /= scale.z;
        x += (width) / 2;
        z += (depth) / 2;
        if(x >= width - 1.f || z >= width - 1.f || x < 0.f || z < 0.f)
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
        float h = (x1 * (1 - yr) + x2 * (yr)) * scale.y + pos.y;
        vec3 normal = normalize(cross(v1 - v2, v3 - v2));
        return terrainHit(normal,h);
    }
    float makeHeight(float x, float z){
        return getNoise(x,z)* 10.f + getNoise((float)x / 5.f,(float)z / 5.f) * 50.f + getNoise((float)x / 20.f,(float)z / 20.f) * 300.f;
    }
    glm::vec3 makeVert(float x, float z){
        return glm::vec3(x - width / 2,makeHeight(x + this->offsetX,z + this->offsetZ),z - width / 2);
    }
    void update(){
        auto c = COMPONENT_LIST(_camera);
        vec3 pos = transform->getPosition();
        vec3 pos2 = c->get(0)->transform->getPosition();
        pos.y = pos2.y = 0;
        bool inThreshold = glm::length(pos - pos2) < 2000;
        if( inThreshold && scatter.size() == 0 )
        {
            for(glm::vec3 p : scatterPos){
                game_object* s = new game_object(*scatter_obj);
                s->transform->setPosition(p);
                s->transform->rotate(vec3(1,0,0),radians(-90.f));
                scatter.push_back(s);
            }
        }else if(!inThreshold && scatter.size() > 0){
            for(game_object* g : scatter){
                g->destroy();
            }
            scatter.clear();
        }
    }
    void genHeightMap(int _width, int _depth, int _offsetX, int _offsetZ){
        width = _width + 1;
        depth = _depth + 1;
        this->offsetX = _offsetX;
        this->offsetZ = _offsetZ;
        float min = 1000000;
        float max = -1000000;
        for(int x = offsetX; x < width + offsetX; x++){
            heightMap.push_back(vector<float>());
            for(int z = offsetZ; z < depth + offsetZ; z++){
                float h = makeHeight(x,z);
                heightMap.back().push_back(h);
                if(h > max) max = h;
                if(h < min) min = h;
            }
        }
        middle = (max + min) / 2;
        max_height = max;
        glm::vec2 index = transform->getPosition().xz() / transform->getScale().xz() / glm::vec2(width -1 ,depth - 1);
        terrains[index.x][index.y] = transform->gameObject->getComponent<terrain>();
        generated = true;

        // game_object* tree_go = new game_object(scatter_obj);
        // tree_go->addComponent<_renderer>()->set(modelShader,tree);
        // tree_go->transform->rotate(vec3(1,0,0),radians(-90.f));
        glm::vec3 pos = transform->getPosition();
        for(int i = -width / 2; i < width / 2; i++){
            for(int j = -depth / 2; j < depth / 2; j++){
                float x = pos.x + (i + randf()) * 20.f;
                float z = pos.z + (j + randf()) * 20.f;
                terrainHit h = getHeight(x, z);
                if(dot(h.normal, vec3(0,1,0)) > 0.85){
                    // game_object* s = new game_object(*scatter_obj);
                    scatterPos.emplace_back(x,h.height,z);
                    // s->transform->setPosition(vec3(x,h.height,z));
                    // s->transform->rotate(vec3(1,0,0),radians(-90.f));
                }
            }	
        }
        enqueRenderJob([&](){this->generate();});
    }
    void generate(){
        
        if(r == 0)
            return;
        _model model = r->getModel();
        if(model.meshes().size() == 0){
            model.meshes().push_back(Mesh());
            // cout << "added mesh" << endl;
        }
            
        model.mesh().vertices = vector<glm::vec3>(width * depth);

        vector<glm::vec3>& verts = model.mesh().vertices;
        vector<glm::vec2>& uvs = model.mesh().uvs;
        vector<glm::vec2>& uvs2 = model.mesh().uvs2;
        uvs = vector<glm::vec2>(width * depth);
        uvs2 = vector<glm::vec2>(width * depth);

        _texture grass;
        grass.load("res/images/grass.jpg");
        grass.setType("texture_diffuse");
        model.mesh().addTexture(grass);

        _texture rock;
        rock.load("res/images/rock.jpg");
        rock.setType("texture_diffuse");
        model.mesh().addTexture(rock);


        _texture mountain;
        mountain.load("res/images/mountain.jpg");
        mountain.setType("texture_diffuse");
        model.mesh().addTexture(mountain);

        glm::vec2 uv;
        glm::vec2 uv2;
        for(int x = 0; x < width; x++){
            uv.y = (float)!(bool)(int)uv.y;
            float u = (float)(x % 8) / 4;
            if(x % 8 >= 4)
                    uv2.y = 2 - u;
                else
                    uv2.y = u;
            for(int z = 0; z < depth; z++){
                verts[x * width + z] = glm::vec3(x - (width - 1) / 2.0,heightMap[x][z], z - (depth - 1) / 2.0);
                uvs[x * width + z] = uv;
                uv.x = (float)!(bool)(int)uv.x;

                float u = (float)(z % 8) / 4;
                if(z % 8 >= 4)
                    uv2.x = 2 - u;
                else
                    uv2.x = u;
                uvs2[x * width + z] = uv2;
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
        for(int x = 0; x < width; x++){
            for(int z = 0; z < depth; z++){
                if(x == 0 || x == width - 1 || z == 0 || z == width - 1){
                    glm::vec3 p = model.mesh().vertices[xz(x,z)];
                    glm::vec3 a1 = glm::cross(p - makeVert(x,z - 1),        p - makeVert(x - 1,z - 1));
                    glm::vec3 a2 = glm::cross(p - makeVert(x - 1,z - 1),    p - makeVert(x - 1,z));
                    glm::vec3 a3 = glm::cross(p - makeVert(x + 1,z),        p - makeVert(x,z - 1));
                    glm::vec3 a4 = glm::cross(p - makeVert(x - 1,z),        p - makeVert(x,z + 1));
                    glm::vec3 a5 = glm::cross(p - makeVert(x + 1,z + 1),    p - makeVert(x + 1,z));
                    glm::vec3 a6 = glm::cross(p - makeVert(x + 1,z + 1),    p - makeVert(x + 1,z + 1));
                    model.mesh().normals[xz(x,z)] = glm::vec3(glm::normalize(a1 + a2 + a3 + a4 + a5 + a6));
                    continue;
                }

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
        // {
        //     int z = 0;
        //     for(int x = 0; x < width; x++){
        //         glm::vec3 p = model.mesh().vertices[xz(x,z)];
        //         glm::vec3 a1 = glm::cross(p - makeHeight(x,z - 1),        p - makeHeight(x - 1,z - 1));
        //         glm::vec3 a2 = glm::cross(p - makeHeight(x - 1,z - 1),    p - makeHeight(x - 1,z));
        //         glm::vec3 a3 = glm::cross(p - makeHeight(x + 1,z),        p - makeHeight(x,z - 1));
        //         glm::vec3 a4 = glm::cross(p - makeHeight(x - 1,z),        p - makeHeight(x,z + 1));
        //         glm::vec3 a5 = glm::cross(p - makeHeight(x + 1,z + 1),    p - makeHeight(x + 1,z));
        //         glm::vec3 a6 = glm::cross(p - makeHeight(x + 1,z + 1),    p - makeHeight(x + 1,z + 1));
        //         model.mesh().normals[xz(x,z)] = glm::vec3(glm::normalize(a1 + a2 + a3 + a4 + a5 + a6));
        //     }
        // }
        model.mesh().makePoints();
        model.mesh().reloadMesh();
        model.recalcBounds();
        generated = true;
    }
};
atomic<int> terrain::terrId;