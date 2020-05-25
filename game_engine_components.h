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
	map<string, map<string, gpu_vector<GLuint>* >> shader_model_culled; // for transforms
	map<string, map<string, gpu_vector_proxy<matrix> *>> mats;		  // for rendering

	glm::mat4 view;
	glm::mat4 rot;
	glm::mat4 proj;
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
			for (auto &i : d->shader_model_culled)
			{
				for (auto &j : i.second)
				{
					j.second->bufferData();
				}
			}
		}
	}
	void prepRender(Shader &matProgram)
	{
		glUseProgram(matProgram.Program);
		glUniformMatrix4fv(glGetUniformLocation(matProgram.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(matProgram.Program, "vRot"), 1, GL_FALSE, glm::value_ptr(rot));
		glUniformMatrix4fv(glGetUniformLocation(matProgram.Program, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, GPU_TRANSFORMS->bufferId);
		// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, GPU_RENDERERS->bufferId);
		// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, GPU_MATRIXES->bufferId);

		// change to this cameras data
		mainCamPos = transform->getPosition();
		MainCamForward = transform->forward();
		mainCamUp = transform->up();
		glUniform3f(glGetUniformLocation(matProgram.Program, "camPos"), mainCamPos.x, mainCamPos.y, mainCamPos.z);
		glUniform3f(glGetUniformLocation(matProgram.Program, "camForward"), MainCamForward.x, MainCamForward.y, MainCamForward.z);
		glUniform3f(glGetUniformLocation(matProgram.Program, "floatingOrigin"), mainCamPos.x, mainCamPos.y, mainCamPos.z);
		glUniform1i(glGetUniformLocation(matProgram.Program, "stage"), 1);

		for (auto &i : mats)
		{
			for (auto &j : i.second)
			{
				shader_model_culled[i.first][j.first]->bindData(4);
				uint size = shader_model_culled[i.first][j.first]->size();
				// j.second->bufferData();
				j.second->tryRealloc(size);
				j.second->bindData(3); // bind to something -> mats is matCompute output buffer
				glUniform1ui(glGetUniformLocation(matProgram.Program, "num"), size);
				glDispatchCompute(size / 64 + 1, 1, 1);
				glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
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
				glUniform3fv(glGetUniformLocation(currShader->Program, "viewPos"), 1, glm::value_ptr(mainCamPos));
				glUniform1f(glGetUniformLocation(currShader->Program, "screenHeight"), (float)SCREEN_HEIGHT);
				glUniform1f(glGetUniformLocation(currShader->Program, "screenWidth"), (float)SCREEN_WIDTH);
				// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, j->second->_ids->bufferId);
				mats[i->first][j->first]->bindData(3);

				j->second->m.m->model->Draw(*currShader, shader_model_culled[i->first][j->first]->size());
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
	//void onStart() {
	//	if (!inited) {
	//		transform->gameObject->removeComponent(transform->gameObject->getcomponent<floatingOrigin>());
	//		inited = true;
	//	}
	//}
	//void lateUpdate() {
	//	if (Input.getKeyDown(GLFW_KEY_L))
	//		lockFrustum = !lockFrustum;
	//	if (!lockFrustum) {
	//		mainCamFrust = getFrustum();
	//		mainCamPos = transform->getPosition();
	//		MainCamForward = transform->forward();
	//	}
	//}
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

Barrier cullObjectsBarrier(1);
class cullObjects : public component
{
	
	bool _registerEngineComponent()
	{
		return true;
	}
	void update()
	{
		componentStorage<_camera> * cameras = ((componentStorage<_camera> *)allcomponents.at(typeid(_camera).hash_code()));
		componentStorage<_renderer> * renderers = ((componentStorage<_renderer> *)allcomponents.at(typeid(_renderer).hash_code()));
		deque<_camera>::iterator d = cameras->data.data.begin();
		deque<bool>::iterator v = cameras->data.valid.begin();
		for (; d != cameras->data.data.end(); d++, v++)
		{
			if (*v)
			{
				for (auto &i : renderingManager.shader_model_vector)
				{
					for (auto &j : i.second)
					{
						if(d->shader_model_culled[i.first].find(j.first) == d->shader_model_culled[i.first].end()){
							d->shader_model_culled[i.first][j.first] = new gpu_vector<GLuint>();
							d->mats[i.first][j.first] = new gpu_vector_proxy<matrix>();
							// d->mats[i.first][j.first]->ownStorage();
						}
						auto &s = d->shader_model_culled[i.first][j.first];
						s->ownStorage();
						s->storage->clear();
					}
				}

				for (auto &i : renderingManager.shader_model_vector)
				{
					for (auto &j : i.second)
					{
						j.second->_transformIds = d->shader_model_culled[i.first][j.first];
						// auto &vec = d->shader_model_culled[i.first][j.first]->storage;
						// for (auto &k : j.second->transformIds.data)
						// {
						// 	vec->push_back(k);
						// }
						// std::sort(vec->begin(),vec->end());
					}
				}

				typename deque<bool>::iterator valid = renderers->data.valid.begin();
				for(_renderer& r : renderers->data.data){
					if(*valid){
						if(r.meta)
							r.meta->_transformIds->storage->push_back(r.transform->_T);
					}
					valid++;
				}
				// multi threaded culling / vector build
			}
		}
	}
public:
	UPDATE(cullObjects, update);
	COPY(cullObjects);
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
			{
				vector<_transform>::iterator from = TRANSFORMS.data.begin() + TRANSFORMS.size() / concurrency::numThreads * getThreadID();
				vector<_transform>::iterator to = TRANSFORMS.data.begin() + (getThreadID() != concurrency::numThreads - 1 ? TRANSFORMS.size() / concurrency::numThreads * (getThreadID() + 1) : TRANSFORMS.size());
				int itr = TRANSFORMS.size() / concurrency::numThreads * getThreadID();
				while (from != to)
					GPU_TRANSFORMS->storage->at(itr++) = *from++;
			}

			
			// for (map<string, map<string, renderingMeta *>>::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++)
			// {
			// 	for (map<string, renderingMeta *>::iterator j = i->second.begin(); j != i->second.end(); j++)
			// 	{

			// 		size_t from = j->second->ids.size() / numt * getThreadID();
			// 		size_t to = (getThreadID() != numt - 1 ? j->second->ids.size() / numt : j->second->ids.size() - from);
			// 		if (to > 0)
			// 			memcpy(&(j->second->_ids->storage->at(from)), &(j->second->ids.data[from]), sizeof(GLuint) * to);
			// 	}
			// }
		}
		//		renderCounts[getThreadID()]++;
	}
public:
	UPDATE(copyBuffers, update);
	COPY(copyBuffers);
};
