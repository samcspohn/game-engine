#include <iostream>
#include <glm/glm.hpp>
#include <vector>

#ifndef INCLUDE
#define INCLUDE
using namespace std;
void log(string log = ""){
    std::cout << log << std::endl;
}

vector<glm::vec3> getVerts(){
    vector<glm::vec3> verts;
    verts.push_back(glm::vec3(0.f,0.f,0));// 0,0
    verts.push_back(glm::vec3(.25f,.5f,0));// .5,.86
    verts.push_back(glm::vec3(.5f,.5f,0));// 1,0
    return verts;
}
#endif