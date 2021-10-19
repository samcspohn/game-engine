
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
    GLuint id = -1;
    aiString path;
    string type;
    glm::ivec2 dims;
    TextureMeta();
    TextureMeta( string path, string type);
    void load(string path);
    void gen(int width, int height, GLenum format = GL_RGBA, GLenum = GL_UNSIGNED_BYTE,  float* data = 0);
    void write(void* data, GLenum format = GL_RGBA, GLenum = GL_UNSIGNED_BYTE);
    void read(void* data, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);
};

struct _texture{
    TextureMeta* t = 0;
    void namedTexture(string name);
    void load(string path);
    void setType(string type);
    void load(string path, string type);
};
_texture getNamedTexture(string name);

GLint TextureFromFile( const char *path, string directory );

bool operator<(const _texture& a, const _texture& b);

void renderEdit(const char* name, _texture& t);