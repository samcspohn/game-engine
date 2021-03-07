#include "_rendering/camera.h"

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


camera::camera()
{
	inited = false;
	// enqueRenderJob([&]() {
		// if (lv.vertices.size() == 0)
		// {
		// 	lightVolumeModel = _model("res/models/cube/cube.obj");
		// 	modelManager::models_id[lightVolumeModel.m]->model->loadModel();
		// 	// lightVolumeModel.m->loadModel();
		// 	lv.indices = lightVolumeModel.mesh().indices;
		// 	lv.vertices = lightVolumeModel.mesh().vertices;
		// 	lv.setupMesh();
		// }

		gBuffer.scr_width = SCREEN_WIDTH;
		gBuffer.scr_height = SCREEN_HEIGHT;
		gBuffer.init();
		gBuffer.addColorAttachment("gAlbedoSpec", renderTextureType::UNSIGNED_BYTE, 0);
		gBuffer.addColorAttachment("gPosition", renderTextureType::FLOAT, 1);
		gBuffer.addColorAttachment("gNormal", renderTextureType::FLOAT, 2);
		gBuffer.addDepthBuffer();
		gBuffer.finalize();
		inited = true;
	// });
	// _shaderLightingPass = _shader("res/shaders/defferedLighting.vert", "res/shaders/defferedLighting.frag");
	// _quadShader = _shader("res/shaders/defLighting.vert", "res/shaders/defLighting.frag");
};

camera::~camera()
{
	// waitForRenderJob([&]() {
		gBuffer.destroy();
	// });
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

void camera::update(glm::vec3 position, glm::quat rotation){
		this->pos = position;
		this->dir = rotation * glm::vec3(0,0,1);
		this->up = rotation * glm::vec3(0,1,0);
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
	// timer t;
	// t.start();
	// if (!lockFrustum)
	// 	particle_renderer::setCamCull(camInv, cullpos);
	// particle_renderer::sortParticles(proj * rot * view, rot * view, pos, dir, up, screen);
	// appendStat("particles sort", t.stop());
}

void renderOpaque(camera& c){
	gpuTimer t;

    glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	int counter = 0;
	GPU_MATRIXES->bindData(3);
	// for (auto &i : batchManager::batches.front())
	// {
    for(auto &i : renderingManager::shader_model_vector){
		Shader *currShader = shaderManager::shaders_ids[i.first]->shader;
		currShader->use();
		currShader->setFloat("FC", 2.0 / log2(c.farPlane + 1));
		currShader->setVec3("viewPos", c.pos);
		currShader->setVec3("viewDir", c.dir);
		currShader->setFloat("screenHeight", (float)SCREEN_HEIGHT);
		currShader->setFloat("screenWidth", (float)SCREEN_WIDTH);
		glm::mat4 vp = c.proj * c.rot;
		currShader->setMat4("vp", vp);


        for(auto &j : i.second){
            Model* mod = modelManager::models_id[j.first]->model;
            for(auto& mesh : mod->meshes){
                texArray ta = mesh->textures;
                currShader->bindTextures(ta);
                int count = __renderer_offsets->storage->at(counter) - c._rendererOffsets[counter];
				if (count > 0)
				{
					currShader->setUint("matrixOffset", c._rendererOffsets[counter]);
					glBindVertexArray(mesh->VAO);
					glDrawElementsInstanced(currShader->primitiveType, mesh->indices.size(), GL_UNSIGNED_INT, 0, count);

					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glBindVertexArray(0);
				}
				++counter;
    			ta.unbind();
            }
        }
		// for (auto &j : i.second)
		// {
		// 	texArray ta = j.first;
		// 	currShader->bindTextures(ta);
		// 	for (auto &k : j.second)
		// 	{
		// 		int count = __renderer_offsets->storage->at(counter) - c._rendererOffsets[counter];
		// 		if (count > 0)
		// 		{
		// 			currShader->setUint("matrixOffset", c._rendererOffsets[counter]);
		// 			glBindVertexArray(k.second->VAO);
		// 			glDrawElementsInstanced(currShader->primitiveType, k.second->indices.size(), GL_UNSIGNED_INT, 0, count);

		// 			glBindBuffer(GL_ARRAY_BUFFER, 0);
		// 			glBindVertexArray(0);
		// 		}
		// 		++counter;
		// 	}
		// 	ta.unbind();
		// }
		// }
	}
	// batchManager::batches.pop();
	// appendStat("render cam", t.stop());
}
void directionalLighting(camera& c){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Shader &quadShader = *_quadShader.meta()->shader;
	quadShader.use();
	quadShader.setInt("gAlbedoSpec", 0);
	quadShader.setInt("gPosition", 1);
	quadShader.setInt("gNormal", 2);
	quadShader.setVec3("viewPos", c.pos);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.getTexture("gAlbedoSpec"));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.getTexture("gPosition"));
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, c.gBuffer.getTexture("gNormal"));
	renderQuad();
	for (GLuint i = 0; i < 4; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	// appendStat("render lighting", gt_.stop());
	// appendStat("render lighting cpu", t.stop());
}

// void defferedLighting(camera& c){
// 	gpuTimer t;

// // t.start();

// 	c.gBuffer.blitDepth(0, c.width, c.height);

// 	// glEnable(GL_DEPTH_CLAMP);
// 	glDisable(GL_DEPTH_TEST);
// 	// glDisable(GL_CULL_FACE);
// 	// glDepthFunc(GL_LEQUAL);
// 	glCullFace(GL_FRONT);
// 	glDepthMask(GL_FALSE);
// 	glEnable(GL_BLEND);
// 	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
// 	glBlendEquation(GL_FUNC_ADD);

// 	Shader &shaderLightingPass = *_shaderLightingPass.meta()->shader;
// 	shaderLightingPass.use();
// 	shaderLightingPass.setInt("gAlbedoSpec", 0);
// 	shaderLightingPass.setInt("gPosition", 1);
// 	shaderLightingPass.setInt("gNormal", 2);
// 	shaderLightingPass.setInt("gDepth", 3);

// 	lightingManager::gpu_pointLights->bindData(1);
// 	GPU_TRANSFORMS->bindData(2);
// 	glActiveTexture(GL_TEXTURE0);
// 	glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gAlbedoSpec"));
// 	glActiveTexture(GL_TEXTURE1);
// 	glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gPosition"));
// 	glActiveTexture(GL_TEXTURE2);
// 	glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gNormal"));
// 	glActiveTexture(GL_TEXTURE3);
// 	glBindTexture(GL_TEXTURE_2D, gBuffer.rboDepth);
// 	shaderLightingPass.setVec2("WindowSize", glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT));

// 	shaderLightingPass.setMat4("view", c.view);
// 	shaderLightingPass.setMat4("proj", c.proj);
// 	shaderLightingPass.setFloat("FC", 2.0 / log2(c.farPlane + 1));
// 	shaderLightingPass.setVec3("viewPos", c.pos);
// 	shaderLightingPass.setVec3("floatingOrigin", c.pos);
// 	lv.Draw(lightingManager::pointLights.size());

// 	// Always good practice to set everything back to defaults once configured.
// 	for (GLuint i = 0; i < 4; i++)
// 	{
// 		glActiveTexture(GL_TEXTURE0 + i);
// 		glBindTexture(GL_TEXTURE_2D, 0);
// 	}
// 	// appendStat("render lighting", gt_.stop());
// 	appendStat("render lighting cpu", t.stop());
// }

// void renderParticles(camera& c){
// // render particle
//     gpuTimer t;
// 	t.start();
// 	// gt_.start();
// 	glEnable(GL_DEPTH_TEST);
// 	glEnable(GL_BLEND);
// 	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 	glDisable(GL_CULL_FACE);
// 	glDepthMask(GL_FALSE);
// 	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 	particle_renderer::drawParticles(view, rot, proj, pos);
// 	// appendStat("render particles", gt_.stop());
// 	appendStat("render particles", t.stop());
// }
void camera::render(GLFWwindow* window)
{
	if (!inited)
		return;
	glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float)height;

    glViewport(0, 0, width, height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gBuffer.use();
	gBuffer.resize(width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderOpaque(*this);
    glDepthMask(GL_TRUE);
    directionalLighting(*this);
    glDepthMask(GL_TRUE);
    // defferedLighting(*this);

    // renderParticles(*this);
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
	return glm::perspective(this->fov, (GLfloat)width / (GLfloat)height, nearPlane, farPlane);
}