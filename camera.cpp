#include "camera.h"
using namespace std;

typedef glm::vec4 plane;
struct _frustum
{
	plane top;
	plane left;
	plane right;
	plane bottom;
};
struct DrawElementsIndirectCommand
{
	uint count;
	uint primCount;
	uint firstIndex;
	uint baseVertex;
	uint baseInstance;
};

// vector<glm::vec4> lightPos;
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,
			1.0f,
			0.0f,
			0.0f,
			1.0f,
			-1.0f,
			-1.0f,
			0.0f,
			0.0f,
			0.0f,
			1.0f,
			1.0f,
			0.0f,
			1.0f,
			1.0f,
			1.0f,
			-1.0f,
			0.0f,
			1.0f,
			0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
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
void makeBillboard(_model m, _texture t, _renderer *r)
{
	// billBoardGenerator = _shader("res/shaders/defLighting.vert","res/shaders/red.frag");
	billBoardGenerator = _shader("res/shaders/model_no_inst.vert", "res/shaders/model.frag");
	billBoardShader = _shader("res/shaders/model.vert", "res/shaders/model.frag");

	glm::vec3 max = [&] {
		glm::vec3 vert(-INFINITY);
		for (auto &i : m.meshes())
		{
			for (auto v : i->vertices)
			{
				// v = v * glm::mat3(glm::rotate(glm::radians(-90.f), vec3(1,0,0)));
				if (vert.x < v.x)
				{
					vert.x = v.x;
				}
				if (vert.y < v.z)
				{
					vert.y = v.z;
				}
			}
		}
		return vert;
	}();
	glm::vec3 min = [&] {
		glm::vec3 vert(INFINITY);
		for (auto &i : m.meshes())
		{
			for (auto v : i->vertices)
			{
				// v = v * glm::mat3(glm::rotate(glm::radians(-90.f), vec3(1,0,0)));
				if (vert.x > v.x)
				{
					vert.x = v.x;
				}
				if (vert.y > v.z)
				{
					vert.y = v.z;
				}
			}
		}
		return vert;
	}();
	bill.makeProcedural();
	waitForRenderJob([&]() {
		bill.meshes().push_back(new Mesh());
		bill.mesh().vertices = {glm::vec3(min.x, max.y, 0.0f), glm::vec3(min.x, min.y, 0.0f), glm::vec3(max.x, max.y, 0.0f), glm::vec3(max.x, min.y, 0.0f)};

		// for(auto& i : bill.mesh().vertices){
		// 	i *= 10;
		// }

		bill.mesh().normals = {glm::vec3(0, 0, 1),
							   glm::vec3(0, 0, 1),
							   glm::vec3(0, 0, 1),
							   glm::vec3(0, 0, 1)};

		bill.mesh().uvs = {glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f)};

		bill.mesh().indices = {0, 3, 1, 0, 2, 3};

		bill.mesh().makePoints();
		bill.mesh().reloadMesh();
		bill.recalcBounds();

		renderTexture gBuffer;

		gBuffer.scr_width = t.t->dims.x + 1;
		gBuffer.scr_height = t.t->dims.y + 1;
		gBuffer.init();
		gBuffer.addColorAttachment("gAlbedoSpec", renderTextureType::UNSIGNED_BYTE, 0);
		gBuffer.addColorAttachment("gPosition", renderTextureType::FLOAT, 1);
		gBuffer.addColorAttachment("gNormal", renderTextureType::FLOAT, 2);
		gBuffer.addDepthBuffer();
		gBuffer.finalize();

		gBuffer.use();
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// billBoardGenerator.ref().use();
		// renderQuad();
		billBoardGenerator.ref().use();
		billBoardGenerator.ref().setFloat("FC", 2.0 / log2(1e2 + 1));
		billBoardGenerator.ref().setVec3("viewPos", glm::vec3(0));
		billBoardGenerator.ref().setFloat("screenHeight", (float)t.t->dims.y);
		billBoardGenerator.ref().setFloat("screenWidth", (float)t.t->dims.x);
		mat4 mvp =
			// glm::perspective(radians(90.f),1.f,0.001f,20000.f)
			// * glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0,0,1), glm::vec3(0,1,0))
			glm::translate(glm::vec3(-0.4f, -0.9f, -1)) * glm::rotate(glm::radians(-90.f), vec3(1, 0, 0)) * glm::scale(glm::vec3(1.2f / (abs(max.x) + abs(min.x)), 0, 1.95 / (abs(max.y) + abs(min.y))));
		billBoardGenerator.ref().setMat4("mvp", mvp);

		mat4 model = glm::translate(glm::vec3(0, 0, -10)) * glm::rotate(glm::radians(-90.f), vec3(1, 0, 0));
		billBoardGenerator.ref().setMat4("model", model);

		for (auto &mes : m.meshes())
		{

			billBoardGenerator.ref().bindTextures(mes->textures);
			glBindVertexArray(mes->VAO);
			glDrawElements(GL_TRIANGLES, mes->indices.size(), GL_UNSIGNED_INT, 0);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			mes->textures.unbind();
		}

		t.t->id = gBuffer.getTexture("gAlbedoSpec");
		// t = gBuffer.getTex("gAlbedoSpec");
		// glCopyImageSubData(gBuffer.getTexture("gAlbedoSpec"), GL_TEXTURE_2D, 0, 0, 0, 0,
		//            t.t->id, GL_TEXTURE_2D, 0, 0, 0, 0,
		//            gBuffer.scr_width, gBuffer.scr_height, 1);
	});
	bill.mesh().textures.push_back(t);
	r->set(billBoardShader, bill);
	r->meta->isBillboard = 1;
}

lightVolume lv;

// class _camera : public component
// {
// public:
// _camera::_camera(GLfloat fov, GLfloat nearPlane, GLfloat farPlane);
camera::camera()
{
	inited = false;
	enqueRenderJob([&]() {
		if (lv.vertices.size() == 0)
		{
			lightVolumeModel = _model("res/models/cube/cube.obj");
			modelManager::models_id[lightVolumeModel.m]->model->loadModel();
			// lightVolumeModel.m->loadModel();
			lv.indices = lightVolumeModel.mesh().indices;
			lv.vertices = lightVolumeModel.mesh().vertices;
			lv.setupMesh();
		}

		gBuffer.scr_width = SCREEN_WIDTH;
		gBuffer.scr_height = SCREEN_HEIGHT;
		gBuffer.init();
		gBuffer.addColorAttachment("gAlbedoSpec", renderTextureType::UNSIGNED_BYTE, 0);
		gBuffer.addColorAttachment("gPosition", renderTextureType::FLOAT, 1);
		gBuffer.addColorAttachment("gNormal", renderTextureType::FLOAT, 2);
		gBuffer.addDepthBuffer();
		gBuffer.finalize();
		inited = true;
	});
	_shaderLightingPass = _shader("res/shaders/defferedLighting.vert", "res/shaders/defferedLighting.frag");
	_quadShader = _shader("res/shaders/defLighting.vert", "res/shaders/defLighting.frag");
};

camera::~camera()
{
	waitForRenderJob([&]() {
		gBuffer.destroy();
	});
}
void _camera::onEdit()
{
	RENDER(c->fov);
	RENDER(c->nearPlane);
	RENDER(c->farPlane);
}
glm::vec3 camera::screenPosToRay(glm::vec2 mp)
{

	float mouseX = mp.x / (SCREEN_WIDTH * 0.5f) - 1.0f;
	float mouseY = mp.y / (SCREEN_HEIGHT * 0.5f) - 1.0f;

	glm::mat4 invVP = glm::inverse(this->getProjection() * this->getRotationMatrix());
	glm::vec4 screenPos = glm::vec4(mouseX, -mouseY, 1.0f, 1.0f);
	glm::vec4 worldPos = invVP * screenPos;

	return glm::normalize(glm::vec3(worldPos));
}
glm::vec2 camera::getScreen()
{
	return glm::vec2(glm::tan(fov / 2) * SCREEN_WIDTH / SCREEN_HEIGHT, glm::tan(fov / 2));
}

// int _camera::order()
// {
// 	return 1 - 2;
// }

void _camera::onStart()
{
	c = new camera();
}
void _camera::onDestroy()
{
	delete c;
}
void camera::update(glm::vec3 position, glm::quat rotation){
		this->pos = position;
		this->dir = rotation * vec3(0,0,1);
		this->up = rotation * vec3(0,1,0);
		this->view = this->GetViewMatrix();
		this->rot = this->getRotationMatrix();
		this->proj = this->getProjection();
		this->screen = this->getScreen();
		if (!this->lockFrustum)
		{
			this->camInv = glm::mat3(this->rot);
			this->cullpos = this->pos;
		}
}
void camera::prepRender(Shader &matProgram)
{
	gpuTimer gt_;
	timer cpuTimer;
	gt_.start();
	cpuTimer.start();

	_rendererOffsets = *(__renderer_offsets->storage);

	GPU_MATRIXES->tryRealloc(__RENDERERS_in->size());
	GPU_TRANSFORMS->bindData(0);
	GPU_MATRIXES->bindData(3);
	__RENDERERS_in->bindData(1);
	__renderer_offsets->bindData(5);
	__renderer_offsets->bufferData();
	__rendererMetas->bindData(8);
	__rendererMetas->bufferData();

	matProgram.use();
	matProgram.setMat4("view", view);
	matProgram.setMat4("vRot", rot);
	matProgram.setMat4("projection", proj);
	matProgram.setVec3("floatingOrigin", pos);
	matProgram.setInt("stage", 1);
	matProgram.setMat3("camInv", camInv);
	matProgram.setVec3("cullPos", cullpos);
	matProgram.setVec3("camUp", up);
	matProgram.setVec2("screen", screen);
	matProgram.setUint("num", __RENDERERS_in->size());

	glDispatchCompute(__RENDERERS_in->size() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
	__renderer_offsets->retrieveData();

	appendStat("matrix compute cpu", cpuTimer.stop());
	appendStat("matrix compute", gt_.stop());

	// sort particles
	timer t;
	t.start();
	if (!lockFrustum)
		particle_renderer::setCamCull(camInv, cullpos);
	particle_renderer::sortParticles(proj * rot * view, rot * view, pos, dir, up, screen);
	appendStat("particles sort", t.stop());
}
void camera::render()
{
	// _shader wireFrame("res/shaders/terrainShader/terrain.vert",
	// 		  "res/shaders/terrainShader/terrain.tesc",
	// 		  "res/shaders/terrainShader/terrain.tese",
	// 		  "res/shaders/terrainShader/terrain.geom",
	// 		  "res/shaders/terrainShader/terrain.frag");

	// vector<GLuint>& _rendererOffsets = *(__renderer_offsets->storage);
	if (!inited)
		return;
	gpuTimer t;
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glCullFace(GL_BACK);
	t.start();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gBuffer.use();
	gBuffer.resize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	int counter = 0;
	GPU_MATRIXES->bindData(3);
	for (auto &i : batchManager::batches.front())
	{
		Shader *currShader = i.first.meta()->shader;
		currShader->use();
		currShader->setFloat("FC", 2.0 / log2(farPlane + 1));
		currShader->setVec3("viewPos", pos);
		currShader->setVec3("viewDir", dir);
		currShader->setFloat("screenHeight", (float)SCREEN_HEIGHT);
		currShader->setFloat("screenWidth", (float)SCREEN_WIDTH);
		glm::mat4 vp = proj * rot;
		currShader->setMat4("vp", vp);

		for (auto &j : i.second)
		{
			texArray ta = j.first;
			currShader->bindTextures(ta);
			for (auto &k : j.second)
			{
				int count = __renderer_offsets->storage->at(counter) - _rendererOffsets[counter];
				if (count > 0)
				{
					currShader->setUint("matrixOffset", _rendererOffsets[counter]);
					glBindVertexArray(k.second->VAO);
					glDrawElementsInstanced(currShader->primitiveType, k.second->indices.size(), GL_UNSIGNED_INT, 0, count);

					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glBindVertexArray(0);
				}
				++counter;
			}
			ta.unbind();
		}
		// }
	}
	batchManager::batches.pop();
	appendStat("render cam", t.stop());

	t.start();
	// gt_.start();
	// // 2. lighting pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Shader &quadShader = *_quadShader.meta()->shader;
	quadShader.use();
	quadShader.setInt("gAlbedoSpec", 0);
	quadShader.setInt("gPosition", 1);
	quadShader.setInt("gNormal", 2);
	quadShader.setVec3("viewPos", pos);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gAlbedoSpec"));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gPosition"));
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gNormal"));
	renderQuad();

	gBuffer.blitDepth(0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// glEnable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);
	// glDisable(GL_CULL_FACE);
	// glDepthFunc(GL_LEQUAL);
	glCullFace(GL_FRONT);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	Shader &shaderLightingPass = *_shaderLightingPass.meta()->shader;
	shaderLightingPass.use();
	shaderLightingPass.setInt("gAlbedoSpec", 0);
	shaderLightingPass.setInt("gPosition", 1);
	shaderLightingPass.setInt("gNormal", 2);
	shaderLightingPass.setInt("gDepth", 3);

	lightingManager::gpu_pointLights->bindData(1);
	GPU_TRANSFORMS->bindData(2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gAlbedoSpec"));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gPosition"));
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gNormal"));
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gBuffer.rboDepth);
	shaderLightingPass.setVec2("WindowSize", glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT));

	shaderLightingPass.setMat4("view", view);
	shaderLightingPass.setMat4("proj", proj);
	shaderLightingPass.setFloat("FC", 2.0 / log2(farPlane + 1));
	shaderLightingPass.setVec3("viewPos", pos);
	shaderLightingPass.setVec3("floatingOrigin", pos);
	lv.Draw(lightingManager::pointLights.size());

	// Always good practice to set everything back to defaults once configured.
	for (GLuint i = 0; i < 4; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
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
	particle_renderer::drawParticles(view, rot, proj, pos);
	// appendStat("render particles", gt_.stop());
	appendStat("render particles", t.stop());

	glDepthMask(GL_TRUE);
}

glm::mat4 camera::getRotationMatrix()
{
	return glm::lookAt(glm::vec3(0.f), dir, up);
	// return glm::toMat4(glm::quatLookAtLH(transform->forward(), transform->up()));
}
glm::mat4 camera::GetViewMatrix()
{
	return glm::translate(-pos);
}
glm::mat4 camera::getProjection()
{
	return glm::perspective(this->fov, (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.0001f, farPlane);
}
// _frustum _camera::getFrustum()
// {
// 	glm::mat4 m = getProjection() * getRotationMatrix() * GetViewMatrix();
// 	f.left = plane(m[0][3] + m[0][0],
// 				   m[1][3] + m[1][0],
// 				   m[2][3] + m[2][0],
// 				   m[3][3] + m[3][0]);
// 	f.right = plane(m[0][3] - m[0][0],
// 					m[1][3] - m[1][0],
// 					m[2][3] - m[2][0],
// 					m[3][3] - m[3][0]);
// 	f.top = plane(m[0][3] - m[0][1],
// 				  m[1][3] - m[1][1],
// 				  m[2][3] - m[2][1],
// 				  m[3][3] - m[3][1]);
// 	f.bottom = plane(m[0][3] + m[0][1],
// 					 m[1][3] + m[1][1],
// 					 m[2][3] + m[2][1],
// 					 m[3][3] + m[3][1]);
// 	return f;
// }
// COPY(_camera);
// SER3(fov,nearPlane,farPlane);
// private:
// };
REGISTER_COMPONENT(_camera)

vector<int> renderCounts = vector<int>(concurrency::numThreads);
vector<vector<vector<GLint>>> transformIdThreadcache;
// vector<vector<_transform>> transformThreadcache;

// vector<int> transformIdsToBuffer;
// vector<_transform> transformsToBuffer;
vector<vector<glm::vec3>> positionsToBuffer;
vector<vector<glm::quat>> rotationsToBuffer;
vector<vector<glm::vec3>> scalesToBuffer;

// class copyBuffers : public component
// {
// 	bool _registerEngineComponent()
// 	{
// 		return true;
// 	}

// public:
// 	int id;
// 	int offset;
// 	void update()
// 	{
// 		int numt = concurrency::numThreads;
// 		int step = Transforms.size() / concurrency::numThreads;
// 		int i = step * id;
// 		// transformIdThreadcache[id].clear();
// 		// if(Transforms.density() > 0.5){

// 		// 	int from = step * id;
// 		// 	int to = from + step;

// 		// 	if(id == concurrency::numThreads - 1)
// 		// 		to = Transforms.size();
// 		// 	for (auto itr = from; itr != to; itr++, i++){
// 		// 		TRANSFORMS_TO_BUFFER[i] = transform2(itr).getTransform();
// 		// 		Transforms.transform_updates[itr].pos = false;
// 		// 		Transforms.transform_updates[itr].rot = false;
// 		// 		Transforms.transform_updates[itr].scl = false;
// 		// 	}

// 		// }else{
// 		transformIdThreadcache[id][0].clear(); // pos
// 		transformIdThreadcache[id][1].clear(); // scl
// 		transformIdThreadcache[id][2].clear(); // rot

// 		positionsToBuffer[id].clear();
// 		rotationsToBuffer[id].clear();
// 		scalesToBuffer[id].clear();

// 		auto from = Transforms.transform_updates.begin() + step * id;
// 		auto to = from + step;

// 		// transformIdThreadcache[id].reserve(step + 1);
// 		if (id == concurrency::numThreads - 1)
// 			to = Transforms.transform_updates.end();
// 		while (from != to)
// 		{
// 			if (from->pos)
// 			{
// 				from->pos = false;
// 				transformIdThreadcache[id][0].emplace_back(i);
// 				positionsToBuffer[id].emplace_back(((transform2)i).getPosition());
// 			}
// 			if (from->rot)
// 			{
// 				from->rot = false;
// 				transformIdThreadcache[id][1].emplace_back(i);
// 				rotationsToBuffer[id].emplace_back(((transform2)i).getRotation());
// 			}
// 			if (from->scl)
// 			{
// 				from->scl = false;
// 				transformIdThreadcache[id][2].emplace_back(i);
// 				scalesToBuffer[id].emplace_back(((transform2)i).getScale());
// 			}
// 			++from;
// 			++i;
// 		}
// 		// }

// 		int __rendererId = 0;
// 		int __rendererOffset = 0;
// 		typename vector<__renderer>::iterator __r = __RENDERERS_in->storage->begin();
// 		for (auto &i : batchManager::batches.back())
// 		{
// 			for (auto &j : i.second)
// 			{
// 				for (auto &k : j.second)
// 				{
// 					int step = k.first->ids.size() / concurrency::numThreads;
// 					typename deque<GLuint>::iterator from = k.first->ids.data.begin() + step * id;
// 					typename deque<GLuint>::iterator to = from + step;
// 					__r = __RENDERERS_in->storage->begin() + __rendererOffset + step * id;
// 					if (id == concurrency::numThreads - 1)
// 					{
// 						to = k.first->ids.data.end();
// 					}
// 					while (from != to)
// 					{
// 						__r->transform = *from;
// 						__r->id = __rendererId;
// 						++from;
// 						++__r;
// 					}
// 					++__rendererId;
// 					__rendererOffset += k.first->ids.size();
// 				}
// 			}
// 		}
// 	}
// 	// void lateUpdate(){

// 	// 	for(int i = 0; i < transformIdThreadcache[id].size(); i++){
// 	// 		transformIdsToBuffer[offset + i] = transformIdThreadcache[id][i];
// 	// 		// transformsToBuffer[offset + i] = TRANSFORMS[transformIdThreadcache[id][i]];
// 	// 	}
// 	// }
// public:
// 	//UPDATE(copyBuffers, update);
// 	COPY(copyBuffers);
// };
