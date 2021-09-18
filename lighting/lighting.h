#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include "components/Component.h"
#include "fast_list.h"
#include "editor.h"
#include "components/game_object.h"

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
    Light(const Light &l);
    ~Light();
    void deinit(int i);
    void init(int i);
    void setColor(glm::vec3 col);
    void setlinear(float l);
    void setQuadratic(float q);
    void setConstant(float c);
    void setOuterCutoff(float radians);
    void setInnerCutoff(float radians);
    // void onStart();
    // void onDestroy();
    void onEdit();
    SER_FUNC()
    {
        switch (x)
        {
            // &color &constant &linear &quadratic &transfromId &radius &cutOff &outerCutOff;
        case ser_mode::edit_mode:
            renderEdit("color", pl->color);
            renderEdit("constant", pl->constant);
            renderEdit("linear", pl->linear);
            renderEdit("quadratic", pl->quadratic);
            renderEdit("cut off", pl->cutOff);
            renderEdit("outer cut off", pl->outerCutOff);
            pl->setRadius();
            break;
        case ser_mode::read_mode:
            (*_iar) >> *pl;
            break;
        case ser_mode::write_mode:
            (*_oar) << *pl;
            break;
        default:
            cout << "no mode provided";
            break;
        }
    }
};
// REGISTER_COMPONENT(Light)