#include "components/Component.h"
#include "components/game_object.h"
#include <glm/glm.hpp>
#include <string>
#include <map>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include "editor.h"
#include "serialize.h"

using namespace std;

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
    // friend audioMeta *audioManager::_new(string);
    string type();
    audioMeta();
    void load(const string &_file);
    audioMeta(string _file);
    ~audioMeta();
    bool onEdit();
};
class audioManager : public assets::assetManager<audioMeta>
{
public:
    void init();

    audioMeta *_new(string file);
    void updateListener(glm::vec3 pos);
    void destroy();
};
extern audioManager audio_manager;

struct audio : public assets::asset_instance<audioMeta>
{
    audio();
    audioMeta *meta() const;
    audio(string file);
    void play(glm::vec3 pos, float pitch, float gain);
    int a = -1;
};

void renderEdit(const char *name, audio &a);
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
    audiosource();
    audiosource(const audiosource &as);
    void setPitch(float p);
    void play();
    void play(glm::vec3 pos);

    bool _isPlaying();
    void set(audio &_a);
    void onStart();
    void onDestroy();
    SER_FUNC()
    {

        SER(a)
    }
};
namespace YAML
{

    template <>
    struct convert<audio>
    {
        static Node encode(const audio &rhs)
        {
            Node node;
            node["id"] = rhs.a;
            return node;
        }

        static bool decode(const Node &node, audio &rhs)
        {
            rhs.a = node["id"].as<int>();
            return true;
        }
    };
}