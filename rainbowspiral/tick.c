
#include <GL/glew.h>
#include <GLFW/glfw3.h>

static float dir = 1;
static float dirdesired = 1;
static int pressed = 0;

static float timedelta, timelast, pos;

void tick(GLFWwindow* window, GLuint shader, int w, int h) {

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (pressed != 1) {
			pressed = 1;
			dirdesired *= -1;
		}
	} else {
		pressed = 0;
	}

	dir = (dirdesired + dir) / 3;

	// Pass time
    static float time;
    time = glfwGetTime();
    timedelta = time - timelast;
    timelast = time;
    pos += timedelta * dir;
    
    glUniform1f(glGetUniformLocation(shader, "time"), pos);

    // Pass mouse
    static double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glUniform2f(glGetUniformLocation(shader, "mouse"), mouseX, h - mouseY);

}