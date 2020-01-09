#include "Model.h"
#include "rendering.h"
#include "concurrency.h"
#include <map>

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
map<int,component_ref(terrain)> terrains;

class terrain : public component{
public:
    COPY(terrain);
    component_ref(_renderer) r;
    int width;
    int depth;
    bool generated = false;
    void onStart(){
        terrains.insert(std::pair(0, transform->gameObject->getComponent<terrain>()));
        
    }
    void update(){
        _model model = r->getModel();
        if(!generated && model.m != 0 && model.m->model->ready)
            generate(width,depth);
        // model.m->model->meshes[0].reloadMesh();
    }
    UPDATE(terrain,update);

    int xz(int x, int z){
        return x * width + z;
    }
    vector<vector<float> >heightMap;
    float getHeight(float x, float z){
        if(!generated)
        return -INFINITY;

        x -= transform->getPosition().x;
        z -= transform->getPosition().z;
        x /= transform->getScale().x;
        z /= transform->getScale().z;
        if(x > width - 1 ||z > width - 1 || x < 0 || z < 0)
            return -INFINITY;
        // return 1.f;

        float xr = fmod(x,1.f);
        float yr = fmod(z,1.f);
        float a1 = heightMap[(int)x][(int)z];
        float a2 = heightMap[(int)x + 1][(int)z];
        float a3 = heightMap[(int)x][(int)z + 1];
        float a4 = heightMap[(int)x + 1][((int)z + 1)];
        float x1 = a1 * (1 - xr) + a2 * (xr);
        float x2 = a3 * (1 - xr) + a4 * (xr);
        float h = (x1 * (1 - yr) + x2 * (yr)) * transform->getScale().y + transform->getPosition().y;
        return h;
    }
    
    void generate(int _width, int _depth){
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



        _model model = r->getModel();
        model.m->model->meshes[0].vertices = vector<glm::vec3>(_width * _depth);

        vector<glm::vec3>& verts = model.m->model->meshes[0].vertices;

        for(int x = 0; x < width; x++){
            for(int z = 0; z < depth; z++){
                verts[x * width + z] = glm::vec3(x,heightMap[x][z], z);
            }
        }

        model.m->model->meshes[0].indices = vector<GLuint>((width - 1) * (depth - 1) * 6);
        int k = 0;
        for(int i = 0; i < width - 1; i++){
            for(int j = 0; j < depth - 1; j++){

            
            model.m->model->meshes[0].indices[k++] = xz(i,j);
            model.m->model->meshes[0].indices[k++] = xz(i,j + 1);
            model.m->model->meshes[0].indices[k++] = xz(i + 1,j + 1);
            model.m->model->meshes[0].indices[k++] = xz(i,j);
            model.m->model->meshes[0].indices[k++] = xz(i + 1,j + 1);
            model.m->model->meshes[0].indices[k++] = xz(i + 1,j);
            }
        }
        model.m->model->meshes[0].normals = vector<glm::vec3>(_width * _depth);
        for(int x = 1; x < width - 1; x++){
            for(int z = 1; z < depth - 1; z++){
                glm::vec3 p = model.m->model->meshes[0].vertices[xz(x,z)];
                glm::vec3 a1 = glm::cross(p - model.m->model->meshes[0].vertices[xz(x,z - 1)],        p - model.m->model->meshes[0].vertices[xz(x - 1,z - 1)]);
                glm::vec3 a2 = glm::cross(p - model.m->model->meshes[0].vertices[xz(x - 1,z - 1)],    p - model.m->model->meshes[0].vertices[xz(x - 1,z)]);
                glm::vec3 a3 = glm::cross(p - model.m->model->meshes[0].vertices[xz(x + 1,z)],        p - model.m->model->meshes[0].vertices[xz(x,z - 1)]);
                glm::vec3 a4 = glm::cross(p - model.m->model->meshes[0].vertices[xz(x - 1,z)],        p - model.m->model->meshes[0].vertices[xz(x,z + 1)]);
                glm::vec3 a5 = glm::cross(p - model.m->model->meshes[0].vertices[xz(x + 1,z + 1)],    p - model.m->model->meshes[0].vertices[xz(x + 1,z)]);
                glm::vec3 a6 = glm::cross(p - model.m->model->meshes[0].vertices[xz(x + 1,z + 1)],    p - model.m->model->meshes[0].vertices[xz(x + 1,z + 1)]);
                model.m->model->meshes[0].normals[xz(x,z)] = glm::vec3(glm::normalize(a1 + a2 + a3 + a4 + a5 + a6));
            }
        }
        // model.m->model->meshes.push_back(Mesh());
        // model.m->model->meshes[1].vertices = vector<glm::vec3>(width * depth);
        // vector<glm::vec3>& verts2 = model.m->model->meshes[1].vertices;

        // for(int x = 0; x < width; x++){
        //     for(int z = 0; z < depth; z++){
        //         verts[x * width + z] = glm::vec3(x,heightMap[x][z], z + width);
        //     }
        // }

        // model.m->model->meshes[0].indices = vector<GLuint>((width - 1) * (depth - 1) * 6);
        // int k = 0;
        // for(int i = 0; i < width - 1; i++){
        //     for(int j = 0; j < depth - 1; j++){

            
        //     model.m->model->meshes[1].indices[k++] = xz(i,j);
        //     model.m->model->meshes[1].indices[k++] = xz(i,j + 1);
        //     model.m->model->meshes[1].indices[k++] = xz(i + 1,j + 1);
        //     model.m->model->meshes[1].indices[k++] = xz(i,j);
        //     model.m->model->meshes[1].indices[k++] = xz(i + 1,j + 1);
        //     model.m->model->meshes[1].indices[k++] = xz(i + 1,j);
        //     }
        // }
        // model.m->model->meshes[1].normals = vector<glm::vec3>(_width * _depth);
        // for(int x = 1; x < width - 1; x++){
        //     for(int z = 1; z < depth - 1; z++){
        //         glm::vec3 p = model.m->model->meshes[1].vertices[xz(x,z)];
        //         glm::vec3 a1 = glm::cross(p - model.m->model->meshes[1].vertices[xz(x,z - 1)],        p - model.m->model->meshes[1].vertices[xz(x - 1,z - 1)]);
        //         glm::vec3 a2 = glm::cross(p - model.m->model->meshes[1].vertices[xz(x - 1,z - 1)],    p - model.m->model->meshes[1].vertices[xz(x - 1,z)]);
        //         glm::vec3 a3 = glm::cross(p - model.m->model->meshes[1].vertices[xz(x + 1,z)],        p - model.m->model->meshes[1].vertices[xz(x,z - 1)]);
        //         glm::vec3 a4 = glm::cross(p - model.m->model->meshes[1].vertices[xz(x - 1,z)],        p - model.m->model->meshes[1].vertices[xz(x,z + 1)]);
        //         glm::vec3 a5 = glm::cross(p - model.m->model->meshes[1].vertices[xz(x + 1,z + 1)],    p - model.m->model->meshes[1].vertices[xz(x + 1,z)]);
        //         glm::vec3 a6 = glm::cross(p - model.m->model->meshes[1].vertices[xz(x + 1,z + 1)],    p - model.m->model->meshes[1].vertices[xz(x + 1,z + 1)]);
        //         model.m->model->meshes[1].normals[xz(x,z)] = glm::vec3(glm::normalize(a1 + a2 + a3 + a4 + a5 + a6));
        //     }
        // }
        // model.m->model->meshes[1].reloadMesh();
        model.m->model->meshes[0].reloadMesh();
        generated = true;
    }
};