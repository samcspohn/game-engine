#include "game_engine.h"

using namespace std;

struct particle_tester : public component{

    int num_spawners;
    game_object_prototype p1;
    game_object_prototype p2;
    void onStart(){
        for(int i = 0; i < num_spawners; i++){
            auto g1 = instantiate(p1);
            g1->transform->setPosition(randomSphere() * 100.f);
            auto g2 = instantiate(p2);
            g2->transform->setPosition(randomSphere() * 100.f);
        }
    }
    SER_FUNC(){
        SER(num_spawners)
        
    }
};
REGISTER_COMPONENT(particle_tester)