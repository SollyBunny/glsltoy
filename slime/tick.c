
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define randf() ((float)rand() / (float)RAND_MAX)
#define rands() (rand() > (RAND_MAX / 2) ? 1 : -1)

#define PARTICLES 100

int getMouseButtonState(GLFWwindow* window, int button) {
	return glfwGetMouseButton(window, button);
}

void init(GLFWwindow* window, GLuint shader, GLuint compute, int w, int h) {

	GLfloat *data = malloc(sizeof(GLfloat) * 4 * PARTICLES);
	for (unsigned int i = 0; i < PARTICLES * 4; i += 4) {
		static float dir, mag;
		dir = randf() * 2 * M_PI;
		mag = randf() * 500;
		data[i]     = (float)w / 2 + sin(dir) * mag;
		data[i + 1] = (float)h / 2 + cos(dir) * mag;
		data[i + 2] = dir;
		data[i + 3] = 0;
		// printf("%f %f %f %f\n", data[i], data[i + 1], data[i + 2], data[i + 3]);
	}


	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLfloat) * 4 * PARTICLES, data, GL_DYNAMIC_DRAW);

	GLuint bufferIndex;
	bufferIndex = glGetProgramResourceIndex(shader, GL_SHADER_STORAGE_BLOCK, "Particles");
	glShaderStorageBlockBinding(shader, bufferIndex, 0);

	glUseProgram(compute);
	bufferIndex = glGetProgramResourceIndex(compute, GL_SHADER_STORAGE_BLOCK, "Particles");
	glShaderStorageBlockBinding(compute, bufferIndex, 0);

	free(data);

}

void tick(GLFWwindow* window, GLuint shader, GLuint compute, GLuint texture, int w, int h) {

	// Pass time
	// static float time;
	// time = glfwGetTime();
	// glUniform1f(glGetUniformLocation(shader, "time"), time);

	// Pass mouse
	// static double mouseX, mouseY;
	// glfwGetCursorPos(window, &mouseX, &mouseY);
	// glUniform2f(glGetUniformLocation(shader, "mouse"), mouseX, h - mouseY);

	glUniform1i(glGetUniformLocation(shader, "texture"), texture);

	// Compute
	glUseProgram(compute);
	glDispatchCompute(PARTICLES, 1, 1);

	glUniform2f(glGetUniformLocation(compute, "size"), w, h);

	glUniform1i(glGetUniformLocation(compute, "textureIn"), texture);

	GLuint imageUnit = 0; // Choose an available image unit
	glBindImageTexture(imageUnit, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

}