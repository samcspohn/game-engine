#include "Input.h"
#include <stdio.h>
#include <string.h>
#include "imgui/imgui.h"
using namespace std;

bool _Input::getKey(unsigned int key)
{
	return ImGui::IsKeyDown(key);
	// return ImGui::GetIO().KeysDown[key];
	// return keys[key];
}
bool _Input::getKeyDown(unsigned int key)
{
	return ImGui::IsKeyPressed(key,false);
	// return ImGui::GetIO().KeysDown[key];
	// return keyDowns[key];
}

float _Mouse::getX()
{
	return ImGui::GetIO().MouseDelta.x;
	// return xOffset;
}
float _Mouse::getY()
{
	return ImGui::GetIO().MouseDelta.y;
	// return yOffset;
}
bool _Mouse::getButton(unsigned int button)
{
	return mouseButtons[button];
}
float _Mouse::getScroll()
{
	return ImGui::GetIO().MouseWheel;
	// return mouseScroll;
}

void _Input::resetKeyDowns()
{
	memset(keyDowns, 0, sizeof(bool) * 1024);
}

_Input Input;