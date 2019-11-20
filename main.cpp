#include "game_engine.h"
#include <fstream>
#include <iostream>
#include <string>

atomic<int> numCubes(0);
game_object* proto = nullptr;


class player_sc :public component {
    bool cursorReleased = false;
    float speed = 30.f;
public:
	void update() {
		transform->rotate(glm::vec3(0, 1, 0), Input.Mouse.getX() * -0.01f);
		transform->rotate(glm::vec3(1, 0, 0), Input.Mouse.getY() * -0.01f);
		transform->rotate(glm::vec3(0, 0, 1), (Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * Time.deltaTime * -1.f);
		transform->translate(glm::vec3(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D), Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT), Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
		 cout << "\rcubes: " << numCubes << " \tfps: " << 1.f / Time.unscaledSmoothDeltaTime << "                  ";

		 if(Input.getKeyDown(GLFW_KEY_R)){
            speed *= 2;
		 }
		 else if(Input.getKeyDown(GLFW_KEY_F)){
            speed /= 2;
		 }
//
//		 if(Input.getKeyDown(GLFW_KEY_ESCAPE) && cursorReleased){
//            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//		 }else if(Input.getKeyDown(GLFW_KEY_ESCAPE) && !cursorReleased){
//            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_ENABLED);
//		 }

	}
	UPDATE(player_sc, update);
	COPY(player_sc);
};
class cube_sc : public component {
public:
	glm::vec3 dir;
	glm::vec3 rot;
	array_heap<_transform>::ref T;
	cube_sc() {}
	void onStart() {
		dir = randomSphere();
		rot = randomSphere();
		T = transform->_T;
	}
	void update() {
		transform->translate(dir * Time.deltaTime * 10.f);
		transform->rotate(rot, Time.deltaTime * glm::radians(100.f));
		if (Input.getKey(GLFW_KEY_C) && proto != transform->gameObject && randf() < 30000.f / numCubes * Time.deltaTime) {
//		if (Input.getKey(GLFW_KEY_C) && proto != transform->gameObject && randf() < 0.01f) {
			numCubes.fetch_add(-1);
			transform->gameObject->destroy();
		}
	}
	UPDATE(cube_sc, update);
	COPY(cube_sc);
};

class nbody : public component{

    glm::vec3 vel;

    void update(){
        deque<nbody*>& v = COMPONENT_LIST(nbody)->data.data;
        glm::vec3 acc{0};
        for(auto& i : v){
            glm::vec3 dir = i->transform->getPosition() - transform->getPosition();
            float r = glm::length2(dir) - 0.5f;
            if(r < -0.1f || r > 5000.f)
                continue;
            acc += 10.f * glm::normalize(dir) / r;
        }
        vel += acc * Time.deltaTime;
        transform->move(vel * Time.deltaTime);
    }
    UPDATE(nbody, update);
	COPY(nbody);
};

class moreCUBES : public component {

public:
	float rof = 30000;
	void update() {
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_1)) {
			float r = rof * Time.deltaTime;
			for (int i = 0; i < r; i++) {
				game_object* go = new game_object(*proto);
//				go->getComponent<cube_sc>()->dir = transform->forward() * 2.f + randomSphere() * 0.6f;
				go->transform->setPosition(transform->getPosition() + transform->forward() * 5.f - transform->up() * 2.5f);
				numCubes.fetch_add(1);

			}
		}
	}
	UPDATE(moreCUBES, update);
	COPY(moreCUBES);
};
int main(void)
{
    init();
    _shader modelShader("res/shaders/model.vert", "res/shaders/model.frag");
	_model cubeModel("res/models/cube/cube.obj");
	_model nanoSuitModel("res/models/nanosuit/nanosuit.obj");
//	waitForRenderQueue();

	player = new game_object();
	player->addComponent<player_sc>();
	player->addComponent<moreCUBES>();

    ifstream config("config.txt");
    int n;
    config >> n;
	numCubes = n;
	srand(100);

	game_object* CUBE = new game_object();
	CUBE->addComponent<_renderer>();
	CUBE->addComponent<cube_sc>();
	CUBE->getComponent<_renderer>()->set(modelShader, cubeModel);
	//gameObjects.front()->addComponent<mvpSolver>();

	proto = CUBE;

	game_object* go = new game_object(*CUBE);
	for (int i = 0; i < n; i++) {
        go = new game_object(*go);
		go->transform->translate(randomSphere() * 20.f);
		if(i % (n / 100) == 0)
            cout << "\r" << (float)i / (float)n << "    " << flush;
	}
	auto nanosuitMan = new game_object(*CUBE);
	nanosuitMan->getComponent<_renderer>()->set(modelShader, nanoSuitModel);
	nanosuitMan->removeComponent(nanosuitMan->getComponent<cube_sc>());
	nanosuitMan->transform->move(glm::vec3(-10.f));

	game_object* proto2 = new game_object(*CUBE);
	proto2->removeComponent<cube_sc>();
	proto2->transform->setScale(glm::vec3(10.f));
	proto2->transform->translate(glm::vec3(10.f) * 0.5f);
	for(int i = 0; i < 15; ++i){
        proto2 = new game_object(*proto2);
        proto2->transform->setScale(glm::vec3(pow(10.f,(float)(i + 1))));
        proto2->transform->translate(glm::vec3(pow(10.f,(float)(i + 1))) * 2.f);
	}

    run();

    return 0;
}
