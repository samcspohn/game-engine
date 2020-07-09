#include "game_engine.h"
// #include "initMain.h"
#include <fstream>
#include <iostream>
#include <string>

#include <iomanip>
#include <locale>

using namespace glm;
using namespace std;

class player_sc : public component {
public:
    bool cursorReleased = false;
    void update(){
        transform->translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * 3.f);
		transform->translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * 3.f);
		transform->translate(glm::vec3(0, 1, 0) * (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * 3.f);
        transform->rotate(glm::vec3(0, 0, 1), (float)(Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * -Time.deltaTime);
        transform->rotate(vec3(0,1,0), Input.Mouse.getX() * Time.unscaledDeltaTime * -0.4f);
        transform->rotate(vec3(1,0,0), Input.Mouse.getY() * Time.unscaledDeltaTime * -0.4f);

        if (Input.getKeyDown(GLFW_KEY_ESCAPE) && cursorReleased)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			cursorReleased = false;
		}
		else if (Input.getKeyDown(GLFW_KEY_ESCAPE) && !cursorReleased)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			cursorReleased = true;
		}
    }
    COPY(player_sc);
    
};


int main(int argc, char **argv)
{
    
	// hideMouse = false;
    ::init();

	_shader modelShader("res/shaders/model.vert", "res/shaders/model.frag");
	_model cubeModel("res/models/cube/cube.obj");
	_model nanoSuitModel("res/models/nanosuit/nanosuit.obj");

    game_object* player = new game_object();
	auto playerCam = player->addComponent<_camera>();
	playerCam->fov = 80;
	playerCam->farPlane = 1000.f;
    player->addComponent<player_sc>();

    auto cube = new game_object();
    cube->addComponent<_renderer>()->set(modelShader,cubeModel);
    cube->transform->scale(vec3(20));
    cube->transform->move(vec3(0,-21,0));

    for(int i = 0; i < 40; i++){
        cube = new game_object();
        cube->addComponent<_renderer>()->set(modelShader,cubeModel);
        cube->transform->translate(randomSphere() * glm::sqrt(randf()) * 15.0f);
        cube->transform->rotate(randomSphere(), randf() * 5.0f);
    }
	run();

}