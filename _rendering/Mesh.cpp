#include "Mesh.h"
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

Mesh::Mesh()
{
    // enqueRenderJob([&]() { this->setupMesh(); });
    // id = meshCounter++;
}
void Mesh::makePoints()
{
    points.resize(vertices.size());
    int p = 0;
    for (auto &i : vertices)
    {
        points[p].position = i;
        p++;
    }
    p = 0;
    for (auto &i : uvs)
    {
        points[p].uv1 = i;
        p++;
    }
    p = 0;
    for (auto &i : uvs2)
    {
        points[p].uv2 = i;
        p++;
    }
    p = 0;
    for (auto &i : normals)
    {
        points[p].normal = i;
        p++;
    }
}
Mesh::Mesh(vector<glm::vec3> _vertices, vector<glm::vec2> _uvs, vector<glm::vec3> _normals, vector<GLuint> indices, vector<_texture> textures)
{
    this->vertices = _vertices;
    this->uvs = _uvs;
    this->normals = _normals;
    this->indices = indices;
    this->makePoints();
    this->textures.setTextures(textures);
    this->setupMesh();
    // id = meshCounter++;
}
Mesh::Mesh(const Mesh &M)
{
    this->ready = false;
    this->vertices = M.vertices;
    this->uvs = M.uvs;
    this->uvs2 = M.uvs2;
    this->normals = M.normals;
    this->indices = M.indices;
    this->makePoints();
    this->textures = M.textures;
    this->setupMesh();
    // id = meshCounter++;
}
Mesh::~Mesh()
{
    if (this->VAO != 0)
    {
        glDeleteBuffers(1, &this->EBO);
        glDeleteBuffers(1, &this->VBO);
        glDeleteVertexArrays(1, &this->VAO);
    }
}
void Mesh::addTexture(_texture t)
{
    this->textures.push_back(t);
}

void Mesh::reloadMesh()
{
    this->setupMesh();
}

void Mesh::setupMesh()
{
    // enqueRenderJob([&]() {
        // Create buffers/arrays
        if (this->VAO == 0)
        {
            glGenVertexArrays(1, &this->VAO);
            glGenBuffers(1, &this->VBO);
            glGenBuffers(1, &this->EBO);
        }

        glBindVertexArray(this->VAO);
        // Load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.

        int offset = 0;
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * points.size(), points.data(), GL_STATIC_DRAW);

        //glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        offset = 0;
        // postion attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid *)offset);
        offset += sizeof(glm::vec3);
        // uvs attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid *)offset);
        offset += sizeof(glm::vec2);
        // uvs2 attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid *)offset);
        offset += sizeof(glm::vec2);
        // normals
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid *)offset);

        // instanced
        //glGenBuffers(1, &instVBO);
        //instanceMatrixAttrib(instVBO, models, 3);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), this->indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
        ready = true;
    // });
}

void lightVolume::setupMesh()
{
    // Create buffers/arrays
    if (this->VAO == 0)
    {
        glGenVertexArrays(1, &this->VAO);
        glGenBuffers(1, &this->VBO);
        glGenBuffers(1, &this->EBO);
    }

    glBindVertexArray(this->VAO);
    // Load data into vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    // A great thing about structs is that their memory layout is sequential for all its items.
    // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
    // again translates to 3/2 floats which translates to a byte array.

    int offset = 0;
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(glm::vec3) * vertices.size(), vertices.data());

    //glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    offset = 0;
    // postion attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)offset);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);
}
// Render the mesh
void lightVolume::Draw(size_t number)
{
    // Draw mesh
    glBindVertexArray(this->VAO);
    glDrawElementsInstanced(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0, number); // instanced
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
