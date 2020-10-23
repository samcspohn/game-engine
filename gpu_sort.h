#include "Shader.h"
#include "gpu_vector.h"
// #include "Shaderinclude.h"
#include <fstream>

// extern gpu_vector<uint> *_atomics;
extern gpu_vector<GLuint> *_block_sums;
extern gpu_vector<GLuint> *_histo;

template <typename t>
struct sorter
{
    Shader shader;
    sorter(){}
    sorter(string element, string elementStruct, string key)
    {
        cout << "\ncreating sort kernel for " + element + "\n";
        std::string code;
        code = shaderLoader::load("res/shaders/sort.glsl");
        int index = code.find("\n");

        code.replace(code.begin() + index, code.begin() + index, "\n" + elementStruct + "\n");

        index = code.find("_K_");
        code.replace(code.begin() + index, code.begin() + index + 3, key);

        index = code.find("_T_");
        while (index != -1)
        {
            code.replace(code.begin() + index, code.begin() + index + 3, element);
            index = code.find("_T_", index);
        }
        ofstream f(element + "_sort.glsl");
        f << code;
        f.close();
        vector<GLuint> shaders;
        shaders.push_back(shader.loadFromString(code, GL_COMPUTE_SHADER));
        shader.compileShader(shaders);
    }

    void _sort(int count)
    {

        /////////////////////////////////
        // if (_atomics->size() == 0)
        //     _atomics->storage->push_back(0);

        _block_sums->ownStorage();
        _block_sums->storage->resize(256);
        _histo->ownStorage();
        _histo->storage->resize(65536);


        _block_sums->bindData(2);
        _histo->bindData(3);
        
        _histo->bufferData();
        _block_sums->bufferData();
        

        /////////////////////////////////
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);
        // gpuTimer gt2;
        shader.use();

        // gt2.start();
        shader.setInt("stage", -1);
        uint subSortGroups = ceil(count / 8) / 256 + 1;
        shader.setUint("count", subSortGroups * 256);
        shader.setUint("nkeys", count);
        glDispatchCompute(subSortGroups, 1, 1); // count
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        // appendStat("sort particle list stage 0", gt2.stop());


        shader.setInt("stage", 0);
        shader.setUint("count", count);
        glDispatchCompute(count / 256 + 1, 1, 1); // count
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        // gt2.start();
        shader.setInt("stage", 1);
        shader.setUint("count", 256);
        glDispatchCompute(256 / 256, 1, 1); // count
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        shader.setInt("stage", 2);
        shader.setUint("count", 1);
        glDispatchCompute(1, 1, 1); // count
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        shader.setInt("stage", 3);
        shader.setUint("count", 65536);
        glDispatchCompute(65536 / 256, 1, 1); // count
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        // appendStat("sort particle list stage 1,2,3", gt2.stop());

        // gt2.start();
        shader.setInt("stage", 4);
        shader.setUint("count", count);
        glDispatchCompute(count / 256 + 1, 1, 1); // count
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        // appendStat("sort particle list stage 4", gt2.stop());
    }
    void sort(int count, gpu_vector<t> *_data_in, gpu_vector_proxy<t> *_data_out)
    {

        // _atomics->ownStorage();
// 
        _data_in->bindData(0);
        _data_in->bufferData();

        _data_out->bindData(1);
        _sort(count);
    }

    void sort(int count, gpu_vector_proxy<t> *_data_in, gpu_vector_proxy<t> *_data_out)
    {

        // shader.use();
        // _atomics->ownStorage();

        _data_in->bindData(0);

        _data_out->bindData(1);
        _sort(count);
    };
};
