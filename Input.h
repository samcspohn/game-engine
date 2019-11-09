#pragma once
#include <GLFW/glfw3.h>

using namespace std;

class {
	bool keys[1024];
	bool keyDowns[1024];
public:
	bool getKey(unsigned int key) {
		return keys[key];
	}
	bool getKeyDown(unsigned int key) {
		return keyDowns[key];
	}
	class {
		bool firstMouse = true;
		GLfloat lastX = 860 / 2.0;
		GLfloat lastY = 540 / 2.0;
		GLfloat xOffset = 0, yOffset = 0;
		float mouseScroll;
		bool mouseButtons[32];
	public:
		float getX() { return xOffset; }
		float getY() { return yOffset; }
		bool getButton(unsigned int button) {
			return mouseButtons[button];
		}
		float  getScroll() {
			return mouseScroll;
		}
		friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

		friend void MouseCallback(GLFWwindow *window, double xPos, double yPos);

		friend void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset);
		friend void mouseFrameBegin();
	}Mouse;

	friend void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
	void resetKeyDowns() {
		memset(keyDowns, 0, sizeof(bool) * 1024);
	}
}Input;