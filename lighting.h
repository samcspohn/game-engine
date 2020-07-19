#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_SWIZZLE 
#include <glm/glm.hpp>
#include "Component.h"
#include "fast_list.h"

struct pointLight{
    glm::vec3 color;
    float constant;

    float linear;
    float quadratic;
    int transfromId;
private:
    float radius;

public:
    glm::vec2 p;
    float cutOff = -1;
    float outerCutOff = -1;

    void setRadius(){
        float lightMax  = std::fmaxf(std::fmaxf(color.r, color.g), color.b);
         radius  = (-linear +  std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * lightMax))) 
         / (2 * quadratic);  
        // radius2 *= radius2;
    }
};

class lightingManager{
public:
    fast_list<pointLight> pointLights;
    gpu_vector<pointLight>* gpu_pointLights;
} plm;

namespace lighting{
    void init(){
        plm.gpu_pointLights = new gpu_vector<pointLight>();
        plm.gpu_pointLights->storage = &plm.pointLights.data;
    }
}

class Light : public component {
    typename fast_list<pointLight>::iterator pl;
public:
    void setColor(glm::vec3 col){
        pl->color = col;
    }
    void setlinear(float l){
        pl->linear = l;
        pl->setRadius();
    }
    void setQuadratic(float q){
        pl->quadratic = q;
        pl->setRadius();
    }
    void setConstant(float c){
        pl->constant = c;
        pl->setRadius();
    }
    void setOuterCutoff(float radians){
        pl->outerCutOff = glm::cos(radians);
    }
    void setInnerCutoff(float radians){
        pl->cutOff = glm::cos(radians);
    }
    void onStart(){
        pl = plm.pointLights.push_back(pointLight());
        pl->transfromId = transform->_T.index;
        
    }
    void onDestroy(){
        plm.pointLights.erase(pl);
    }
    COPY(Light);
};