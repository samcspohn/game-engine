#include "Model.h"
#include "rendering.h"
#include "concurrency.h"

class terrain : public component{
public:
    COPY(terrain);
    component_ref(_renderer) r;
    int width;
    int depth;
    bool generated = false;
    void update(){
        _model model = r->getModel();
        if(!generated && model.m != 0 && model.m->model->ready)
            generate(width,depth);
            
    }
    UPDATE(terrain,update);
    int xz(int x, int z){
        return x * width + z;
    }
    // void getHeight(int x1, int z1, int x2, int z2, int level){
    //     if((float)level >= log2((float)width) + 1){
    //         cout << level << endl;
    //         return;
    //     }
    //     _model model = r->getModel();
    //     float h = model.m->model->meshes[0].vertices[xz(x1,z1)].y + model.m->model->meshes[0].vertices[xz(x2,z2)].y + model.m->model->meshes[0].vertices[xz(x1,z2)].y + model.m->model->meshes[0].vertices[xz(x2,z1)].y;
    //     h /= 4;
    //     h += (randf() - .5f) * 100 / powf(2.f,(float)level);
    //     int xm = (x1 + x2) / 2;
    //     int zm = (z1 + z2) / 2;
    //     model.m->model->meshes[0].vertices[xz(xm,zm)].y = h;
    //     // if(xm > 0 && xm < width && zm > 0 && zm < width){
    //     //     model.m->model->meshes[0].vertices[xz(xm - 1,zm - 1)].y = h;
    //     //     model.m->model->meshes[0].vertices[xz(xm - 1,zm + 1)].y = h;    
    //     //     model.m->model->meshes[0].vertices[xz(xm + 1,zm - 1)].y = h;
    //     //     model.m->model->meshes[0].vertices[xz(xm + 1,zm + 1)].y = h;
    //     // }

    //     getHeight(x1, z1, xm, zm, level + 1);
    //     getHeight(xm, z1, x2, zm, level + 1);
    //     getHeight(x1, zm, xm, z2, level + 1);
    //     getHeight(xm, zm, x2, z2, level + 1);
    // }

    void generate(int _width, int _depth){
        width = _width;
        depth = _depth;
        _model model = r->getModel();
        model.m->model->meshes[0].vertices = vector<glm::vec3>(_width * _depth);

        vector<glm::vec3>& verts = model.m->model->meshes[0].vertices;
        for(int x = 0; x < width; x++){
            for(int z = 0; z < depth; z++){
                model.m->model->meshes[0].vertices[xz(x,z)] = glm::vec3(x, randf() * 20.f, z);
            }
        }
        for(int smoothing = 0; smoothing < 4; smoothing++){
            for(int x = 1; x < width - 1; x++){
                for(int z = 1; z < depth - 1; z++){
                    verts[xz(x,z)] = glm::vec3(x, (verts[xz(x,z)].y
                    + verts[xz(x - 1,z - 1)].y
                    + verts[xz(x - 1,z + 1)].y
                    + verts[xz(x + 1,z - 1)].y
                    + verts[xz(x + 1,z + 1)].y

                    + verts[xz(x,z - 1)].y
                    + verts[xz(x,z + 1)].y
                    + verts[xz(x - 1,z)].y
                    + verts[xz(x + 1,z)].y) / 9, z);
                }
            }
        }

        for(auto& a : verts){
            a *= 10;
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
        model.m->model->meshes[0].reloadMesh();
        generated = true;
    }
};