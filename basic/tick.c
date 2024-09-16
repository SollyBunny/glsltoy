
#include <GL/glew.h>
#include <GLFW/glfw3.h>

int getMouseButtonState(GLFWwindow* window, int button) {
    return glfwGetMouseButton(window, button);
}

void init(GLFWwindow* window, GLuint shader, int w, int h) {

}

void tick(GLFWwindow* window, GLuint shader, int w, int h) {

	// Pass resolution
    glUniform2f(glGetUniformLocation(shader, "resolution"), w, h);

	// Pass time
    static float time;
    time = glfwGetTime();
    glUniform1f(glGetUniformLocation(shader, "time"), time);

    // Pass mouse
    static double mouseX, mouseY, mouseBtn;
    glfwGetCursorPos(window, &mouseX, &mouseY);
	if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		mouseBtn = 1;
	else if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		mouseBtn = 2;
	else if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
		mouseBtn = 3;
	else
		mouseBtn = 0;
    glUniform3f(glGetUniformLocation(shader, "mouse"), mouseX, h - mouseY, mouseBtn);

}