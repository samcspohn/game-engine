#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//#include <assimp/Importer.hpp>
//#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Shader.h"
#include "helper1.h"
#include "texture.h"
// #include "rendering.h"
#include <map>
using namespace std;

//struct Vertex
//{
//    // Position
//    glm::vec3 Position;
//    // Normal
//    glm::vec3 Normal;
//    // TexCoords
//    glm::vec2 TexCoords;
//};

// struct Texture
// {
//     GLuint id;
//     string type;
//     aiString path;
// };



struct vertex{
    glm::vec3 position;
    glm::vec2 uv1;
    glm::vec2 uv2;
    glm::vec3 normal;
};
// int meshCounter = 0;
class Mesh
{
public:
    /*  Mesh Data  */
    vector<glm::vec3> vertices;
	vector<glm::vec2> uvs;
    vector<glm::vec2> uvs2;
	vector<glm::vec3> normals;
    vector<vertex> points;
    vector<GLuint> indices;
    texArray textures;
    bool ready = false;
    int id;
    /*  Functions  */
    // Constructor
    Mesh();
    void makePoints();
    Mesh( vector<glm::vec3> _vertices, vector<glm::vec2> _uvs, vector<glm::vec3> _normals,vector<GLuint> indices, vector<_texture> textures );
    Mesh(const Mesh& M);
    ~Mesh();
    void addTexture(_texture t);

    GLuint VAO = 0, VBO = 0, EBO = 0;
    void reloadMesh();
private:
    /*  Render data  */
    
    /*  Functions    */
    // Initializes all the buffer objects/arrays
    void setupMesh( );
};

class lightVolume{
    public:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    vector<glm::vec3> vertices;
    vector<GLuint> indices;

    void setupMesh( );
     // Render the mesh
    void Draw(size_t number);
};



