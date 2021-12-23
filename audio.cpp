#include "audio.h"
#include "camera.h"
using namespace std;

// class audioMeta;
// namespace audioManager
// {
//     audioMeta *_new(string);
// }

void audioMeta::inspect()
{
    renderEdit("path", file);
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("FILE_DRAG_AND_DROP.wav"))
        {
            // IM_ASSERT(payload->DataSize == sizeof(string*));
            string payload_n = string((const char *)payload->Data);
            cout << "file payload:" << payload_n << endl;
            file = payload_n;
        }
        ImGui::EndDragDropTarget();
    }
    if (ImGui::Button("reload"))
    {
        // do something
    }
}
string audioMeta::type()
{
    return "AUDIO_DRAG_AND_DROP";
}
audioMeta::audioMeta() = default;
void audioMeta::load(const string &_file)
{
    this->file = _file;
    alGenBuffers(1, &Buffer);
    alutLoadWAVFile((ALbyte *)file.c_str(), &format, &data, &size, &freq, &loop);
    alBufferData(Buffer, format, data, size, freq);
    alutUnloadWAV(format, data, size, freq);
}
audioMeta::audioMeta(string _file)
{
    load(_file);
}
audioMeta::~audioMeta()
{
    alDeleteBuffers(1, &Buffer);
}
bool audioMeta::onEdit()
{
    // char input[1024];
    // sprintf(input, name.c_str());
    // if (ImGui::InputText("", input, 1024, ImGuiInputTextFlags_None))
    //     name = {input};
    // ImGui::PopID();
    // ImGui::PopItemWidth();
    // ImGui::Button(name.c_str(), {40, 40});
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        // Set payload to carry the index of our item (could be anything)
        ImGui::SetDragDropPayload("AUDIO_DRAG_AND_DROP", &id, sizeof(int));
        ImGui::EndDragDropSource();
    }
    return false;
}


// Position of the Listener.
glm::vec3 ListenerPos;

// Velocity of the Listener.
ALfloat ListenerVel[] = {0.0, 0.0, 0.0};

// Orientation of the Listener. (first 3 elements are "at", second 3 are "up")
// Also note that these should be units of '1'.
ALfloat ListenerOri[] = {0.0, 0.0, -1.0, 0.0, 1.0, 0.0};

void audioManager::init()
{
    alutInit(NULL, 0);
    alListenerfv(AL_POSITION, glm::value_ptr(ListenerPos));
    alListenerfv(AL_VELOCITY, ListenerVel);
    alListenerfv(AL_ORIENTATION, ListenerOri);

    // error checking omitted for brevity
    ALCdevice *device = alcOpenDevice(NULL);
    ALCcontext *context = alcCreateContext(device, NULL);
    ALCint size;
    alcGetIntegerv(device, ALC_ATTRIBUTES_SIZE, 1, &size);
    std::vector<ALCint> attrs(size);
    alcGetIntegerv(device, ALC_ALL_ATTRIBUTES, size, &attrs[0]);
    for (size_t i = 0; i < attrs.size(); ++i)
    {
        if (attrs[i] == ALC_MONO_SOURCES)
        {
            std::cout << "max mono sources: " << attrs[i + 1] << std::endl;
        }
    }
}

audioMeta *audioManager::_new(string file)
{
    audioMeta *am = new audioMeta(file);
    int id = am->genID();
    path[file] = id;
    meta[id] = shared_ptr<audioMeta>(am);
    return am;
}
void audioManager::updateListener(glm::vec3 pos)
{
    ListenerPos = pos;
    alListenerfv(AL_POSITION, glm::value_ptr(ListenerPos));
    alListenerfv(AL_VELOCITY, ListenerVel);
    alListenerfv(AL_ORIENTATION, ListenerOri);
}
void audioManager::destroy()
{
    // for (auto &i : meta)
    // {
    //     delete i.second;
    // }
    meta.clear();
    alutExit();
}

audioManager audio_manager;

audio::audio() {}
audioMeta *audio::meta() const
{
    return audio_manager.meta[a].get();
}
audio::audio(string file)
{
    auto am = audio_manager.path.find(file);
    if (am == audio_manager.path.end())
    {
        audio_manager._new(file);
        am = audio_manager.path.find(file);
    }
    a = am->second;
}

void renderEdit(const char *name, audio &a)
{
    // ImGui::DragInt(name,&i);
    if (a.a == -1) // uninitialized
        ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
    else
        ImGui::InputText(name, (char *)a.meta()->name.c_str(), a.meta()->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("AUDIO_DRAG_AND_DROP"))
        {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int *)payload->Data;
            a.a = payload_n;
        }
        ImGui::EndDragDropTarget();
    }
}

audioSource::audioSource()
{
    alGenSources(1, &Source);
}
audioSource::audioSource(const audioSource &as)
{
    alGenSources(1, &Source);
}
void audioSource::play()
{
    alSourcePlay(Source);
}
void audioSource::stop()
{
    // if(isPlaying != 0){
    // *isPlaying = false;
    alSourceStop(Source);
    // }
    // isPlaying = 0;
}
void audioSource::play(glm::vec3 pos, audio &a, float pitch, float gain)
{
    audioMeta *_a = audio_manager.meta[a.a].get();
    alSourcei(Source, AL_BUFFER, _a->Buffer);
    alSourcef(Source, AL_PITCH, pitch);
    alSourcef(Source, AL_GAIN, gain * 100 / glm::length(pos));
    alSourcefv(Source, AL_POSITION, glm::value_ptr(pos));
    alSourcefv(Source, AL_VELOCITY, ListenerVel);
    alSourcei(Source, AL_LOOPING, _a->loop);

    // alSourcefv(Source, AL_POSITION, glm::value_ptr(pos));
    // alSourcef(Source, AL_GAIN, glm::min(1.0, 300.0 / length(pos)));
    alSourcePlay(Source);
}
bool audioSource::_isPlaying()
{
    ALint isp;
    alGetSourcei(Source, AL_SOURCE_STATE, &isp);
    // *isPlaying = (bool)isp;
    return (bool)isp;
}

namespace audioSourceManager
{
    mutex audio_m;

    int maxSources;
    std::deque<audioSource> sources;
    std::deque<audioSource *> inUse;
    std::set<audioSource *> notInUse;

    void init()
    {
        while (true)
        {
            auto as = audioSource();
            if (alGetError() != AL_NO_ERROR)
            {
                break;
            }
            sources.push_back(as);
            notInUse.emplace(&sources.back());
        }
        maxSources = sources.size();
    }

    audioSource *getSource(atomic<bool> *playing)
    {
        audio_m.lock();
        if (inUse.size() >= maxSources)
        {
            inUse.front()->stop();
            notInUse.emplace(inUse.front());
            inUse.pop_front();
        }
        audioSource *ret;
        ret = *notInUse.begin();
        ret->isPlaying = playing;
        notInUse.erase(ret);
        inUse.push_back(ret);
        audio_m.unlock();
        return ret;
    }
    audioSource *getSource()
    {
        audio_m.lock();
        if (inUse.size() >= maxSources)
        {
            inUse.front()->stop();
            notInUse.emplace(inUse.front());
            inUse.pop_front();
        }
        audioSource *ret;
        ret = *notInUse.begin();
        ret->isPlaying = 0;
        notInUse.erase(ret);
        inUse.push_back(ret);
        audio_m.unlock();
        return ret;
    }
} // namespace audioSourceManager

void audiosource::onEdit()
{
    RENDER(pitch);
    RENDER(gain);
}

audiosource::audiosource() : isPlaying(false){};
audiosource::audiosource(const audiosource &as) : isPlaying(false), a(as.a){};
void audiosource::setPitch(float p)
{
    pitch = p;
}
void audiosource::play()
{
    auto s = audioSourceManager::getSource(&isPlaying);
    s->play(transform->getPosition(), a, pitch, glm::min(1.0, 10.0 / length(transform->getPosition() - ListenerPos)) * gain);
    // alSourcePlay(Source);
}
void audiosource::play(glm::vec3 pos)
{
    auto s = audioSourceManager::getSource(&isPlaying);
    s->play(pos, a, pitch, glm::min(1.0, 10.0 / length(pos - ListenerPos)) * gain);
}

bool audiosource::_isPlaying()
{
    return isPlaying;
}
void audiosource::set(audio &_a)
{
    this->a = _a;
}
void audiosource::onStart()
{
    if (a.a != 0)
        set(this->a);
}
void audiosource::onDestroy()
{
}

// BOOST_CLASS_EXPORT(componentStorage<audiosource>)

void audio::play(glm::vec3 pos, float pitch, float gain)
{
    auto s = audioSourceManager::getSource();
    s->play(pos - COMPONENT_LIST(_camera)->get(0)->c->pos, *this, pitch, gain);
}
