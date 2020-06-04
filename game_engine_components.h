#pragma once
#include "Component.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "rendering.h"
#include "array_heap.h"
#include "Transform.h"
#include <vector>

using namespace std;

const GLint WIDTH = 768, HEIGHT = 432;
int SCREEN_WIDTH, SCREEN_HEIGHT;

class barrier
{
	mutex m;
	atomic<size_t> count;
	atomic<size_t> objective;

public:
	void reset(size_t num)
	{
		m.lock();
		objective.fetch_add(num);
		m.unlock();
	}
	void wait()
	{
		m.lock();
		count.fetch_add(1);
		m.unlock();
		while (objective.load() != count.load())
		{
			this_thread::sleep_for(1ns);
		}
	}
};

typedef glm::vec4 plane;
struct _frustum
{
	plane top;
	plane left;
	plane right;
	plane bottom;
};

vector<vector<GLuint>> transformIdThreadcache;
vector<vector<_transform>> transformThreadcache;

gpu_vector<GLuint>* camAtomics = new gpu_vector<GLuint>();
class _camera : public component
{
public:
	_camera(GLfloat fov, GLfloat nearPlane, GLfloat farPlane);
	_camera(){};

	GLfloat fov;
	GLfloat nearPlane;
	GLfloat farPlane;
	bool lockFrustum = false;
	bool inited = false;
	_frustum f;
	// map<string, map<string, gpu_vector<GLuint>* >> shader_model_culled; // for transforms
	map<string, map<string, gpu_vector_proxy<matrix> *>> mats;		  // for rendering
	map<string, map<string, GLuint>> culledCounts;

	glm::mat4 view;
	glm::mat4 rot;
	glm::mat4 proj;
	glm::vec2 screen;

	glm::vec3 pos;
	glm::vec3 cullpos;
	glm::mat3 camInv;
	glm::vec2 getScreen(){
		return glm::vec2(glm::tan(glm::radians(fov) / 2) * SCREEN_WIDTH / SCREEN_HEIGHT,glm::tan(glm::radians(fov) / 2));
	}

	int order()
	{
		return 1 - 2;
	}
	static void initPrepRender(Shader &matProgram)
	{
		// glUseProgram(matProgram.Program);
		componentStorage<_camera> * cameras = ((componentStorage<_camera> *)allcomponents.at(typeid(_camera).hash_code()));
		auto d = cameras->data.data.begin();
		auto v = cameras->data.valid.begin();
		for (; d != cameras->data.data.end(); d++, v++)
		{
			for (auto &i : renderingManager.shader_model_vector)
			{
				for (auto &j : i.second)
				{
					j.second->_transformIds->bufferData();
				}
			}
		}
	}
	void prepRender(Shader &matProgram)
	{
		camAtomics->ownStorage();
		glUseProgram(matProgram.Program);
		glUniformMatrix4fv(glGetUniformLocation(matProgram.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(matProgram.Program, "vRot"), 1, GL_FALSE, glm::value_ptr(rot));
		glUniformMatrix4fv(glGetUniformLocation(matProgram.Program, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

		GPU_TRANSFORMS->bindData(0);
		camAtomics->bindData(5);

		// change to this cameras data
		mainCamPos = transform->getPosition();
		MainCamForward = transform->forward();
		mainCamUp = transform->up();
		
		glUniform3f(glGetUniformLocation(matProgram.Program, "floatingOrigin"), pos.x, pos.y, pos.z);
		glUniform1i(glGetUniformLocation(matProgram.Program, "stage"), 1);

		glUniformMatrix3fv(glGetUniformLocation(matProgram.Program, "camInv"), 1, GL_FALSE, glm::value_ptr(camInv));
		glUniform3f(glGetUniformLocation(matProgram.Program, "cullPos"), cullpos.x, cullpos.y, cullpos.z);
		glUniform2f(glGetUniformLocation(matProgram.Program, "screen"), screen.x, screen.y);
		for (auto &i : renderingManager.shader_model_vector)
		{
			for (auto &j : i.second)
			{
				camAtomics->storage->clear();
				camAtomics->storage->push_back(0);
				camAtomics->bufferData();
				renderingManager.shader_model_vector[i.first][j.first]->_transformIds->bindData(4);
				uint size = renderingManager.shader_model_vector[i.first][j.first]->_transformIds->size();
				gpu_vector_proxy<matrix>* mat = mats[i.first][j.first];
				if(mat == 0){
					mat = new gpu_vector_proxy<matrix>();
					mats[i.first][j.first] = mat;
				}
				mat->tryRealloc(size);
				mat->bindData(3); 

				glUniform1f(glGetUniformLocation(matProgram.Program, "radius"), renderingManager.shader_model_vector[i.first][j.first]->radius);
				glUniform1ui(glGetUniformLocation(matProgram.Program, "num"), size);
				glDispatchCompute(size / 64 + 1, 1, 1);
				glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

				camAtomics->retrieveData();
				culledCounts[i.first][j.first] = (*camAtomics)[0];
				// j.second->retrieveData();
				// cout << "f";
			}
		}
	}
	void render()
	{
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glCullFace(GL_BACK);
		for (map<string, map<string, renderingMeta *>>::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++)
		{
			Shader *currShader = i->second.begin()->second->s.s->shader;
			currShader->Use();
			for (map<string, renderingMeta *>::iterator j = i->second.begin(); j != i->second.end(); j++)
			{
				glUseProgram(currShader->Program);
				glUniform1f(glGetUniformLocation(currShader->Program, "material.shininess"), 32);
				glUniform1f(glGetUniformLocation(currShader->Program, "FC"), 2.0 / log2(farPlane + 1));
				glUniform3fv(glGetUniformLocation(currShader->Program, "viewPos"), 1, glm::value_ptr(pos));
				glUniform1f(glGetUniformLocation(currShader->Program, "screenHeight"), (float)SCREEN_HEIGHT);
				glUniform1f(glGetUniformLocation(currShader->Program, "screenWidth"), (float)SCREEN_WIDTH);
				// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, j->second->_ids->bufferId);
				mats[i->first][j->first]->bindData(3);

				j->second->m.m->model->Draw(*currShader, culledCounts[i->first][j->first]);
			}
		}
	}

	glm::mat4 getRotationMatrix()
	{
		return glm::lookAt(glm::vec3(0, 0, 0), transform->forward(), transform->up());
	}
	glm::mat4 GetViewMatrix()
	{
		return glm::translate(-transform->getPosition());
	}
	glm::mat4 getProjection()
	{
		return glm::perspective(glm::radians(this->fov), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.0001f, farPlane);
	}
	_frustum getFrustum()
	{
		glm::mat4 m = getProjection() * getRotationMatrix() * GetViewMatrix();
		f.left = plane(m[0][3] + m[0][0],
					   m[1][3] + m[1][0],
					   m[2][3] + m[2][0],
					   m[3][3] + m[3][0]);
		f.right = plane(m[0][3] - m[0][0],
						m[1][3] - m[1][0],
						m[2][3] - m[2][0],
						m[3][3] - m[3][0]);
		f.top = plane(m[0][3] - m[0][1],
					  m[1][3] - m[1][1],
					  m[2][3] - m[2][1],
					  m[3][3] - m[3][1]);
		f.bottom = plane(m[0][3] + m[0][1],
						 m[1][3] + m[1][1],
						 m[2][3] + m[2][1],
						 m[3][3] + m[3][1]);
		return f;
	}
	COPY(_camera);

private:
};

vector<int> renderCounts = vector<int>(concurrency::numThreads);
class copyBuffers : public component
{
	bool _registerEngineComponent()
	{
		return true;
	}
	void update()
	{
		int numt = concurrency::numThreads;
		if (getThreadID() < numt)
		{
			int id = getThreadID();
			{
				int step = TRANSFORMS.size() / concurrency::numThreads;
				uint i = step * id;
				deque<_transform>::iterator from = TRANSFORMS.data.begin() + step * id;
				deque<bool>::iterator v = TRANSFORMS.valid.begin() + step * id;
				deque<_transform>::iterator to = from + step;
				transformIdThreadcache[id].clear();
				transformIdThreadcache[id].reserve(step + 1);
				transformThreadcache[id].clear();
				transformThreadcache[id].reserve(step + 1);
				if(id == concurrency::numThreads - 1)
					to = TRANSFORMS.data.end();
				while (from != to){
					if(*v){
						transformIdThreadcache[id].emplace_back(i);
						transformThreadcache[id].emplace_back(*from);
					}
					++from;
					++v;
					++i;
				}
			}

			
			for (map<string, map<string, renderingMeta *>>::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++)
			{
				for (map<string, renderingMeta *>::iterator j = i->second.begin(); j != i->second.end(); j++)
				{
					int step = j->second->ids.size() / concurrency::numThreads;
					typename deque<GLuint>::iterator from = j->second->ids.data.begin() + step  * id;
					typename deque<GLuint>::iterator to = from + step;
					typename vector<GLuint>::iterator v = j->second->_transformIds->storage->begin() + step * id;
					if(id == concurrency::numThreads - 1){
						to = j->second->ids.data.end();
					}
					while(from != to){
						*v = *from;
						++from;
						++v;
					}
						// memcpy(&(j->second->_transformIds->storage->at(from)), &(j->second->ids.data[from]), sizeof(GLuint) * to);
					
				}
			}
		}
		//		renderCounts[getThreadID()]++;
	}
public:
	UPDATE(copyBuffers, update);
	COPY(copyBuffers);
};
