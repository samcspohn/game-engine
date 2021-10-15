
#include "texture.h"
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
#include <map>
#include "helper1.h"

using namespace std;

namespace textureManager
{
    map<string, shared_ptr<TextureMeta>> textures;

} // namespace textureManager

void _texture::load(string path)
{
    auto tm = textureManager::textures.find(path);
    if (tm != textureManager::textures.end())
    {
        this->t = tm->second.get();
    }
    else
    {
        textureManager::textures.emplace(path, make_shared<TextureMeta>());
        textureManager::textures.at(path)->load(path);
        this->t = textureManager::textures.at(path).get();
    }
}

void _texture::namedTexture(string name)
{
    if (textureManager::textures.find(name) == textureManager::textures.end())
        textureManager::textures.emplace(name, make_shared<TextureMeta>());
    this->t = textureManager::textures.at(name).get();
}

void _texture::load(string path, string type)
{
    auto tm = textureManager::textures.find(path);
    if (tm != textureManager::textures.end())
    {
        this->t = tm->second.get();
    }
    else
    {
        textureManager::textures.emplace(path, make_shared<TextureMeta>(path, type));
        this->t = textureManager::textures.at(path).get();
    }
}

void _texture::setType(string type)
{
    this->t->type = type;
}

_texture getNamedTexture(string name)
{
    _texture t;
    t.namedTexture(name);
    return t;
}

TextureMeta::TextureMeta() {}
TextureMeta::TextureMeta(string path, string type)
{
    //Generate texture ID and load texture data

    unsigned char *image = SOIL_load_image(path.c_str(), &dims.x, &dims.y, 0, SOIL_LOAD_RGBA);

    // enqueRenderJob([=]() {
        glGenTextures(1, &id);
        // Assign texture to ID
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dims.x, dims.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        SOIL_free_image_data(image);
    // });

    this->type = type;
    this->path = path;
}
void TextureMeta::load(string path)
{

    unsigned char *image = SOIL_load_image(path.c_str(), &dims.x, &dims.y, 0, SOIL_LOAD_RGBA);

// enqueRenderJob([=]() {
    glGenTextures(1, &id);
    // Assign texture to ID
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dims.x, dims.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    SOIL_free_image_data(image);

// });
}

void TextureMeta::gen(int width, int height, GLenum format, GLenum type, float *data = 0)
{
    // Assign texture to ID
    if(id == -1){
        glDeleteTextures(1,&id);
    }

    dims.x = width;
    dims.y = height;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, (format == GL_RED ? GL_R32F : format), dims.x, dims.y, 0, format, type, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureMeta::write(void *data, GLenum format, GLenum type)
{
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, (format == GL_RED ? GL_R32F : format), dims.x, dims.y, 0, format, type, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void TextureMeta::read(void* data, GLenum format, GLenum type){
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage( GL_TEXTURE_2D, 0,format, type, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// GLint TextureFromFile( const char *path, string directory )
// {
//     //Generate texture ID and load texture data
//     string filename = string( path );
//     filename = directory + '/' + filename;
//     GLuint textureID;
//     glGenTextures( 1, &textureID );

//     int width, height;

//     unsigned char *image = SOIL_load_image( filename.c_str( ), &width, &height, 0, SOIL_LOAD_RGBA );

//     // Assign texture to ID
//     glBindTexture( GL_TEXTURE_2D, textureID );
//     glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );
//     glGenerateMipmap( GL_TEXTURE_2D );

//     // Parameters
//     glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
//     glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
//     glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
//     glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glBindTexture( GL_TEXTURE_2D, 0 );
//     SOIL_free_image_data( image );

//     return textureID;
// }