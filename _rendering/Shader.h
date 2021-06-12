#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <GL/glew.h>
#include <map>
#include <functional>
#include <bits/stdc++.h>
#include "helper1.h"
#include "Shaderinclude.h"
#include "texture.h"
#include "serialize.h"
#include "../console.h"

using namespace std;

// struct shaderVariable {
// 	int type = -1;
// 	union {
// 		//constants
// 		GLfloat f;
// 		GLint i;
// 		GLuint ui;
// 		glm::vec3 v3;
// 		glm::vec2 v2;
// 		glm::vec4 v4;
// 		glm::mat4 m4;
// 		glm::mat3 m3;

// 		//references
// 		GLfloat* fp;
// 		GLint* ip;
// 		GLuint* uip;
// 		glm::vec3* v3p;
// 		glm::vec2* v2p;
// 		glm::vec4* v4p;
// 		glm::mat4* m4p;
// 		glm::mat3* m3p;

// 	};
// 	shaderVariable operator=(int _i) {
// 		if (type != 1 || type != -1)
// 			throw "mismatched types";
// 		type = 1;
// 		i = _i;
// 	}
// 	shaderVariable operator=(float _f) {
// 		type = 0;
// 		f = _f;
// 	}
// 	shaderVariable operator=(unsigned int _ui) {
// 		type = 2;
// 		ui = _ui;
// 	}
// 	shaderVariable operator=(glm::vec3 _v3) {
// 		type = 3;
// 		v3 = _v3;
// 	}
// 	shaderVariable operator=(glm::vec2 _v2) {
// 		type = 4;
// 		v2 = _v2;
// 	}
// 	shaderVariable operator=(glm::vec4 _v4) {
// 		type = 5;
// 		v4 = _v4;
// 	}
// 	shaderVariable operator=(glm::mat4 _m4) {
// 		type = 6;
// 		m4 = _m4;
// 	}
// 	shaderVariable operator=(glm::mat3 _m3) {
// 		type = 7;
// 		m3 = _m3;
// 	}
// 	shaderVariable operator=(int* _i) {
// 		if (type != 1 || type != -1)
// 			throw "mismatched types";
// 		type = 8;
// 		ip = _i;
// 	}
// 	shaderVariable operator=(float* _f) {
// 		type = 9;
// 		fp = _f;
// 	}
// 	shaderVariable operator=(unsigned int* _ui) {
// 		type = 10;
// 		uip = _ui;
// 	}
// 	shaderVariable operator=(glm::vec3* _v3) {
// 		type = 11;
// 		v3p = _v3;
// 	}
// 	shaderVariable operator=(glm::vec2* _v2) {
// 		type = 12;
// 		v2p = _v2;
// 	}
// 	shaderVariable operator=(glm::vec4* _v4) {
// 		type = 13;
// 		v4p = _v4;
// 	}
// 	shaderVariable operator=(glm::mat4 *_m4) {
// 		type = 14;
// 		m4p = _m4;
// 	}
// 	shaderVariable operator=(glm::mat3 *_m3) {
// 		type = 15;
// 		m3p = _m3;
// 	}

// 	void data() {
// 		switch (this->type) {

// 		}
// 	}
// };

struct texArray
{
	vector<_texture> textures;
	size_t hash;

	// size_t getTexARrayHash(vector<GLuint> &v)
	// {
	// }
	void calcHash()
	{
		if (textures.size() == 0)
		{
			hash = -1;
			return;
		}
		hash = textures[0].t->id;
		for (int i = 1; i < textures.size(); i++)
		{
			hash *= 1e6;
			hash += textures[i].t->id;
		}
	}
	void setTextures(vector<_texture> &texs)
	{
		this->textures = texs;
		this->calcHash();
	}
	void push_back(_texture t)
	{
		this->textures.push_back(t);
		hash = hash / (10.0 * t.t->id);
	}
	_texture &operator[](size_t index)
	{
		return textures[index];
	}
	void unbind()
	{
		for (GLuint i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
};

// class shaderVariables {
// 	map<string, shaderVariable> variables;
// };

class Shader
{
public:
	GLuint Program = -1;

	map<GLenum, string> _shaders;
	GLenum primitiveType = GL_TRIANGLES;
	unordered_map<std::string, GLuint> vars;

	SER_HELPER()
	{
		ar &_shaders &primitiveType;
	}

	GLuint getVar(const string &name)
	{
		auto var = vars.find(name);
		if (var != vars.end())
		{
			return var->second;
		}
		else
		{
			vars.emplace(name, glGetUniformLocation(Program, name.c_str()));
			return vars.at(name);
		}
	}
	void use()
	{
		glUseProgram(Program);
	}
	void setVec2(const std::string &name, const glm::vec2 &value) const
	{
		glUniform2fv(glGetUniformLocation(Program, name.c_str()), 1, glm::value_ptr(value));
	}
	void setVec3(const std::string &name, const glm::vec3 &value) const
	{
		glUniform3fv(glGetUniformLocation(Program, name.c_str()), 1, glm::value_ptr(value));
	}
	void setFloat(const std::string &name, float value) const
	{
		glUniform1f(glGetUniformLocation(Program, name.c_str()), value);
	}
	void setInt(const std::string &name, int value) const
	{
		GLuint vid = glGetUniformLocation(Program, name.c_str());
		if (vid == -1)
			return;
		glUniform1i(vid, value);
	}
	void setUint(const std::string &name, uint value) const
	{
		glUniform1ui(glGetUniformLocation(Program, name.c_str()), value);
	}
	void setMat3(const std::string &name, glm::mat3 &value)
	{
		glUniformMatrix3fv(glGetUniformLocation(Program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
	}
	void setMat4(const std::string &name, glm::mat4 &value)
	{
		glUniformMatrix4fv(glGetUniformLocation(Program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
	}
	void setTexture(int binding, string name, int texId)
	{
		glActiveTexture(GL_TEXTURE0 + binding);
		setInt(name, binding);
		glBindTexture(GL_TEXTURE_2D, texId);
	}
	void bindTextures(texArray &ta)
	{
		GLuint diffuseNr = 0;
		GLuint specularNr = 0;
		GLuint normalNr = 0;
		for (GLuint i = 0; i < ta.textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // Active proper texture unit before binding
			// Retrieve texture number (the N in diffuse_textureN)
			stringstream ss;
			string number;
			string name = ta[i].t->type;

			if (name == "texture_diffuse")
			{
				ss << diffuseNr++; // Transfer GLuint to stream
			}
			else if (name == "texture_specular")
			{
				ss << specularNr++; // Transfer GLuint to stream
			}
			else if (name == "texture_normal")
			{
				ss << normalNr++; // Transfer GLuint to stream
			}
			number = ss.str();

			setTexture(i, "material." + name + number, ta[i].t->id);
		}
	}
	Shader() {}
	Shader(const string computePath)
	{

		_shaders.emplace(GL_COMPUTE_SHADER, computePath);
		// enqueRenderJob([&]() { _Shader(); });
		_Shader();
	}

	Shader(const string vertexPath, const string fragmentPath)
	{
		_shaders.emplace(GL_VERTEX_SHADER, vertexPath);
		_shaders.emplace(GL_FRAGMENT_SHADER, fragmentPath);
		// enqueRenderJob([&]() { _Shader(); });
		_Shader();
	}
	Shader(const string vertexPath, const string geometryPath, const string fragmentPath)
	{
		_shaders.emplace(GL_VERTEX_SHADER, vertexPath);
		_shaders.emplace(GL_GEOMETRY_SHADER, geometryPath);
		_shaders.emplace(GL_FRAGMENT_SHADER, fragmentPath);
		_Shader();
		// enqueRenderJob([&]() { _Shader(); });
	}
	Shader(const string vertexPath, const string tessControlPath, const string tessEvalPath, const string geometryPath, const string fragmentPath)
	{
		_shaders.emplace(GL_VERTEX_SHADER, vertexPath);
		_shaders.emplace(GL_TESS_CONTROL_SHADER, tessControlPath);
		_shaders.emplace(GL_TESS_EVALUATION_SHADER, tessEvalPath);
		_shaders.emplace(GL_GEOMETRY_SHADER, geometryPath);
		_shaders.emplace(GL_FRAGMENT_SHADER, fragmentPath);
		_Shader();
		// enqueRenderJob([&]() { _Shader(); });
	}
	Shader(const string vertexPath, const string tessControlPath, const string tessEvalPath, const string fragmentPath)
	{
		_shaders.emplace(GL_VERTEX_SHADER, vertexPath);
		_shaders.emplace(GL_TESS_CONTROL_SHADER, tessControlPath);
		_shaders.emplace(GL_TESS_EVALUATION_SHADER, tessEvalPath);
		_shaders.emplace(GL_FRAGMENT_SHADER, fragmentPath);
		_Shader();
		// enqueRenderJob([&]() { _Shader(); });
	}
	GLuint loadFile(string file, GLenum ShaderType)
	{
		std::string code;
		code = shaderLoader::load(file);
		const GLchar *vShaderCode = code.c_str();

		GLuint shader;
		GLint success;
		GLchar infoLog[4096];
		// Vertex Shader
		shader = glCreateShader(ShaderType);
		glShaderSource(shader, 1, &vShaderCode, NULL);
		glCompileShader(shader);
		// Print compile errors if any
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 4096, NULL, infoLog);
			// console::log(file);
			console::log(file + "\nERROR::SHADER::COMPILATION_FAILED\n" + string(infoLog));
			cout << file << endl;
			std::cout << "ERROR::SHADER::COMPILATION_FAILED\n"
					  << infoLog << std::endl;
		}
		return shader;
	}
	GLuint loadFromString(string code, GLenum ShaderType)
	{
		const GLchar *vShaderCode = code.c_str();

		GLuint shader;
		GLint success;
		GLchar infoLog[4096];
		// Vertex Shader
		shader = glCreateShader(ShaderType);
		glShaderSource(shader, 1, &vShaderCode, NULL);
		glCompileShader(shader);
		// Print compile errors if any
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 4096, NULL, infoLog);
			console::log("ERROR::SHADER::COMPILATION_FAILED\n" + string(infoLog));
			std::cout << "ERROR::SHADER::COMPILATION_FAILED\n"
					  << infoLog << std::endl;
		}
		return shader;
	}

	void compileShader(vector<GLuint> &shaders)
	{
		GLint success;
		GLchar infoLog[4096];
		// Shader Program
		this->Program = glCreateProgram();
		for (auto &s : shaders)
			glAttachShader(this->Program, s);

		glLinkProgram(this->Program);
		// Print linking errors if any
		glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(this->Program, 4096, NULL, infoLog);
			// std::cout << this->vertexFile << ", " << this->tessCtrlFile << ", " << this->geometryFile << ", " << this->fragmentFile << ", " << this->computeFile << std::endl;
			console::log("ERROR::SHADER::PROGRAM::LINKING_FAILEDn" + string(infoLog));
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
					  << infoLog << std::endl;
		}
		// Delete the shaders as they're linked into our program now and no longer necessery
		for (auto &s : shaders)
			glDeleteShader(s);
	}
	void _Shader()
	{
		waitForRenderJob([&]() {
			if (this->Program != 0)
			{
				glDeleteProgram(this->Program);
				this->Program = 0;
			}
			vector<GLuint> shaders;
			for (auto &i : _shaders)
			{
				shaders.push_back(loadFile(i.second, i.first));
			}
			compileShader(shaders);
		});
	}
	~Shader()
	{
		waitForRenderJob([&]() {
			glDeleteProgram(this->Program);
			_shaders.clear();
		})
	}
};

namespace YAML {

template<>
struct convert<Shader> {
  static Node encode(const Shader& rhs) {
    Node node;
	node["shaders"] = rhs._shaders;
	node["primitive_type"] = rhs.primitiveType;
    return node;
  }

  static bool decode(const Node& node, Shader& rhs) {
	rhs._shaders = node["id"].as<map<GLenum,string>>();
	rhs.primitiveType = node["primitive_type"].as<GLenum>();
    return true;
  }
};
}


