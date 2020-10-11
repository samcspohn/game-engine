#include "Component.h"
#include "game_object.h"
#include <glm/glm.hpp>
#include <string>
#include <map>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
 
using namespace std;

struct audioMeta{
    ALuint Buffer;
    // Variables to load into.
    ALenum format;
    ALsizei size;
    ALvoid* data;
    ALsizei freq;
    ALboolean loop;
    audioMeta(string file){
        alGenBuffers(1, &Buffer);
        alutLoadWAVFile((ALbyte*)file.c_str(), &format, &data, &size, &freq, &loop);
        alBufferData(Buffer, format, data, size, freq);
        alutUnloadWAV(format, data, size, freq);
    }
    ~audioMeta(){
        alDeleteBuffers(1, &Buffer);
    }
};
// Position of the Listener.
glm::vec3 ListenerPos;
 
// Velocity of the Listener.
ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
 




// Orientation of the Listener. (first 3 elements are "at", second 3 are "up")
// Also note that these should be units of '1'.
ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };
namespace audioManager{
    map<string, audioMeta*> audios;

    void init(){
        alutInit(NULL, 0);
        alListenerfv(AL_POSITION,    glm::value_ptr(ListenerPos));
        alListenerfv(AL_VELOCITY,    ListenerVel);
        alListenerfv(AL_ORIENTATION, ListenerOri);

        //error checking omitted for brevity
        ALCdevice* device = alcOpenDevice(NULL);
        ALCcontext* context = alcCreateContext(device,NULL);
        ALCint size;
        alcGetIntegerv( device, ALC_ATTRIBUTES_SIZE, 1, &size);
        std::vector<ALCint> attrs(size);
        alcGetIntegerv( device, ALC_ALL_ATTRIBUTES, size, &attrs[0] );
        for(size_t i=0; i<attrs.size(); ++i)
        {
        if( attrs[i] == ALC_MONO_SOURCES )
        {
            std::cout << "max mono sources: " << attrs[i+1] << std::endl;
        }
        }
    }
    void updateListener(glm::vec3 pos){
        ListenerPos = pos;
        alListenerfv(AL_POSITION,    glm::value_ptr(ListenerPos));
        alListenerfv(AL_VELOCITY,    ListenerVel);
        alListenerfv(AL_ORIENTATION, ListenerOri);
    }
    void destroy(){
        for(auto& i : audios){
            delete i.second;
        }
    }
}

struct audio{
    audio(){}
    audio(string file){
        auto am = audioManager::audios.find(file);
        if(am != audioManager::audios.end()){
            a = am->second;
        }else{
            audioManager::audios[file] = new audioMeta(file);
            a = audioManager::audios.at(file);
        }
    }
    void play(vec3 pos, float pitch, float gain);
    audioMeta* a = 0;
};


class audioSource{
public:
    ALuint Source = -1;
    atomic<bool>* isPlaying;
    audioSource(){
         alGenSources(1, &Source);
    }
    audioSource(const audioSource& as){
        alGenSources(1, &Source);
    }
    void play(){
        alSourcePlay(Source);
    }
    void stop(){
        // if(isPlaying != 0){
            // *isPlaying = false;
            alSourceStop(Source);
        // }
        // isPlaying = 0;
    }
    void play(glm::vec3 pos, audio& a, float pitch, float gain){
        alSourcei(Source, AL_BUFFER, a.a->Buffer);
        alSourcef(Source, AL_PITCH, pitch);
        alSourcef(Source, AL_GAIN, gain);
        alSourcefv(Source, AL_POSITION, glm::value_ptr(pos));
        alSourcefv(Source, AL_VELOCITY, ListenerVel);
        alSourcei(Source, AL_LOOPING, a.a->loop);

        // alSourcefv(Source, AL_POSITION, glm::value_ptr(pos));
        // alSourcef(Source, AL_GAIN, glm::min(1.0, 300.0 / length(pos)));
        alSourcePlay(Source);
    }
    bool _isPlaying(){
        ALint isp;
        alGetSourcei(Source,AL_SOURCE_STATE,&isp);
        // *isPlaying = (bool)isp;
        return (bool)isp;
    }
};

namespace audioSourceManager {
    mutex audiom;

    int maxSources;
    std::deque<audioSource> sources;
    std::deque<audioSource*> inUse;
    std::set<audioSource*> notInUse;

    void init(){
        while(true)
        {
            auto as = audioSource();
            if(alGetError() != AL_NO_ERROR){
                break;
            }
            sources.push_back(as);
            notInUse.emplace(&sources.back());
        } 
        maxSources = sources.size();
    }

    audioSource* getSource(atomic<bool>* playing){
        audiom.lock();
        if(inUse.size() >= maxSources){
            inUse.front()->stop();
            notInUse.emplace(inUse.front());
            inUse.pop_front();
        }
        audioSource* ret;
        ret = *notInUse.begin();
        ret->isPlaying = playing;
        notInUse.erase(ret);
        inUse.push_back(ret);
        audiom.unlock();
        return ret;
    }
     audioSource* getSource(){
        audiom.lock();
        if(inUse.size() >= maxSources){
            inUse.front()->stop();
            notInUse.emplace(inUse.front());
            inUse.pop_front();
        }
        audioSource* ret;
        ret = *notInUse.begin();
        ret->isPlaying = 0;
        notInUse.erase(ret);
        inUse.push_back(ret);
        audiom.unlock();
        return ret;
    }
}
 

class audiosource : public component{

    ALfloat SourcePos[3] = { 0.0, 0.0, 0.0 };
    ALfloat SourceVel[3] = { 0.0, 0.0, 0.0 };
    audio a;
    atomic<bool> isPlaying = {false};
public:
    ALfloat pitch = 1;
    ALfloat gain = 1;
    COPY(audiosource);
    audiosource() : isPlaying(false) {};
    audiosource(const audiosource& as) : isPlaying(false), a(as.a) {};
    void setPitch(float p){
        pitch = p;
    }
    void play(){
        auto s = audioSourceManager::getSource(&isPlaying);
        s->play(transform->getPosition(),a,pitch,glm::min(1.0, 10.0 / length(transform->getPosition() - ListenerPos)) * gain);
        // alSourcePlay(Source);
    }
    void play(glm::vec3 pos){
        auto s = audioSourceManager::getSource(&isPlaying);
        s->play(pos,a,pitch,glm::min(1.0, 10.0 / length(pos - ListenerPos)) * gain);

    }
    
    bool _isPlaying(){
        return isPlaying;
    }
    void set(audio& _a){
        this->a = _a;
    }
    void onStart(){
        if(a.a != 0)
            set(this->a);
    }
    void onDestroy(){

    }
};

void audio::play(vec3 pos, float pitch, float gain){
    auto s = audioSourceManager::getSource();
    s->play(pos,*this,pitch,gain);
}

