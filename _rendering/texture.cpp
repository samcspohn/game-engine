
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
#include "imgui/imgui.h"

using namespace std;

// namespace textureManager
// {
// map<string, int> textures;
// unordered_map<int, shared_ptr<TextureMeta>> textures_ids;
//     void textureManager::encode(YAML::Node &node)
//     {
//         YAML::Node textures_ids_node;
//         // YAML::Node models_node;
//         for (auto &i : textures_ids)
//         {
//             textures_ids_node.force_insert(i.first,*i.second);
//         }
//         node["textures_ids"] = textures_ids_node;
//         node["textures"] = textures;
//     }

//     void decode(YAML::Node &node)
//     {

//         // models_id = node["models_id"].as<map<int, shared_ptr<_modelMeta>>>();
//         for (YAML::const_iterator i = node["textures_ids"].begin(); i != node["textures_ids"].end(); ++i)
//         {
//             textures_ids[i->first.as<int>()] = make_shared<TextureMeta>(i->second.as<TextureMeta>());
//         }
//         textures = node["textures"].as<map<string, int>>();

//         // waitForRenderJob([&]()
//         //                  {
// 		// 	for (auto &t : textures_ids)
// 		// 	{
// 		// 		t.second->load();
// 		// 	} });
//     }
// } // namespace textureManager

textureManager texture_manager __attribute__((init_priority(2000)));
;

void _texture::load(string path)
{
    auto tm = texture_manager.path.find(path);

    if (tm == texture_manager.path.end())
    {
        auto texMet = make_shared<TextureMeta>(path);
        texMet->genID();
        texture_manager.meta.emplace(texMet->id, texMet);
        texture_manager.path.emplace(path, texMet->id);
        tm = texture_manager.path.find(path);
        texMet->load(path);
    }
    textureID = tm->second;
}
TextureMeta *_texture::meta() const
{
    try
    {
        if (textureID != -1)
            return texture_manager.meta.at(textureID).get();
        else
            return 0;
    }
    catch (...)
    {
        return 0;
    }
}

// void _texture::namedTexture(string name)
// {
//     if (textureManager::textures.find(name) == textureManager::textures.end())
//         textureManager::textures.emplace(name, make_shared<TextureMeta>());
//     meta()->t = textureManager::textures.at(name).get();
// }

void _texture::load(string path, string type)
{
    auto tm = texture_manager.path.find(path);

    if (tm == texture_manager.path.end())
    {
        auto texMet = make_shared<TextureMeta>(path, type);
        texMet->genID();
        texture_manager.meta.emplace(texMet->id, texMet);
        texture_manager.path.emplace(path, texMet->id);
        tm = texture_manager.path.find(path);
        texMet->load(path);
    }
    textureID = tm->second;
}

void _texture::setType(string type)
{
    meta()->_type = type;
}

_texture::_texture(string path)
{
    this->load(path);
}
void _texture::_new()
{

    auto texMet = make_shared<TextureMeta>();
    texMet->genID();
    texture_manager.meta.emplace(texMet->id, texMet);
    // textureManager::textures.emplace(path, texMet->id);
    // texMet->load(path);
    textureID = texMet->id;
}

string TextureMeta::type()
{
    return "TEXTURE";
}
bool TextureMeta::onEdit()
{
    bool b = false;
    ImGui::Checkbox("uhh", &b);
    return true;
}

TextureMeta::TextureMeta() {}

TextureMeta::TextureMeta(string path)
{
    // Generate texture ID and load texture data

    unsigned char *image = SOIL_load_image(path.c_str(), &dims.x, &dims.y, 0, SOIL_LOAD_RGBA);

    // enqueRenderJob([=]() {
    glGenTextures(1, &glid);
    // Assign texture to ID
    glBindTexture(GL_TEXTURE_2D, glid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dims.x, dims.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    SOIL_free_image_data(image);
    this->path = path;
}

TextureMeta::TextureMeta(string path, string type)
{
    new (this) TextureMeta(path);
    this->_type = type;
}
void TextureMeta::load(string path)
{
    this->path = path;
    this->load();
}
void TextureMeta::load()
{
    unsigned char *image = SOIL_load_image(path.c_str(), &dims.x, &dims.y, 0, SOIL_LOAD_RGBA);

    // enqueRenderJob([=]() {
    glGenTextures(1, &glid);
    // Assign texture to ID
    glBindTexture(GL_TEXTURE_2D, glid);
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
    if (glid == -1)
    {
        glDeleteTextures(1, &glid);
    }

    dims.x = width;
    dims.y = height;
    glGenTextures(1, &glid);
    glBindTexture(GL_TEXTURE_2D, glid);
    glTexImage2D(GL_TEXTURE_2D, 0, (format == GL_RED ? GL_R32F : format), dims.x, dims.y, 0, format, type, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 7);
    // Parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureMeta::write(void *data, GLenum format, GLenum type)
{
    glBindTexture(GL_TEXTURE_2D, glid);
    glTexImage2D(GL_TEXTURE_2D, 0, (format == GL_RED ? GL_R32F : format), dims.x, dims.y, 0, format, type, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Parameters
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 7);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void TextureMeta::read(void *data, GLenum format, GLenum type)
{
    glBindTexture(GL_TEXTURE_2D, glid);
    glGetTexImage(GL_TEXTURE_2D, 0, format, type, data);
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

bool operator<(const _texture &a, const _texture &b)
{
    return a.textureID < b.textureID;
}

// if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
// {
// 	ImGui::SetDragDropPayload("TRANSFORM_DRAG_AND_DROP", &t.id, sizeof(int));
// 	ImGui::EndDragDropSource();
// }

void renderEdit(const char *name, _texture &t)
{
    ImGui::InputText(name, t.meta()->path.c_str(), 1, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("FILE_DRAG_AND_DROP.png"))
        {
            // IM_ASSERT(payload->DataSize == sizeof(int));
            // int size = payload->DataSize;
            char *payload_n = (char *)payload->Data;
            t.load(string(payload_n));
        }
        ImGui::EndDragDropTarget();
    }
}