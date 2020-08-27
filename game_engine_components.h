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


_shader billBoardGenerator;
_shader billBoardShader;
_model bill;
// _texture t;
// t.namedTexture("bill1");
// t.t->gen(1024,1024);
void makeBillboard(_model m, _texture t, _renderer* r){

	billBoardGenerator = _shader("res/shaders/model_no_inst.vert","res/shaders/model.frag");
	billBoardShader = _shader("res/shaders/model.vert","res/shaders/model.frag");

	bill.makeProcedural();
	waitForRenderJob([&](){
		bill.meshes().push_back(Mesh());
		bill.mesh().vertices = {glm::vec3(-1.0f,  1.0f, 0.0f)
		,glm::vec3(-1.0f, -1.0f, 0.0f)
		,glm::vec3(1.0f,  1.0f, 0.0f)
		,glm::vec3(1.0f, -1.0f, 0.0f)};

		bill.mesh().uvs = {glm::vec2(0.0f, 1.0f)
		,glm::vec2(0.0f, 0.0f)
		,glm::vec2(1.0f, 1.0f)
		,glm::vec2(1.0f, 0.0f)};

		bill.mesh().indices = {0,3,1,0,2,3};

		bill.mesh().makePoints();
		bill.mesh().reloadMesh();
		bill.recalcBounds();

		renderTexture gBuffer;
		
		gBuffer.scr_width = t.t->dims.x;
		gBuffer.scr_height = t.t->dims.y;
		gBuffer.init();
		gBuffer.addColorAttachment("gAlbedoSpec",renderTextureType::UNSIGNED_BYTE,0);
		gBuffer.addColorAttachment("gPosition",renderTextureType::FLOAT,1);
		gBuffer.addColorAttachment("gNormal",renderTextureType::FLOAT,2);
		gBuffer.addDepthBuffer();
		gBuffer.finalize();

		gBuffer.use();

		billBoardGenerator.ref().use();
		billBoardGenerator.ref().setFloat("FC", 2.0 / log2(1e2 + 1));
		billBoardGenerator.ref().setVec3("viewPos",glm::vec3(0));
		billBoardGenerator.ref().setFloat("screenHeight", (float)t.t->dims.y);
		billBoardGenerator.ref().setFloat("screenWidth", (float)t.t->dims.x);
		mat4 mvp = glm::perspective(10.f,1.f,0.f,20.f) * glm::translate(glm::vec3(0,0,10));
		billBoardGenerator.ref().setMat4("mvp", mvp);
		
		mat4 model = glm::translate(glm::vec3(0,0,10));
		billBoardGenerator.ref().setMat4("model", model);
		
		glBindVertexArray( m.mesh().VAO );
		glDrawElements(GL_TRIANGLES,m.mesh().indices.size(),GL_UNSIGNED_INT, 0);

		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindVertexArray( 0 );


		glCopyImageSubData(t.t->id, GL_TEXTURE_2D, 0, 0, 0, 0,
                   gBuffer.getTexture("gAlbedoSpec"), GL_TEXTURE_2D, 0, 0, 0, 0,
                   gBuffer.scr_width, gBuffer.scr_height, 1);	
	});
	bill.mesh().textures.push_back(t);
	r->set(billBoardShader,bill);
	r->meta->isBillboard = 1;
}


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
		// batchManager::updateBatches();
		// glUseProgram(matProgram.Program);
		// componentStorage<_camera> * cameras = ((componentStorage<_camera> *)allcomponents.at(typeid(_camera).hash_code()));
		// auto d = cameras->data.data.begin();
		// auto v = cameras->data.valid.begin();
		// for (; d != cameras->data.data.end(); d++, v++)
		// {
		// 	for (auto &i : renderingManager::shader_model_vector)
		// 	{
		// 		for (auto &j : i.second)
		// 		{
		// 			j.second->_transformIds->bufferData();
		// 		}
		// 	}
		// }
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
		matProgram.setMat4("view",view);
		matProgram.setMat4("vRot",rot);
		matProgram.setMat4("projection",proj);
		matProgram.setVec3("floatingOrigin",pos);
		matProgram.setInt("stage",1);
		matProgram.setMat3("camInv",camInv);
		matProgram.setVec3("cullPos",cullpos);
		matProgram.setVec2("screen",screen);
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
			currShader->setFloat("FC", 2.0 / log2(farPlane + 1));
			currShader->setVec3("viewPos",pos);
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
		batchManager::batches.pop();
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
		quadShader.setVec3("viewPos",pos);

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
		shaderLightingPass.setVec2("WindowSize",glm::vec2(SCREEN_WIDTH,SCREEN_HEIGHT));

		shaderLightingPass.setMat4("view",view);
		shaderLightingPass.setMat4("proj",proj);
		shaderLightingPass.setFloat("FC", 2.0 / log2(farPlane + 1));
		shaderLightingPass.setVec3("viewPos", pos);
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
		particle_renderer::drawParticles(view, rot, proj);
		// appendStat("render particles", gt_.stop());
		appendStat("render particles", t.stop());

		glDepthMask(GL_TRUE);
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

vector<GLuint> transformIdsToBuffer;
vector<_transform> transformsToBuffer;

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
		transformIdThreadcache[id].clear();
		if(TRANSFORMS.density() > 0.5){

			deque<_transform>::iterator from = TRANSFORMS.data.begin() + step * id;
			deque<_transform>::iterator to = from + step;

			if(id == concurrency::numThreads - 1)
				to = TRANSFORMS.data.end();
			for (auto itr = from; itr != to; itr++, i++){
				TRANSFORMS_TO_BUFFER[i] = *itr;
			}

		}else{

			deque<bool>::iterator from = TRANSFORMS.valid.begin() + step * id;
			deque<bool>::iterator to = from + step;

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

		int __rendererId = 0;
		int __rendererOffset = 0;
		typename vector<__renderer>::iterator __r = __RENDERERS->storage->begin();
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
	void lateUpdate(){
		
		for(int i = 0; i < transformIdThreadcache[id].size(); i++){
			transformIdsToBuffer[offset + i] = transformIdThreadcache[id][i];
			// transformsToBuffer[offset + i] = TRANSFORMS[transformIdThreadcache[id][i]];
		}
	}
public:
	//UPDATE(copyBuffers, update);
	COPY(copyBuffers);
};
