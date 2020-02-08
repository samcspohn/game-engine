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

class barrier{
	mutex m;
	atomic<size_t> count;
	atomic<size_t> objective;
	public:
	void reset(size_t num){
		m.lock();
		objective.fetch_add(num);
		m.unlock();
	}
	void wait(){
		m.lock();
		count.fetch_add(1);
		m.unlock();
		while(objective.load() != count.load()){
			this_thread::sleep_for(1ns);
		}
	}
};

typedef glm::vec4 plane;
struct _frustum {
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
	~_camera();
	GLfloat fov;
	GLfloat nearPlane;
	GLfloat farPlane;
	bool lockFrustum = false;
	bool inited = false;
	_frustum f;
	int order() {
		return 1 - 2;
	}
	void render(glm::mat4 rot, glm::mat4 proj, glm::mat4 view) {
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		for (map<string, map<string, renderingMeta*> >::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++) {
			Shader* currShader = i->second.begin()->second->s.s->shader;
			currShader->Use();
			for (map<string, renderingMeta*>::iterator j = i->second.begin(); j != i->second.end(); j++) {
				glUseProgram(currShader->Program);
				glUniform1f(glGetUniformLocation(currShader->Program, "material.shininess"), 32);
				glUniform1f(glGetUniformLocation(currShader->Program, "farPlane"), 1e32f);
				// GLuint matPView = glGetUniformLocation(currShader->Program, "view");
				// GLuint matvRot = glGetUniformLocation(currShader->Program, "vRot");
				// GLuint matProjection = glGetUniformLocation(currShader->Program, "proj");
				// glUniformMatrix4fv(matPView, 1, false, glm::value_ptr(view));
				// glUniformMatrix4fv(matvRot, 1, false, glm::value_ptr(rot));
				// glUniformMatrix4fv(matProjection, 1, false, glm::value_ptr(proj));

				cout << j->second->ids.size() << endl;
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, j->second->_ids->bufferId);

				j->second->m.m->model->Draw(*currShader, j->second->ids.size());
			}
		}
	}

	glm::mat4 getRotationMatrix() {
		glm::vec3 p1(0), p2(glm::vec3(1));
		return glm::lookAt(p1, p2, this->transform->up());
	}
	glm::mat4 GetViewMatrix() {
		return glm::translate(-transform->getPosition());
	}
	glm::mat4 getProjection() {
		return glm::perspective(glm::radians(this->fov), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, nearPlane, farPlane);
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
	_frustum getFrustum() {
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
class copyBuffers : public component {
	bool _registerEngineComponent(){ 
		return true;
	}
	void update() {
		int numt = concurrency::numThreads;
		if (getThreadID() < numt) {
           {
               vector<_transform>::iterator from = TRANSFORMS.data.begin() + TRANSFORMS.size() / concurrency::numThreads * getThreadID();
               vector<_transform>::iterator to = TRANSFORMS.data.begin() + (getThreadID() != concurrency::numThreads - 1 ?
                                               TRANSFORMS.size() / concurrency::numThreads * (getThreadID() + 1) :
                                               TRANSFORMS.size());
               int itr = TRANSFORMS.size() / concurrency::numThreads * getThreadID();
               while(from != to)
               GPU_TRANSFORMS->storage->at(itr++) = *from++;
           }

            {
                size_t from = gpu_renderers.size() / numt * getThreadID();
                size_t to = (getThreadID() != numt - 1 ?
                    gpu_renderers.size() / numt :
                    gpu_renderers.size() - from);
                memcpy(&(GPU_RENDERERS->storage->at(from)), &(gpu_renderers.data[from]), sizeof(__renderer) * to);
            }


			for (map<string, map<string, renderingMeta*> >::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++) {
				for (map<string, renderingMeta*>::iterator j = i->second.begin(); j != i->second.end(); j++) {

					size_t from = j->second->ids.size() / numt * getThreadID();
					size_t to = (getThreadID() != numt - 1 ?
						j->second->ids.size() / numt :
						j->second->ids.size() - from);
					if(to > 0)
						memcpy(&(j->second->_ids->storage->at(from)), &(j->second->ids.data[from]), sizeof(GLuint) * to);
				}
			}

		}
//		renderCounts[getThreadID()]++;

	}
	UPDATE(copyBuffers, update);
	COPY(copyBuffers);
};
