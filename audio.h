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
ALfloat ListenerPos[] = { 0.0, 0.0, 0.0 };
 
// Velocity of the Listener.
ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
 
// Orientation of the Listener. (first 3 elements are "at", second 3 are "up")
// Also note that these should be units of '1'.
ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };
namespace audioManager{
    map<string, audioMeta*> audios;
    void init(){
        alutInit(NULL, 0);
        alListenerfv(AL_POSITION,    ListenerPos);
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
    audioMeta* a = 0;
};

mutex audiom;
 
class audiosource : public component{
    ALuint Source = -1;
    ALfloat SourcePos[3] = { 0.0, 0.0, 0.0 };
    ALfloat SourceVel[3] = { 0.0, 0.0, 0.0 };
    ALfloat pitch = 1;
    audio a;
public:
    COPY(audiosource);
    audiosource(){
        Source = -1;
        a.a = 0;
    }
    audiosource(const audiosource& as){
        Source = -1;
        a = as.a;
    }
    void setPitch(float p){
        pitch = p;
        alSourcef(Source, AL_PITCH, pitch);
    }
    void play(){
        alSourcePlay(Source);
    }
    void play(glm::vec3 pos){
        SourcePos[0] = pos.x;
        SourcePos[1] = pos.y;
        SourcePos[2] = pos.z;
        alSourcefv(Source, AL_POSITION, SourcePos);
        alSourcef(Source, AL_GAIN, glm::min(1.0, 300.0 / length(pos)));
        alSourcePlay(Source);
    }
    bool isPlaying(){
        ALint isp;
        alGetSourcei(Source,AL_SOURCE_STATE,&isp);
        return (bool)isp;
    }
    void set(audio& _a){
        this->a = _a;
        if(Source != -1){
            alDeleteSources(1, &Source);
        }
        alGenSources(1, &Source);
        if(alGetError() != AL_NO_ERROR)
            cout << "could not gen source" << endl;
        alSourcei(Source, AL_BUFFER, a.a->Buffer);
        alSourcef(Source, AL_PITCH, pitch);
        alSourcef(Source, AL_GAIN, 0.7);
        alSourcefv(Source, AL_POSITION, SourcePos);
        alSourcefv(Source, AL_VELOCITY, SourceVel);
        alSourcei(Source, AL_LOOPING, a.a->loop);

    }
    void onStart(){
        if(a.a != 0)
            set(this->a);
    }
    void onDestroy(){
        if(Source != -1){
            alDeleteSources(1, &Source);
        }
    }
};