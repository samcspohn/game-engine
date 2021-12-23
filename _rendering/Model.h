#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// #include <SOIL2.h>
// #include "SOIL2/SOIL2.h"
#include <SOIL/SOIL.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"
#include "helper1.h"
#include <string>
#include "serialize.h"
#include <memory>

using namespace std;

GLint TextureFromFile( const char *path, string directory );

class Model
{
public:
	string modelPath;
    bool loaded = false;
	bool ready();
  
    Model(string path);
    Model(const Model& M);
    Model();
    ~Model();
    
	int numVerts=0;
    vector<Mesh*> meshes;
    void loadModel();
private:
  
    string directory;
    void loadModel( string path );
    
    // Processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode( aiNode* node, const aiScene* scene );
    
    Mesh *processMesh( aiMesh *mesh, const aiScene *scene );
    
    // Checks all material textures of a given type and loads the textures if they're not loaded yet.
    // The required info is returned as a Texture struct.
    vector<_texture> loadMaterialTextures( aiMaterial *mat, aiTextureType type, string typeName );
};
namespace YAML
{
	template <>
	struct convert<Model>
	{
		static Node encode(const Model &rhs)
		{
			Node node;
			node["file"] = rhs.modelPath;
			return node;
		}

		static bool decode(const Node &node, Model &rhs)
		{
			rhs.modelPath = node["file"].as<string>();
            waitForRenderJob([&](){
                rhs.loadModel();
            })
			return true;
		}
	};
}