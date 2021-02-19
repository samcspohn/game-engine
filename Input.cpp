#include "Input.h"
#include <stdio.h>
#include <string.h>
using namespace std;

bool _Input::getKey(unsigned int key)
{
	return keys[key];
}
bool _Input::getKeyDown(unsigned int key)
{
	return keyDowns[key];
}

float _Mouse::getX() { return xOffset; }
float _Mouse::getY() { return yOffset; }
bool _Mouse::getButton(unsigned int button)
{
	return mouseButtons[button];
}
float _Mouse::getScroll()
{
	return mouseScroll;
}

void _Input::resetKeyDowns()
{
	memset(keyDowns, 0, sizeof(bool) * 1024);
}

_Input Input;