
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

struct Texture
{
    GLuint id;
    string type;
    aiString path;
    glm::ivec2 dims;
    Texture();
    Texture( string path, aiTextureType type, string typeName);
    void load(string path);
};


GLint TextureFromFile( const char *path, string directory );