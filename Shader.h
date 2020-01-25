#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <GL/glew.h>
#include <map>
#include "helper1.h"
#include "Shaderinclude.h"
#include "include.h"

using namespace std;


struct shaderVariable {
	int type = -1;
	union {
		//constants
		GLfloat f;
		GLint i;
		GLuint ui;
		glm::vec3 v3;
		glm::vec2 v2;
		glm::vec4 v4;
		glm::mat4 m4;
		glm::mat3 m3;

		//references
		GLfloat* fp;
		GLint* ip;
		GLuint* uip;
		glm::vec3* v3p;
		glm::vec2* v2p;
		glm::vec4* v4p;
		glm::mat4* m4p;
		glm::mat3* m3p;

	};
	shaderVariable operator=(int _i) {
		if (type != 1 || type != -1)
			throw "mismatched types";
		type = 1;
		i = _i;
	}
	shaderVariable operator=(float _f) {
		type = 0;
		f = _f;
	}
	shaderVariable operator=(unsigned int _ui) {
		type = 2;
		ui = _ui;
	}
	shaderVariable operator=(glm::vec3 _v3) {
		type = 3;
		v3 = _v3;
	}
	shaderVariable operator=(glm::vec2 _v2) {
		type = 4;
		v2 = _v2;
	}
	shaderVariable operator=(glm::vec4 _v4) {
		type = 5;
		v4 = _v4;
	}
	shaderVariable operator=(glm::mat4 _m4) {
		type = 6;
		m4 = _m4;
	}
	shaderVariable operator=(glm::mat3 _m3) {
		type = 7;
		m3 = _m3;
	}
	shaderVariable operator=(int* _i) {
		if (type != 1 || type != -1)
			throw "mismatched types";
		type = 8;
		ip = _i;
	}
	shaderVariable operator=(float* _f) {
		type = 9;
		fp = _f;
	}
	shaderVariable operator=(unsigned int* _ui) {
		type = 10;
		uip = _ui;
	}
	shaderVariable operator=(glm::vec3* _v3) {
		type = 11;
		v3p = _v3;
	}
	shaderVariable operator=(glm::vec2* _v2) {
		type = 12;
		v2p = _v2;
	}
	shaderVariable operator=(glm::vec4* _v4) {
		type = 13;
		v4p = _v4;
	}
	shaderVariable operator=(glm::mat4 *_m4) {
		type = 14;
		m4p = _m4;
	}
	shaderVariable operator=(glm::mat3 *_m3) {
		type = 15;
		m3p = _m3;
	}

	void data() {
		switch (this->type) {

		}
	}
};

class shaderVariables {
	map<string, shaderVariable> variables;
};

class Shader
{
public:
	GLuint Program;
	string vertexFile;
	string geometryFile;
	string fragmentFile;
	string computeFile;
	bool shadowMap;
	// Constructor generates the shader on the fly

	Shader(const string computePath) {

		this->computeFile = computePath;
		//_Shader(vertexFile, fragmentFile, this->shadowMap);
		renderJob rj;
		rj.type = doFunc;
		rj.work = [&]() { _Shader(computeFile); };
		renderLock.lock();
		renderWork.push(rj);
		renderLock.unlock();
	}

	Shader(const string vertexPath, const string fragmentPath, bool shadowMap = true) {
		this->vertexFile = vertexPath;
		this->fragmentFile = fragmentPath;
		this->shadowMap = shadowMap;
		//_Shader(vertexFile, fragmentFile, this->shadowMap);
		renderJob rj;
		rj.type = doFunc;
		rj.work = [&]() { _Shader(vertexFile, fragmentFile, this->shadowMap); };
		renderLock.lock();
		renderWork.push(rj);
		renderLock.unlock();
	}
	Shader(const string vertexPath, const string geometryPath, const string fragmentPath, bool shadowMap = true) {
		this->vertexFile = vertexPath;
		this->fragmentFile = fragmentPath;
		this->geometryFile = geometryPath;
		this->shadowMap = shadowMap;
		/*_Shader(vertexFile, geometryFile, fragmentFile, this->shadowMap);
*/
		renderJob rj;
		rj.type = doFunc;
		rj.work = [&]() { _Shader(vertexFile,geometryFile, fragmentFile, this->shadowMap); };
		renderLock.lock();
		renderWork.push(rj);
		renderLock.unlock();
	}
	GLuint loadFile(string file, GLenum ShaderType) {
		std::string code;
		//std::ifstream File;
		//File.exceptions(std::ifstream::badbit);
		//File.open(file);
		//std::stringstream ShaderStream;
		//ShaderStream << File.rdbuf();

		code = shaderLoader::load(file);
		//code = ShaderStream.str();
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
			std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
		return shader;

	}

	void compileShader(vector<GLuint>& shaders) {
		GLint success;
		GLchar infoLog[4096];
		// Shader Program
		this->Program = glCreateProgram();
		for(auto &s : shaders)
			glAttachShader(this->Program, s);

		glLinkProgram(this->Program);
		// Print linking errors if any
		glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(this->Program, 4096, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}
		// Delete the shaders as they're linked into our program now and no longer necessery
		for (auto &s : shaders)
			glDeleteShader(s);
	}
	void _Shader(const string vertexPath, const string geometryPath, const string fragmentPath, bool shadowMap = true)
	{
		vector<GLuint> shaders;
		shaders.push_back(loadFile(vertexPath, GL_VERTEX_SHADER));
		shaders.push_back(loadFile(geometryPath, GL_GEOMETRY_SHADER));
		shaders.push_back(loadFile(fragmentPath, GL_FRAGMENT_SHADER));
		compileShader(shaders);
	}
	void _Shader(const string vertexPath, const string fragmentPath, bool shadowMap=true)
	{

		vector<GLuint> shaders;
		shaders.push_back(loadFile(vertexPath, GL_VERTEX_SHADER));
		shaders.push_back(loadFile(fragmentPath, GL_FRAGMENT_SHADER));
		compileShader(shaders);
	}
	void _Shader(const string computePath)
	{
		vector<GLuint> shaders;
		shaders.push_back(loadFile(computeFile, GL_COMPUTE_SHADER));
		compileShader(shaders);
	}
	// Uses the current shader
	void Use()
	{
		glUseProgram(this->Program);
	}
};

#endif
