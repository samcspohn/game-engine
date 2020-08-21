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
#include "helper1.h"
#include <vector>
#include "renderTexture.h"
#include "lighting.h"
#include "particles.h"
#include <queue>
#include <deque>
#include <set>

using namespace std;


typedef glm::vec4 plane;
struct _frustum
{
	plane top;
	plane left;
	plane right;
	plane bottom;
};
struct DrawElementsIndirectCommand{
	uint  count;
	uint  primCount;
	uint  firstIndex;
	uint  baseVertex;
	uint  baseInstance;
} ;


// vector<glm::vec4> lightPos;
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}



struct camera_frame{
	glm::mat4 view;
	glm::mat4 rot;
	glm::mat4 proj;
	glm::vec2 screen;

	glm::vec3 pos;
	glm::vec3 cullpos;
	glm::mat3 camInv;
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
	// map<string, map<string, gpu_vector<GLuint>* >> shader_model_culled; // for transforms
	// map<string, map<string, gpu_vector_proxy<matrix> *>> mats;		  // for rendering
	vector<GLuint> _rendererOffsets;

	renderTexture gBuffer;
	_shader _shaderLightingPass;
	_shader _quadShader;
	_model lightVolumeModel;
	lightVolume lv;

	queue<camera_frame> frames;

	
	glm::vec2 getScreen(){
		return glm::vec2(glm::tan(glm::radians(fov) / 2) * SCREEN_WIDTH / SCREEN_HEIGHT,glm::tan(glm::radians(fov) / 2));
	}

	int order()
	{
		return 1 - 2;
	}
	void onStart(){
		waitForRenderJob([&](){
			lightVolumeModel = _model("res/models/cube/cube.obj");
			lightVolumeModel.m->model->loadModel();
			// lightVolumeModel.m->loadModel();
			lv.indices = lightVolumeModel.mesh().indices;
			lv.vertices = lightVolumeModel.mesh().vertices;
			lv.setupMesh();

			gBuffer.scr_width = SCREEN_WIDTH;
			gBuffer.scr_height = SCREEN_HEIGHT;
			gBuffer.init();
			gBuffer.addColorAttachment("gAlbedoSpec",renderTextureType::UNSIGNED_BYTE,0);
			gBuffer.addColorAttachment("gPosition",renderTextureType::FLOAT,1);
			gBuffer.addColorAttachment("gNormal",renderTextureType::FLOAT,2);
			gBuffer.addDepthBuffer();
			gBuffer.finalize();
		});
		_shaderLightingPass = _shader("res/shaders/defferedLighting.vert","res/shaders/defferedLighting.frag");
		_quadShader = _shader("res/shaders/defLighting.vert","res/shaders/defLighting.frag");
	}

	void calcFrame(){
		camera_frame c;
		c.view = GetViewMatrix();
		c.rot = getRotationMatrix();
		c.proj = getProjection();
		c.screen = getScreen();
		c.pos = transform->getPosition();
		if(!lockFrustum){
			c.camInv = glm::mat3(c.rot);
			c.cullpos = c.pos;
		}
		frames.push(c);
	}
	void prepRender(Shader &matProgram)
	{

		_rendererOffsets = *(__renderer_offsets->storage);
		
		GPU_MATRIXES->tryRealloc(__RENDERERS->size());
		GPU_TRANSFORMS->bindData(0);
		GPU_MATRIXES->bindData(3);
		__RENDERERS->bindData(1);
		__RENDERERS->bufferData();
		__renderer_offsets->bindData(5);
		__renderer_offsets->bufferData();
		__rendererMetas->bindData(8);
		__rendererMetas->bufferData();

		mainCamPos = transform->getPosition();
		MainCamForward = transform->forward();
		mainCamUp = transform->up();
		
		matProgram.use();
		matProgram.setMat4("view",frames.front().view);
		matProgram.setMat4("vRot",frames.front().rot);
		matProgram.setMat4("projection",frames.front().proj);
		matProgram.setVec3("floatingOrigin",frames.front().pos);
		matProgram.setInt("stage",1);
		matProgram.setMat3("camInv",frames.front().camInv);
		matProgram.setVec3("cullPos",frames.front().cullpos);
		matProgram.setVec2("screen",frames.front().screen);
		matProgram.setUint("num",__RENDERERS->size());
		
		glDispatchCompute(__RENDERERS->size() / 64 + 1, 1, 1);
		glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
		
		__renderer_offsets->retrieveData();
				
	}
	void render()
	{
		timer t;
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glCullFace(GL_BACK);
		t.start();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		gBuffer.use();
		gBuffer.resize(SCREEN_WIDTH,SCREEN_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);    
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		int counter = 0;
		GPU_MATRIXES->bindData(3);
		for(auto &i : batchManager::batches.front()){
			Shader *currShader = i.first.s->shader;
			currShader->use();
			currShader->setFloat("material.shininess",32);
			currShader->setFloat("FC", 2.0 / log2(farPlane + 1));
			currShader->setVec3("viewPos",frames.front().pos);
			currShader->setFloat("screenHeight", (float)SCREEN_HEIGHT);
			currShader->setFloat("screenWidth", (float)SCREEN_WIDTH);
			for(auto &j : i.second){
				texArray ta = j.first;
				currShader->bindTextures(ta);
				for(auto &k : j.second){
					int count = __renderer_offsets->storage->at(counter) - _rendererOffsets[counter];
					if(count > 0){
						currShader->setUint("matrixOffset", _rendererOffsets[counter]);
						glBindVertexArray( k.second->VAO );
						glDrawElementsInstanced(GL_TRIANGLES,k.second->indices.size(),GL_UNSIGNED_INT, 0,count);

						glBindBuffer(GL_ARRAY_BUFFER,0);
						glBindVertexArray( 0 );
					}
					++counter;
				}
				ta.unbind();
			}
		}
		appendStat("render cam", t.stop());

		t.start();
		// gt_.start();
		// // 2. lighting pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		Shader& quadShader = *_quadShader.s->shader;
		quadShader.use();
		quadShader.setInt("gAlbedoSpec", 0);
		quadShader.setInt("gPosition", 1);
		quadShader.setInt("gNormal", 2);
		quadShader.setVec3("viewPos",frames.front().pos);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gAlbedoSpec"));
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gPosition"));
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gNormal"));
		renderQuad();
		
		gBuffer.blitDepth(0,SCREEN_WIDTH,SCREEN_HEIGHT);


		// glEnable(GL_DEPTH_CLAMP);
		glDisable(GL_DEPTH_TEST);  
		// glDisable(GL_CULL_FACE);
		// glDepthFunc(GL_LEQUAL); 
		glCullFace(GL_FRONT);  
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);

		Shader& shaderLightingPass = *_shaderLightingPass.s->shader;
		shaderLightingPass.use();
		shaderLightingPass.setInt("gAlbedoSpec", 0);
		shaderLightingPass.setInt("gPosition", 1);
		shaderLightingPass.setInt("gNormal", 2);
		shaderLightingPass.setInt("gDepth", 3);

		plm.gpu_pointLights->bindData(1);
		GPU_TRANSFORMS->bindData(2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gAlbedoSpec"));
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gPosition"));
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gNormal"));
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gBuffer.rboDepth);
		// glUniform2f(glGetUniformLocation(shaderLightingPass.Program, "WindowSize"), SCREEN_WIDTH, SCREEN_HEIGHT);
		shaderLightingPass.setVec2("WindowSize",glm::vec2(SCREEN_WIDTH,SCREEN_HEIGHT));

		shaderLightingPass.setMat4("view",frames.front().view);
		shaderLightingPass.setMat4("proj",frames.front().proj);
		shaderLightingPass.setFloat("FC", 2.0 / log2(farPlane + 1));
		shaderLightingPass.setVec3("viewPos", frames.front().pos);
		lv.Draw(plm.pointLights.size());

		// Always good practice to set everything back to defaults once configured.
		for ( GLuint i = 0; i < 4; i++ )
		{
			glActiveTexture( GL_TEXTURE0 + i );
			glBindTexture( GL_TEXTURE_2D, 0 );
		}
		// appendStat("render lighting", gt_.stop());
		appendStat("render lighting cpu", t.stop());

		glDepthMask(GL_TRUE);

		// render particle
		t.start();
		// gt_.start();
		glEnable(GL_DEPTH_TEST);  
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// sort particles
		t.start();
		if(!lockFrustum)
			particle_renderer::setCamCull(frames.front().camInv,frames.front().cullpos);
		particle_renderer::sortParticles(frames.front().proj * frames.front().rot * frames.front().view, frames.front().rot * frames.front().view, mainCamPos,frames.front().screen);
		appendStat("particles sort", t.stop());

		particle_renderer::drawParticles(frames.front().view, frames.front().rot, frames.front().proj);
		appendStat("render particles", t.stop());

		glDepthMask(GL_TRUE);
		frames.pop();
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
vector<vector<GLuint>> transformIdThreadcache;
// vector<vector<_transform>> transformThreadcache;

struct render_data{
	vector<GLuint> transformIdsToBuffer;
	vector<_transform> transformsToBuffer;
	map<_shader,map<texArray,map<renderingMeta*,Mesh*>>> frame_batches;
};

struct render_queue{
	mutex m;
	set<int> avail;
	deque<render_data> cache;
	queue<render_data*> data;
	render_data& push(){
		m.lock();
		if(avail.size() > 0){
			data.push(&cache[*avail.begin()]);
			avail.erase(avail.begin());
		}
		else{
			cache.emplace_back();
			data.push(&cache.back());
		}
		m.unlock();
		return *data.back();
	}
	render_data& front(){
		return *data.front();
	}
	render_data& back(){
		return *data.back();
	}
	void pop(){
		m.lock();
		render_data* r = data.front();
		int i = 0;
		while(r != &cache[i]){
			i++;
		}
		r->transformIdsToBuffer.clear();
		r->transformsToBuffer.clear();
		data.pop();
		avail.emplace(i);
		m.unlock();
	}
};

render_queue RENDER_QUEUE;

class copyBuffers : public component
{
	bool _registerEngineComponent()
	{
		return true;
	}
public:
	int id;
	int offset;
	void update()
	{
		int numt = concurrency::numThreads;
		int step = TRANSFORMS.size() / concurrency::numThreads;
		uint i = step * id;
		deque<bool>::iterator from = TRANSFORMS.valid.begin() + step * id;
		deque<bool>::iterator to = from + step;
		transformIdThreadcache[id].clear();
		transformIdThreadcache[id].reserve(step + 1);
		if(id == concurrency::numThreads - 1)
			to = TRANSFORMS.valid.end();
		while (from != to){
			if(*from){
				transformIdThreadcache[id].emplace_back(i);
			}
			++from;
			++i;
		}
	}
	void lateUpdate(){
		
		for(int i = 0; i < transformIdThreadcache[id].size(); i++){
			RENDER_QUEUE.back().transformIdsToBuffer[offset + i] = transformIdThreadcache[id][i];
			RENDER_QUEUE.back().transformsToBuffer[offset + i] = TRANSFORMS[transformIdThreadcache[id][i]];
		}
		
		int __rendererId = 0;
		int __rendererOffset = 0;
		typename vector<__renderer>::iterator __r = __RENDERERS->storage->begin();
		// batchManager::batches.back();
		for(auto &i : batchManager::batches.back()){
			for(auto &j : i.second){
				for(auto &k : j.second){
					int step = k.first->ids.size() / concurrency::numThreads;
					typename deque<GLuint>::iterator from = k.first->ids.data.begin() + step * id;
					typename deque<GLuint>::iterator to = from + step;
					__r = __RENDERERS->storage->begin() + __rendererOffset + step * id;
					if(id == concurrency::numThreads - 1){
						to = k.first->ids.data.end();
					}
					while(from != to){
						__r->transform = *from;
						__r->id = __rendererId;
						++from;
						++__r;
					}
					++__rendererId;
					__rendererOffset += k.first->ids.size();
				}
			}
		}
	}
public:
	//UPDATE(copyBuffers, update);
	COPY(copyBuffers);
};
