#include "_rendering/camera.h"
#include "particles/particles.h"
#include "batchManager.h"
#include "lighting/lighting.h"

unsigned int quadVAO = 0;
unsigned int quadVBO;
std::map<int, std::function<void(camera &)>> renderShit;
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

camera::camera()
{
	inited = false;
	enqueRenderJob([&]() {
		// if (lv.vertices.size() == 0)
		// {
		// 	lightVolumeModel = _model("res/models/cube/cube.obj");
		// 	modelManager::models_id[lightVolumeModel.m]->model->loadModel();
		// 	// lightVolumeModel.m->loadModel();
		// 	lv.indices = lightVolumeModel.mesh().indices;
		// 	lv.vertices = lightVolumeModel.mesh().vertices;
		// 	lv.setupMesh();
		// }
		
		gBuffer.scr_width = this->width;
		gBuffer.scr_height = this->height;
		gBuffer.init();
		gBuffer.addColorAttachment("gAlbedoSpec", renderTextureType::UNSIGNED_BYTE, 0);
		gBuffer.addColorAttachment("gPosition", renderTextureType::FLOAT, 1);
		gBuffer.addColorAttachment("gNormal", renderTextureType::FLOAT, 2);
		gBuffer.addDepthBuffer();
		gBuffer.finalize();
		inited = true;
	});
	// _shaderLightingPass = _shader("res/shaders/defferedLighting.vert", "res/shaders/defferedLighting.frag");
	// _quadShader = _shader("res/shaders/defLighting.vert", "res/shaders/defLighting.frag");
};

camera::~camera()
{
	waitForRenderJob([&]() {
		gBuffer.destroy();
	});
}

glm::vec3 camera::screenPosToRay(glm::vec2 mp)
{

	float mouseX = mp.x / (this->width * 0.5f) - 1.0f;
	float mouseY = mp.y / (this->height * 0.5f) - 1.0f;

	glm::mat4 invVP = glm::inverse(this->getProjection() * this->getRotationMatrix());
	glm::vec4 screenPos = glm::vec4(mouseX, -mouseY, 1.0f, 1.0f);
	glm::vec4 worldPos = invVP * screenPos;

	return glm::normalize(glm::vec3(worldPos));
}
glm::vec2 camera::getScreen()
{
	return glm::vec2(glm::tan(fov / 2) * this->width / this->height, glm::tan(fov / 2));
}

void camera::update(glm::vec3 position, glm::quat rotation)
{
	this->pos = position;
	this->dir = rotation * glm::vec3(0, 0, 1);
	this->up = rotation * glm::vec3(0, 1, 0);
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
void camera::prepRender(Shader &matProgram, GLFWwindow *window)
{
	gpuTimer gt_;
	timer cpuTimer;
	gt_.start();
	cpuTimer.start();

	_rendererOffsets = *(__renderer_offsets->storage);

	GPU_MATRIXES->tryRealloc(__RENDERERS_in_size);
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
	matProgram.setUint("num", __RENDERERS_in_size);

	glDispatchCompute(__RENDERERS_in_size / 64 + 1, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
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

void renderOpaque(camera &c)
{
	gpuTimer t;

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	int counter = 0;
	GPU_MATRIXES->bindData(3);
	for (auto &i : batchManager::batches.front())
	{
		// for (auto &i : renderingManager::shader_model_vector)
		// {
		Shader *currShader = i.first.meta()->shader.get();
		currShader->use();
		currShader->setFloat("FC", 2.0 / log2(c.farPlane + 1));
		currShader->setVec3("viewPos", c.pos);
		currShader->setVec3("viewDir", c.dir);
		currShader->setFloat("screenHeight", (float)c.height);
		currShader->setFloat("screenWidth", (float)c.width);
		glm::mat4 vp = c.proj * c.rot;
		currShader->setMat4("vp", vp);

		// for (auto &j : i.second)
		// {
		// 	Model *mod = modelManager::models_id[j.first]->model.get();
		// 	if (counter >= c._rendererOffsets.size())
		// 		return;6
		// 	for (auto &mesh : mod->meshes)
		// 	{
		// 		texArray ta = mesh->textures;
		// 		currShader->bindTextures(ta);
		// 		int count = __renderer_offsets->storage->at(counter) - c._rendererOffsets[counter];
		// 		if (count > 0)
		// 		{
		// 			currShader->setUint("matrixOffset", c._rendererOffsets[counter]);
		// 			glBindVertexArray(mesh->VAO);
		// 			glDrawElementsInstanced(currShader->primitiveType, mesh->indices.size(), GL_UNSIGNED_INT, 0, count);

		// 			glBindBuffer(GL_ARRAY_BUFFER, 0);
		// 			glBindVertexArray(0);
		// 		}
		// 		ta.unbind();
		// 	}
		// 	// todo remove with batching
		// 	++counter;
		// }
		for (auto &j : i.second)
		{
			texArray ta = j.first;
			currShader->bindTextures(ta);
			for (auto &k : j.second)
			{
				int count = __renderer_offsets->storage->at(counter) - c._rendererOffsets[counter];
				if (count > 0)
				{
					currShader->setUint("matrixOffset", c._rendererOffsets[counter]);
					glBindVertexArray(k.second->VAO);
					glDrawElementsInstanced(currShader->primitiveType, k.second->indices.size(), GL_UNSIGNED_INT, 0, count);

					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glBindVertexArray(0);
				}
				++counter;
			}
			ta.unbind();
		}
	}
	batchManager::batches.pop();
	// appendStat("render cam", t.stop());
}
void directionalLighting(camera &c)
{

	static Shader quadShader("res/shaders/defLighting.vert", "res/shaders/defLighting.frag");
	quadShader.use();
	quadShader.setInt("gAlbedoSpec", 0);
	quadShader.setInt("gPosition", 1);
	quadShader.setInt("gNormal", 2);
	quadShader.setVec3("viewPos", c.pos);

	// glDisable(GL_DEPTH_TEST);
	// // glDisable(GL_CULL_FACE);
	// // glDepthFunc(GL_LEQUAL);
	// // glCullFace(GL_FRONT);
	// glDepthMask(GL_FALSE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.getTexture("gAlbedoSpec"));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.getTexture("gPosition"));
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.getTexture("gNormal"));
	// glActiveTexture(GL_TEXTURE3);
	// glBindTexture(GL_TEXTURE_2D, c.gBuffer.rboDepth);
	renderQuad();
	for (GLuint i = 0; i < 4; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	// appendStat("render lighting", gt_.stop());
	// appendStat("render lighting cpu", t.stop());
}

void defferedLighting(camera &c)
{
	// gpuTimer t;

	static lightVolume lv;
	if (lv.VAO == 0)
	{
		// lv.indices = {
		// 	0, 1, 2,
		// 	0, 2, 3,
		// 	4, 7, 6,
		// 	4, 7, 5,
		// 	0, 4, 5,
		// 	0, 5, 1,
		// 	1, 5, 6,
		// 	1, 6, 2,
		// 	2, 6, 7,
		// 	2, 7, 3,
		// 	4, 0, 3,
		// 	4, 3, 7};
		// lv.vertices = {
		// 	vec3(1.f, -1.f, -1.f),
		// 	vec3(1.f, -1.f, 1.f),
		// 	vec3(-1.f, -1.f, 1.f),
		// 	vec3(-1.f, -1.f, -1.f),
		// 	vec3(1.f, 1.f, -1.f),
		// 	vec3(1.f, 1.f, 1.f),
		// 	vec3(-1.f, 1.f, 1.f),
		// 	vec3(-1.f, 1.f, -1.f)};
		// lv.setupMesh();

		_model lightVolumeModel("res/models/cube/cube.obj");
		model_manager.meta[lightVolumeModel.id]->model->loadModel();
		// lightVolumeModel.m->loadModel();
		lv.indices = lightVolumeModel.mesh().indices;
		lv.vertices = lightVolumeModel.mesh().vertices;
		lv.setupMesh();
	}

	c.gBuffer.blitDepth(0, c.width, c.height);

	// glEnable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);
	// glDisable(GL_CULL_FACE);
	// glDepthFunc(GL_LEQUAL);
	glCullFace(GL_FRONT);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	// _shaderLightingPass = _shader("res/shaders/defferedLighting.vert", "res/shaders/defferedLighting.frag");
	// _quadShader = _shader("res/shaders/defLighting.vert", "res/shaders/defLighting.frag");
	static Shader shaderLightingPass("res/shaders/defferedLighting.vert", "res/shaders/defferedLighting.frag");
	// Shader &shaderLightingPass = *_shaderLightingPass.meta()->shader;
	shaderLightingPass.use();
	shaderLightingPass.setInt("gAlbedoSpec", 0);
	shaderLightingPass.setInt("gPosition", 1);
	shaderLightingPass.setInt("gNormal", 2);
	shaderLightingPass.setInt("gDepth", 3);

	lightingManager::gpu_pointLights->bindData(1);
	GPU_TRANSFORMS->bindData(2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.getTexture("gAlbedoSpec"));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.getTexture("gPosition"));
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.getTexture("gNormal"));
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.rboDepth);
	shaderLightingPass.setVec2("WindowSize", glm::vec2(c.width, c.height));

	shaderLightingPass.setMat4("view", c.rot);
	shaderLightingPass.setMat4("proj", c.proj);
	shaderLightingPass.setFloat("FC", 2.0 / log2(c.farPlane + 1));
	shaderLightingPass.setVec3("viewPos", c.pos);
	shaderLightingPass.setVec3("floatingOrigin", c.pos);
	lv.Draw(lightingManager::pointLights.size());

	for (GLuint i = 0; i < 4; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	// appendStat("render lighting", gt_.stop());
	// appendStat("render lighting cpu", t.stop());
}

void renderParticles(camera &c)
{
	// render particle
	gpuTimer t;
	t.start();
	// gt_.start();
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	particle_renderer::drawParticles(c.view, c.rot, c.proj, c.pos, c.farPlane, c.height, c.width);
	// appendStat("render particles", gt_.stop());
	appendStat("render particles", t.stop());
}
void camera::render(GLFWwindow *window)
{
	if (!inited)
		return;
	// width = 1920;
	// height = 1080;
	glfwGetWindowSize(window, &width, &height);
	ratio = width / (float)height;

	glViewport(0, 0, width, height);
	glClearColor(clearColor.r,clearColor.r,clearColor.b, 1.f);
	// glClearColor(1.0f, 0.7f, 0.5f, 1.0f);
	gBuffer.use();
	gBuffer.resize(width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderOpaque(*this);

	for (auto &x : renderShit)
	{
		x.second(*this);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	directionalLighting(*this);
	gBuffer.blitDepth(0, width, height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	defferedLighting(*this);
	// glDepthMask(GL_TRUE);
	renderParticles(*this);
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
template <typename T>
GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> _perspective(T fovy, T aspect, T zNear, T zFar)
{
	// assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

	T const tanHalfFovy = tan(fovy / static_cast<T>(2));

	mat<4, 4, T, defaultp> Result(static_cast<T>(0));
	Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
	Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
	Result[2][2] = -(zFar + zNear) / (zFar - zNear);
	Result[2][3] = -static_cast<T>(1);
	Result[3][2] = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
	return Result;
}
glm::mat4 camera::getProjection()
{
	// return _perspective(this->fov, (GLfloat)width / (GLfloat)height, nearPlane, farPlane);
	float aspect = float(this->width) / float(this->height);
	if (aspect > 0.f && aspect < 3.f)
		return glm::perspective(this->fov, aspect, nearPlane, farPlane);
	else
		return glm::perspective(this->fov, 1920.f / 1080.f, nearPlane, farPlane);
}

_camera::_camera()
{
	c = make_unique<camera>();
}
_camera::_camera(const _camera &cam)
{
	c = make_unique<camera>();
}
