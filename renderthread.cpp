#include "renderthread.h"
#include <string>
#include "Transform.h"
// #include "_rendering/_renderer.h"
#include "gpu_sort.h"
void renderFunc(camera &c, GLFWwindow *window)
{
    float ratio;
    int width, height;
    // mat4x4 m, p, mvp;

    c.render(window);
    // glfwGetFramebufferSize(window, &width, &height);
    // ratio = width / (float)height;

    // glViewport(0, 0, width, height);
    // glClearColor(1.f, .5f, .3f, 1.f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LESS);
    // glDepthMask(GL_TRUE);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    // shader.meta()->shader->use();
    // glm::mat4 pers = glm::perspective(glm::radians(80.f), 16.f / 9.f, 1.f, 1000.f);
    // shader.meta()->shader->setMat4("perspective", pers);
    // glm::mat4 view = glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
    // view = glm::translate(glm::vec3(0)) * view;
    // shader.meta()->shader->setMat4("view", view);
    // for (auto &m : cube.meta()->model->meshes)
    // {
    //     glBindVertexArray(m->VAO);
    //     for (int i = 0; i < _renderers.size(); i++)
    //     {
    //         if (_renderers.valid[i])
    //         {
    //             glm::mat4 mod = _renderers.data[i].transform.getModel();
    //             shader.meta()->shader->setMat4("model", mod);
    //             glDrawElementsInstanced(GL_TRIANGLES, m->indices.size(), GL_UNSIGNED_INT, 0,1);
    //             // glDrawElements(GL_TRIANGLES, m->indices.size(), GL_UNSIGNED_INT, 0);
    //         }
    //     }
    //     glBindBuffer(GL_ARRAY_BUFFER, 0);
    //     glBindVertexArray(0);
    // }
}

vector<int> renderCounts = vector<int>(concurrency::numThreads);
vector<vector<vector<GLint>>> transformIdThreadcache;
vector<vector<glm::vec3>> positionsToBuffer;
vector<vector<glm::quat>> rotationsToBuffer;
vector<vector<glm::vec3>> scalesToBuffer;

void initiliazeStuff()
{
    _quadShader = _shader("res/shaders/defLighting.vert", "res/shaders/defLighting.frag");

    transformIdThreadcache = vector<vector<vector<int>>>(concurrency::numThreads, vector<vector<int>>(3));
    positionsToBuffer = vector<vector<glm::vec3>>(concurrency::numThreads);
    rotationsToBuffer = vector<vector<glm::quat>>(concurrency::numThreads);
    scalesToBuffer = vector<vector<glm::vec3>>(concurrency::numThreads);

    //#######################################################################

    // shadowShader = new Shader("res/shaders/directional_shadow_map.vert", "res/shaders/directional_shadow_map.frag", false);
    // OmniShadowShader = new Shader("res/shaders/omni_shadow_map.vert", "res/shaders/omni_shadow_map.geom", "res/shaders/omni_shadow_map.frag", false);
    GPU_MATRIXES = new gpu_vector_proxy<matrix>();

    __RENDERERS_in = new gpu_vector<__renderer>();
    __RENDERERS_in->ownStorage();


    __renderer_offsets = new gpu_vector<GLuint>();
    __renderer_offsets->ownStorage();
    __rendererMetas = new gpu_vector<__renderMeta>();
    __rendererMetas->ownStorage();
    initTransform();

    // initParticles();
    // particle_renderer::init();
    // lighting::init();

    // _atomics = new gpu_vector<uint>();
    _block_sums = new gpu_vector<GLuint>();
    _histo = new gpu_vector<GLuint>();
}

Shader matProgram;
void updateTransforms()
{

    tbb::parallel_for(
        tbb::blocked_range<unsigned int>(0, concurrency::numThreads, 1),
        [&](const tbb::blocked_range<unsigned int> &r) {
            for (unsigned int id = r.begin(); id < r.end(); ++id)
            {

                int numt = concurrency::numThreads;
                int step = Transforms.size() / concurrency::numThreads;
                int i = step * id;

                transformIdThreadcache[id][0].clear(); // pos
                transformIdThreadcache[id][1].clear(); // scl
                transformIdThreadcache[id][2].clear(); // rot

                positionsToBuffer[id].clear();
                rotationsToBuffer[id].clear();
                scalesToBuffer[id].clear();

                auto from = Transforms.updates.begin() + step * id;
                auto to = from + step;

                // transformIdThreadcache[id].reserve(step + 1);
                if (id == concurrency::numThreads - 1)
                    to = Transforms.updates.end();
                while (from != to)
                {
                    if (from->pos)
                    {
                        from->pos = false;
                        transformIdThreadcache[id][0].emplace_back(i);
                        positionsToBuffer[id].emplace_back(((transform2)i).getPosition());
                    }
                    if (from->rot)
                    {
                        from->rot = false;
                        transformIdThreadcache[id][1].emplace_back(i);
                        rotationsToBuffer[id].emplace_back(((transform2)i).getRotation());
                    }
                    if (from->scl)
                    {
                        from->scl = false;
                        transformIdThreadcache[id][2].emplace_back(i);
                        scalesToBuffer[id].emplace_back(((transform2)i).getScale());
                    }
                    ++from;
                    ++i;
                }
            }
        });

    //################################################################

    matProgram.use();
    GPU_TRANSFORMS->grow(Transforms.size());
    transformIds->bindData(6);
    GPU_TRANSFORMS->bindData(0);
    gpu_position_updates->bindData(8);
    gpu_rotation_updates->bindData(9);
    gpu_scale_updates->bindData(10);

    matProgram.setInt("stage", -2); // positions
    for (int i = 0; i < concurrency::numThreads; i++)
    {
        transformIds->bufferData(transformIdThreadcache[i][0]);
        gpu_position_updates->bufferData(positionsToBuffer[i]);
        matProgram.setUint("num", transformIdThreadcache[i][0].size());
        glDispatchCompute(transformIdThreadcache[i][0].size() / 64 + 1, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
    }

    matProgram.setInt("stage", -3); // rotations
    for (int i = 0; i < concurrency::numThreads; i++)
    {
        transformIds->bufferData(transformIdThreadcache[i][1]);
        gpu_rotation_updates->bufferData(rotationsToBuffer[i]);
        matProgram.setUint("num", transformIdThreadcache[i][1].size());
        glDispatchCompute(transformIdThreadcache[i][1].size() / 64 + 1, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
    }

    matProgram.setInt("stage", -4); // scales
    for (int i = 0; i < concurrency::numThreads; i++)
    {
        transformIds->bufferData(transformIdThreadcache[i][2]);
        gpu_scale_updates->bufferData(scalesToBuffer[i]);
        matProgram.setUint("num", transformIdThreadcache[i][2].size());
        glDispatchCompute(transformIdThreadcache[i][2].size() / 64 + 1, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
    }
}
void updateRenderers()
{
    int __renderersSize = 0;
    // int __rendererOffsetsSize = 0;
    __renderer_offsets->storage->clear();
    __rendererMetas->storage->clear();
    __renderMeta rm;
    // rm.min = 0;
    // rm.max = 1e32f;

    for (auto &i : renderingManager::shader_model_vector)
    {
        for (auto &j : i.second)
        {
            __renderer_offsets->storage->push_back(__renderersSize);
            rm.radius = j.second->m.meta()->radius;
            rm.isBillboard = j.second->isBillboard;
            rm.min = j.second->minRadius;
            rm.max = j.second->maxRadius;
            __rendererMetas->storage->push_back(rm);
            __renderersSize += j.second->ids.size();
        }
    }
    __RENDERERS_in->storage->resize(__renderersSize);

    tbb::parallel_for(
        tbb::blocked_range<unsigned int>(0, concurrency::numThreads, 1),
        [&](const tbb::blocked_range<unsigned int> &r) {
            for (unsigned int id = r.begin(); id < r.end(); ++id)
            {
                int __rendererId = 0;
                int __rendererOffset = 0;
                typename vector<__renderer>::iterator __r = __RENDERERS_in->storage->begin();

                for (auto &i : renderingManager::shader_model_vector)
                {
                    for (auto &j : i.second)
                    {
                        int step = j.second->ids.size() / concurrency::numThreads;
                        typename deque<GLuint>::iterator from = j.second->ids.data.begin() + step * id;
                        // typename deque<bool>::iterator v = j.second->ids.valid.begin() + step * id;
                        typename deque<GLuint>::iterator to = from + step;
                        __r = __RENDERERS_in->storage->begin() + __rendererOffset + step * id;
                        if (id == concurrency::numThreads - 1)
                        {
                            to = j.second->ids.data.end();
                        }
                        while (from != to)
                        {
                            // if (*v)
                            // {
                                __r->transform = *from;
                                __r->id = __rendererId;
                                ++__r;
                            // }
                            ++from;
                            // ++v;
                        }
                        ++__rendererId;
                        __rendererOffset += j.second->ids.size();
                    }
                }
                // for (auto &i : batchManager::batches.back())
                // {
                // 	for (auto &j : i.second)
                // 	{
                // 		for (auto &k : j.second)
                // 		{
                // 			int step = k.first->ids.size() / concurrency::numThreads;
                // 			typename deque<GLuint>::iterator from = k.first->ids.data.begin() + step * id;
                // 			typename deque<GLuint>::iterator to = from + step;
                // 			__r = __RENDERERS_in->storage->begin() + __rendererOffset + step * id;
                // 			if (id == concurrency::numThreads - 1)
                // 			{
                // 				to = k.first->ids.data.end();
                // 			}
                // 			while (from != to)
                // 			{
                // 				__r->transform = *from;
                // 				__r->id = __rendererId;
                // 				++from;
                // 				++__r;
                // 			}
                // 			++__rendererId;
                // 			__rendererOffset += k.first->ids.size();
                // 		}
                // 	}
                // }
            }
        });

    //##########################################################
    __RENDERERS_in->bufferData();
}
void renderCameras()
{
}