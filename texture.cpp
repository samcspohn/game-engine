
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

using namespace std;

namespace textureManager
{
    map<string,TextureMeta*> textures;
    
} // namespace textureManager

void _texture::load(string path){
    auto tm = textureManager::textures.find(path);
    if(tm != textureManager::textures.end()){
        this->t = tm->second;
    }
    else{
        textureManager::textures.insert(std::pair<string,TextureMeta*>(path, new TextureMeta()));
        textureManager::textures.at(path)->load(path);
        this->t = textureManager::textures.at(path);
    }
}

void _texture::namedTexture(string name){
    textureManager::textures.insert(std::pair<string,TextureMeta*>(name, new TextureMeta()));
    this->t = textureManager::textures.at(name);

}

void _texture::load(string path, string type){
    auto tm = textureManager::textures.find(path);
    if(tm != textureManager::textures.end()){
        this->t = tm->second;
    }
    else{
        textureManager::textures.insert(std::pair<string,TextureMeta*>(path, new TextureMeta(path,type)));
        this->t = textureManager::textures.at(path);
    }
}

void _texture::setType(string type){
    this->t->type = type;
}
TextureMeta::TextureMeta(){}
TextureMeta::TextureMeta( string path, string type){
    //Generate texture ID and load texture data
    glGenTextures( 1, &id );
    
    unsigned char *image = SOIL_load_image( path.c_str( ), &dims.x, &dims.y, 0, SOIL_LOAD_RGBA );
    
    // Assign texture to ID
    glBindTexture( GL_TEXTURE_2D, id );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, dims.x, dims.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );
    glGenerateMipmap( GL_TEXTURE_2D );
    
    // Parameters
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture( GL_TEXTURE_2D, 0 );
    SOIL_free_image_data( image );

    this->type = type;
    this->path = path;
}
void TextureMeta::load(string path){
    glGenTextures( 1, &id );
    
    unsigned char *image = SOIL_load_image( path.c_str( ), &dims.x, &dims.y, 0, SOIL_LOAD_RGBA );
    
    // Assign texture to ID
    glBindTexture( GL_TEXTURE_2D, id );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, dims.x, dims.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );
    glGenerateMipmap( GL_TEXTURE_2D );
    
    // Parameters
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture( GL_TEXTURE_2D, 0 );
    SOIL_free_image_data( image );
}

void TextureMeta::gen(int width, int height){
    // Assign texture to ID
    glGenTextures( 1, &id );
    
    dims.x = width;
    dims.y = height;
    glBindTexture( GL_TEXTURE_2D, id );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, dims.x, dims.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
    glGenerateMipmap( GL_TEXTURE_2D );
    
    // Parameters
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture( GL_TEXTURE_2D, 0 );
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