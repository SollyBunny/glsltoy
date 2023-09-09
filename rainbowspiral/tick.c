
#include <GL/glew.h>
#include <GLFW/glfw3.h>

int getMouseButtonState(GLFWwindow* window, int button) {
    return glfwGetMouseButton(window, button);
}

void tick(GLFWwindow* window, GLuint shader, int w, int h) {

	// Pass size
    glUniform2f(glGetUniformLocation(shader, "size"), w, h);
	

	// Pass time
    static float time;
    time = glfwGetTime();
    glUniform1f(glGetUniformLocation(shader, "time"), time);

    // Pass mouse
    static double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glUniform2f(glGetUniformLocation(shader, "mouse"), mouseX, h - mouseY);

	// Pass btn
	static int btn;
	if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		btn = 1;
	else if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		btn = 2;
	else if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
		btn = 3;
	else
		btn = 0;
	glUniform1i(glGetUniformLocation(shader, "btn"), btn);

}