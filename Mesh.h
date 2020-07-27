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

class Mesh
{
public:
    /*  Mesh Data  */
    vector<glm::vec3> vertices;
	vector<glm::vec2> uvs;
    vector<glm::vec2> uvs2;
	vector<glm::vec3> normals;
    vector<GLuint> indices;
    vector<_texture> textures;
    bool ready = false;
    /*  Functions  */
    // Constructor
    Mesh(){
        // enqueRenderJob([&]() { this->setupMesh(); });
    }
    Mesh( vector<glm::vec3> _vertices, vector<glm::vec2> _uvs, vector<glm::vec3> _normals,vector<GLuint> indices, vector<_texture> textures )
    {
        this->vertices = _vertices;
		this->uvs = _uvs;
		this->normals = _normals;
        this->indices = indices;
        this->textures = textures;
        this->setupMesh( );
    }
    Mesh(const Mesh& M){
        this->ready = false;
        this->vertices = M.vertices;
		this->uvs = M.uvs;
		this->normals = M.normals;
        this->indices = M.indices;
        this->textures = M.textures;
        this->setupMesh( );
    }
    ~Mesh(){
        glDeleteBuffers(1,&this->EBO);
        glDeleteBuffers(1,&this->VBO);
        glDeleteVertexArrays(1, &this->VAO);
    }
    // Render the mesh
    void Draw( Shader shader, /*GLuint instVBO, GLuint rotVBO,*/ size_t number)
    {
        // Bind appropriate textures
        GLuint diffuseNr = 0;
        GLuint specularNr = 0;
		GLuint normalNr = 0;
        
        for( GLuint i = 0; i < this->textures.size( ); i++ )
        {
            glActiveTexture( GL_TEXTURE0 + i ); // Active proper texture unit before binding
            // Retrieve texture number (the N in diffuse_textureN)
            stringstream ss;
            string number;
            string name = this->textures[i].t->type;
            
            if( name == "texture_diffuse" )
            {
                ss << diffuseNr++; // Transfer GLuint to stream
            }
            else if( name == "texture_specular" )
            {
                ss << specularNr++; // Transfer GLuint to stream
            }
			else if (name == "texture_normal")
			{
				ss << normalNr++; // Transfer GLuint to stream
			}
            number = ss.str( );
            // Now set the sampler to the correct texture unit
			GLint texname = glGetUniformLocation(shader.Program, ("material." + name + number).c_str());
            glUniform1i(texname, i );
            // shader.setInt("material." + name + number,i);
            // And finally bind the texture
            glBindTexture( GL_TEXTURE_2D, this->textures[i].t->id );
        }
        
        // Also set each mesh's shininess property to a default value (if you want you could extend this to another mesh property and possibly change this value)
        glUniform1f( glGetUniformLocation( shader.Program, "material.shininess" ), 16.0f );
        
        // Draw mesh
        glBindVertexArray( this->VAO );
		glDrawElementsInstanced(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0, number); // instanced
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindVertexArray( 0 );
        
        // Always good practice to set everything back to defaults once configured.
        for ( GLuint i = 0; i < this->textures.size( ); i++ )
        {
            glActiveTexture( GL_TEXTURE0 + i );
            glBindTexture( GL_TEXTURE_2D, 0 );
        }
    }
    GLuint VAO = 0, VBO = 0, EBO = 0;
    void reloadMesh(){
        enqueRenderJob([&]() { this->setupMesh(); });
    }
private:
    /*  Render data  */
    
    /*  Functions    */
    // Initializes all the buffer objects/arrays
    void setupMesh( )
    {

        // Create buffers/arrays
        if(this->VAO == 0){
            glGenVertexArrays( 1, &this->VAO );
            glGenBuffers( 1, &this->VBO );
            glGenBuffers( 1, &this->EBO );
        }
        
        glBindVertexArray( this->VAO );
        // Load data into vertex buffers
        glBindBuffer( GL_ARRAY_BUFFER, this->VBO );
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.

		int offset = 0;
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size() + sizeof(glm::vec2) * uvs.size()  + sizeof(glm::vec2) * uvs2.size() + sizeof(glm::vec3) * normals.size(), 0, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(glm::vec3) * vertices.size(), vertices.data());
		offset += sizeof(glm::vec3) * vertices.size();
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(glm::vec2) * uvs.size(), uvs.data());
		offset += sizeof(glm::vec2) * uvs.size();
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(glm::vec2) * uvs2.size(), uvs2.data());
		offset += sizeof(glm::vec2) * uvs2.size();
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(glm::vec3) * normals.size(), normals.data());

		//glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
		offset = 0;
		// postion attribute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)offset);
		offset += sizeof(glm::vec3) * vertices.size();
		// uvs attribute
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)offset);
		offset += sizeof(glm::vec2) * uvs.size();
        // uvs2 attribute
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)offset);
		offset += sizeof(glm::vec2) * uvs2.size();
		// normals
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)offset);

		// instanced
		//glGenBuffers(1, &instVBO);
		//instanceMatrixAttrib(instVBO, models, 3);

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, this->EBO );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, this->indices.size( ) * sizeof( GLuint ), &this->indices[0], GL_STATIC_DRAW );
        
        glBindVertexArray( 0 );
        ready = true;
    }
};

class lightVolume{
    public:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    vector<glm::vec3> vertices;
    vector<GLuint> indices;

    void setupMesh( )
    {
        // Create buffers/arrays
        if(this->VAO == 0){
            glGenVertexArrays( 1, &this->VAO );
            glGenBuffers( 1, &this->VBO );
            glGenBuffers( 1, &this->EBO );
        }
        
        glBindVertexArray( this->VAO );
        // Load data into vertex buffers
        glBindBuffer( GL_ARRAY_BUFFER, this->VBO );
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
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)offset);
		
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, this->EBO );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, this->indices.size( ) * sizeof( GLuint ), &this->indices[0], GL_STATIC_DRAW );
        
        glBindVertexArray( 0 );
        
    }
     // Render the mesh
    void Draw(size_t number)
    {
        // Draw mesh
        glBindVertexArray( this->VAO );
		glDrawElementsInstanced(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0, number); // instanced
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindVertexArray( 0 );
    }
};


