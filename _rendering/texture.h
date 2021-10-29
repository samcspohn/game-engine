
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
#include "_serialize.h"
#include "editor.h"
#include "helper1.h"

using namespace std;

struct TextureMeta : public assets::asset
{
    GLuint glid = -1;
    string path;
    string _type;
    glm::ivec2 dims;
    string type();
    bool onEdit();
    TextureMeta();
    TextureMeta(string path);
    TextureMeta(string path, string type);
    void load(string path);
    void load();
    void gen(int width, int height, GLenum format = GL_RGBA, GLenum = GL_UNSIGNED_BYTE, float *data = 0);
    void write(void *data, GLenum format = GL_RGBA, GLenum = GL_UNSIGNED_BYTE);
    void read(void *data, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);
};


struct textureManager : public assets::assetManager<TextureMeta>
{
    // void encode(YAML::Node &node);
    // void decode(YAML::Node &node);
};

extern textureManager texture_manager;


struct _texture : public assets::asset_instance<TextureMeta>
{
    // TextureMeta *t = 0;
    int textureID = -1;
    TextureMeta *meta() const;
    // void namedTexture(string name);
    void load(string path);
    void setType(string type);
    void load(string path, string type);
    void _new();
    _texture() = default;
    _texture(string path);
};
_texture getNamedTexture(string name);

GLint TextureFromFile(const char *path, string directory);

bool operator<(const _texture &a, const _texture &b);

void renderEdit(const char *name, _texture &t);

// YAML_TEMPLATE(_texture,
// {
//     ENCODE_PROTO(t->path)
// },
// {

// });

namespace YAML
{
    template <>
    struct convert<_texture>
    {
        static Node encode(const _texture &rhs)
        {
            Node node;
            node = rhs.textureID;
            return node;
        }
        static bool decode(const Node &node, _texture &rhs)
        {
            rhs.textureID = node.as<int>();
            return true;
        }
    };

    template <>
    struct convert<TextureMeta>
    {
        static Node encode(const TextureMeta &rhs)
        {
            Node node;
            ENCODE_PROTO(id)
            // ENCODE_PROTO(dims)
            ENCODE_PROTO(path)
            ENCODE_PROTO(_type)
            return node;
        }
        static bool decode(const Node &node, TextureMeta &rhs)
        {
            DECODE_PROTO(id)
            // DECODE_PROTO(dims)
            DECODE_PROTO(path)
            DECODE_PROTO(_type)
            waitForRenderJob([&]()
                             { rhs.load(); });
            return true;
        }
    };
}
