#include "lighting.h"
// struct pointLight
// {
//     glm::vec3 color;
//     float constant;

//     float linear;
//     float quadratic;
//     int transfromId;

// private:
//     float radius;

// public:
//     glm::vec2 p;
//     float cutOff = -1;
//     float outerCutOff = -1;

    void pointLight::setRadius()
    {
        float lightMax = std::fmaxf(std::fmaxf(color.r, color.g), color.b);
        radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * lightMax))) / (2 * quadratic);
        // radius2 *= radius2;
    }
//     SER_HELPER()
//     {
//         ar &color &constant &linear &quadratic &transfromId &radius &cutOff &outerCutOff;
//     }
// };

namespace lightingManager
{
    fast_list<pointLight> pointLights;
    gpu_vector<pointLight> *gpu_pointLights;
    void save(OARCHIVE &oa)
    {
        oa << pointLights;
    }
    void load(IARCHIVE &ia)
    {
        ia >> pointLights;
    }
}; // namespace lightingManager

namespace lighting
{
    void init()
    {
        lightingManager::gpu_pointLights = new gpu_vector<pointLight>();
        lightingManager::gpu_pointLights->storage = &lightingManager::pointLights.data;
    }
} // namespace lighting

// class Light : public component
// {
//     typename fast_list<pointLight>::iterator pl;

// public:
    Light::Light() = default;
    Light::Light(const Light& l){
        pl = 0;
    }
    void Light::setColor(glm::vec3 col)
    {
        pl->color = col;
    }
    void Light::setlinear(float l)
    {
        pl->linear = l;
        pl->setRadius();
    }
    void Light::setQuadratic(float q)
    {
        pl->quadratic = q;
        pl->setRadius();
    }
    void Light::setConstant(float c)
    {
        pl->constant = c;
        pl->setRadius();
    }
    void Light::setOuterCutoff(float radians)
    {
        pl->outerCutOff = glm::cos(radians);
    }
    void Light::setInnerCutoff(float radians)
    {
        pl->cutOff = glm::cos(radians);
    }
    void Light::onStart()
    {
        if(pl.isNull())
            pl = lightingManager::pointLights.push_back(pointLight());
        pl->transfromId = transform.id;
    }
    void Light::onDestroy()
    {
        lightingManager::pointLights.erase(pl);
    }
    void Light::onEdit()
    {
        renderEdit("color", pl->color);
        if (ImGui::DragFloat("linear", &pl->linear))
        {
            pl->setRadius();
        }
        if (ImGui::DragFloat("quadratic", &pl->quadratic))
        {
            pl->setRadius();
        }
        if (ImGui::DragFloat("constant", &pl->constant))
        {
            pl->setRadius();
        }
        float angle = glm::degrees(pl->cutOff);
        if (ImGui::DragFloat("cutoff", &angle))
        {
            pl->cutOff = glm::radians(angle);
        }
        angle = glm::degrees(pl->outerCutOff);
        if (ImGui::DragFloat("cutoff", &pl->outerCutOff))
        {
            pl->outerCutOff = glm::radians(angle);
        }
    }
    // COPY(Light);
    // SER_HELPER(){
    //     // SER_BASE(component)
    //     // ar & pl.itr->it;
    // }
    // SER_OUT(){
    //     ar << boost::serialization::base_object<component>(*this);
    //     ar << pl.itr->it;
    // }
    // SER_IN(){
    //     unsigned int id;
    //     ar >> boost::serialization::base_object<component>(*this);;
    //     ar >> id;
    //     pl = lightingManager::pointLights.iterators[id];
    // }
//     // SER1(pl);
// };
REGISTER_COMPONENT(Light)