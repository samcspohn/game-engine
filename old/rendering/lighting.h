#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include "Component.h"
#include "fast_list.h"
#include "editor.h"
#include "game_object.h"

struct pointLight
{
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

    void setRadius();
    SER_HELPER()
    {
        ar &color &constant &linear &quadratic &transfromId &radius &cutOff &outerCutOff;
    }
};

namespace lightingManager
{
    extern fast_list<pointLight> pointLights;
    extern gpu_vector<pointLight> *gpu_pointLights;
    // void save(OARCHIVE &oa);
    // void load(IARCHIVE &ia);
}; // namespace lightingManager

namespace lighting
{
    void init();
} // namespace lighting

class Light : public component
{
    typename fast_list<pointLight>::iterator pl;

public:
    Light();
    void init();
    Light(const Light& l);
    ~Light();
    void deinit();
    void setColor(glm::vec3 col);
    void setlinear(float l);
    void setQuadratic(float q);
    void setConstant(float c);
    void setOuterCutoff(float radians);
    void setInnerCutoff(float radians);
    // void onStart();
    // void onDestroy();
    void onEdit();
    COPY(Light);
    SER_HELPER(){
        // SER_BASE(component)
        // ar & pl.itr->it;
    }
    SER_OUT(){
        ar << boost::serialization::base_object<component>(*this);
        ar << (*pl);
    }
    SER_IN(){
        // unsigned int id;
        ar >> boost::serialization::base_object<component>(*this);;
        ar >> (*pl);
        // ar >> id;
        // pl = lightingManager::pointLights.iterators[id];
    }
    // SER1(pl);
};
// REGISTER_COMPONENT(Light)