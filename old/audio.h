#include "Component.h"
#include "game_object.h"
#include <glm/glm.hpp>
#include <string>
#include <map>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include "editor.h"
#include "serialize.h"

using namespace std;

class audioMeta;
namespace audioManager
{
    audioMeta *_new(string);
}
struct audioMeta : public assets::asset
{
    ALuint Buffer;
    // Variables to load into.
    ALenum format;
    ALsizei size;
    ALvoid *data;
    ALsizei freq;
    ALboolean loop;
    string file;
    void inspect();
    friend class audio;
    friend audioMeta *audioManager::_new(string);
    string type();
    audioMeta();
    void load(const string &_file);
    audioMeta(string _file);
    ~audioMeta();
    bool onEdit();
    SER_HELPER()
    {
        SER_BASE_ASSET
    }
};
// REGISTER_ASSET(audioMeta);

// Position of the Listener.
// extern glm::vec3 ListenerPos;

// Velocity of the Listener.
// ALfloat ListenerVel[] = {0.0, 0.0, 0.0};

// Orientation of the Listener. (first 3 elements are "at", second 3 are "up")
// Also note that these should be units of '1'.
// ALfloat ListenerOri[] = {0.0, 0.0, -1.0, 0.0, 1.0, 0.0};
namespace audioManager
{
    extern map<string, audioMeta *> audios;
    extern map<int, audioMeta *> audios_ids;

    void init();

    audioMeta *_new(string file);
    void updateListener(glm::vec3 pos);
    void destroy();
    void save(OARCHIVE &oa);
    void load(IARCHIVE &ia);
} // namespace audioManager

struct audio
{
    audio();
    audioMeta *meta();
    audio(string file);
    void play(glm::vec3 pos, float pitch, float gain);
    int a = -1;
    SER_HELPER()
    {
        ar &a;
    }
};

void renderEdit(const char* name, audio& a);
class audioSource
{
public:
    ALuint Source = -1;
    atomic<bool> *isPlaying;
    audioSource();
    audioSource(const audioSource &as);
    void play();
    void stop();
    void play(glm::vec3 pos, audio &a, float pitch, float gain);
    bool _isPlaying();
};

namespace audioSourceManager
{
//     mutex audio_m;

//     int maxSources;
//     std::deque<audioSource> sources;
//     std::deque<audioSource *> inUse;
//     std::set<audioSource *> notInUse;

    void init();

    audioSource *getSource(atomic<bool> *playing);
    audioSource *getSource();
} // namespace audioSourceManager

class audiosource : public component
{

    ALfloat SourcePos[3] = {0.0, 0.0, 0.0};
    ALfloat SourceVel[3] = {0.0, 0.0, 0.0};
    audio a;
    atomic<bool> isPlaying = {false};

public:
    ALfloat pitch = 1;
    ALfloat gain = 1;
    void onEdit();
    COPY(audiosource);
    audiosource();
    audiosource(const audiosource &as);
    void setPitch(float p);
    void play();
    void play(glm::vec3 pos);

    bool _isPlaying();
    void set(audio &_a);
    void onStart();
    void onDestroy();
    SER1(a);
};
// BOOST_CLASS_EXPORT(componentStorage<audiosource>)
// REGISTER_COMPONENT(audiosource)

// void audio::play(glm::vec3 pos, float pitch, float gain);
