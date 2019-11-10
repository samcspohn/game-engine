#include "game_engine.h"
#include <fstream>
#include <iostream>
#include <string>

atomic<int> numCubes(0);
game_object* proto = nullptr;


class player_sc :public component {
public:
	void update() {
		transform->rotate(glm::vec3(0, 1, 0), Input.Mouse.getX() * -0.2f);
		transform->rotate(glm::vec3(1, 0, 0), Input.Mouse.getY() * -0.2f);
		transform->rotate(glm::vec3(0, 0, 1), (Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * Time.deltaTime * -100.f);
		transform->translate(glm::vec3(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D), Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT), Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * 30.0f);
		 cout << "\rcubes: " << numCubes << " \tfps: " << 1.f / Time.unscaledSmoothDeltaTime << "                  ";

	}
	UPDATE(player_sc, update);
	COPY(player_sc);
};
class cube_sc : public component {
public:
	glm::vec3 dir;
	glm::vec3 rot;
	cube_sc() {}
	void onStart() {
		dir = randomSphere();
		rot = randomSphere();
	}
	void update() {
		transform->translate(dir * Time.deltaTime * 10.f);
		transform->rotate(rot, Time.deltaTime * 100.f);
		if (Input.getKey(GLFW_KEY_C) && proto != transform->gameObject && randf() < 10000.f / numCubes * Time.deltaTime) {
//		if (Input.getKey(GLFW_KEY_C) && proto != transform->gameObject && randf() < 0.01f) {
			numCubes.fetch_add(-1);
			transform->gameObject->destroy();
		}
	}
	UPDATE(cube_sc, update);
	COPY(cube_sc);
};
class moreCUBES : public component {

public:
	float rof = 30000;
	void update() {
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_1)) {
			float r = rof * Time.deltaTime;
			for (int i = 0; i < r; i++) {
				game_object* go = new game_object(*proto);
				go->getComponent<cube_sc>()->dir = transform->forward() * 2.f + randomSphere() * 0.6f;
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


    for(int i = 0; i < 10; i++)
        cout << randf() << endl;

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
	}
	auto nanosuitMan = new game_object(*CUBE);
	nanosuitMan->getComponent<_renderer>()->set(modelShader, nanoSuitModel);
	cout << "here" << endl;

    run();

    return 0;
}
