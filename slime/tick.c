
#include <GL/glew.h>
#include <GLFW/glfw3.h>

int getMouseButtonState(GLFWwindow* window, int button) {
    return glfwGetMouseButton(window, button);
}

void tick(GLFWwindow* window, GLuint shader, int w, int h) {

	// Pass time
    static float time;
    time = glfwGetTime();
    glUniform1f(glGetUniformLocation(shader, "time"), time);

    // Pass mouse
    static double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glUniform2f(glGetUniformLocation(shader, "mouse"), mouseX, h - mouseY);

}