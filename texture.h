
#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// #include <SOIL2.h>
// #include "SOIL2/SOIL2.h"
#include <SOIL/SOIL.h>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;

struct TextureMeta
{
    GLuint id;
    aiString path;
    string type;
    glm::ivec2 dims;
    TextureMeta();
    TextureMeta( string path, string type);
    void load(string path);
};

struct _texture{
    TextureMeta* t;
    void load(string path);
    void setType(string type);
    void load(string path, string type);
};

GLint TextureFromFile( const char *path, string directory );