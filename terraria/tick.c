
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <png.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>

void __attribute__((noreturn)) die(char *msg) {
	printf("\x1b[31;1m%s", msg);
	if (errno)
		printf(": %s", strerror(errno));
	printf("\x1b[0m\n");
	exit(errno || 1);
}
void warn(char *msg) {
	printf("\x1b[33;1m%s", msg);
	if (errno)
		printf(": %s", strerror(errno));
	printf("\x1b[0m\n");
}

#define randf() ((float)rand() / (float)RAND_MAX)
#define rands() (rand() > (RAND_MAX / 2) ? 1 : -1)
#define BLOCKSIZE 40

uint8_t *world = NULL;
int worldsize = -1;

int camerax = 0;
int cameray = 0;

// Function to load a PNG image using libpng
int loadPNG(const char* filename, int* width, int* height, png_bytep* image_data) {
	FILE* fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "Error: Could not open file %s for reading.\n", filename);
		return 0;
	}

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) {
		fclose(fp);
		return 0;
	}

	png_infop info = png_create_info_struct(png);
	if (!info) {
		png_destroy_read_struct(&png, NULL, NULL);
		fclose(fp);
		return 0;
	}

	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		return 0;
	}

	png_init_io(png, fp);
	png_read_info(png, info);

	*width = png_get_image_width(png, info);
	*height = png_get_image_height(png, info);

	png_read_update_info(png, info);

	int rowbytes = png_get_rowbytes(png, info);
	*image_data = (png_bytep)malloc(rowbytes * (*height));
	if (!*image_data) {
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		return 0;
	}

	png_bytep rows[*height];
	for (int i = 0; i < *height; ++i) {
		rows[i] = (*image_data) + i * rowbytes;
	}

	png_read_image(png, rows);
	png_destroy_read_struct(&png, &info, NULL);
	fclose(fp);

	return 1;
}

int getMouseButtonState(GLFWwindow* window, int button) {
	return glfwGetMouseButton(window, button);
}

float timeOffset;
GLuint ubo;

void init(GLFWwindow* window, GLuint shader, GLuint compute, int w, int h) {

	glUniform1f(glGetUniformLocation(shader, "worldwidth"), w);

	int worldsize = 1920 * 1080 / BLOCKSIZE / BLOCKSIZE;
	uint8_t *world = malloc(worldsize);
	for (int i = 0; i < worldsize; ++i) {
		world[i] = 0;
	}
	
	GLuint ubo;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo); // Binding pt = 0
	glBufferData(GL_UNIFORM_BUFFER, worldsize, world, GL_STATIC_DRAW); 
	GLuint blockIndex = glGetUniformBlockIndex(shader, "World");
	glUniformBlockBinding(shader, blockIndex, 0);  // Binding point 0

	// Atlas
	int width, height;
	png_bytep image_data;
	if (loadPNG("terraria/atlas.png", &width, &height, &image_data)) {
		GLuint textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Set texture parameters (you can adjust these as needed)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Load the image data into the texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

		glUniform1i(glGetUniformLocation(shader, "atlas"), textureID);
		glUniform1f(glGetUniformLocation(shader, "atlaswidth"), width);
		glUniform1f(glGetUniformLocation(shader, "atlasheight"), height);

		// Free the image data
		free(image_data);
	}
}

void tick(GLFWwindow* window, GLuint shader, GLuint compute, int w, int h) {

	// Pass mouse
	static double newX, newY;
	static double mouseX, mouseY;
	static int mouseBtn;
	glfwGetCursorPos(window, &newX, &newY);
	newY = h - newY;
	mouseX = (mouseX * 5 + newX) / 6;
	mouseY = (mouseY * 5 + newY) / 6;
	if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		mouseBtn = 1;
	else if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		mouseBtn = 2;
	else if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
		mouseBtn = 3;
	else
		mouseBtn = 0;
	glUniform3f(glGetUniformLocation(shader, "mouse"), mouseX, mouseY, mouseBtn);

	// Pass time
	static float time;
	time = glfwGetTime() + timeOffset;
	glUniform1f(glGetUniformLocation(shader, "time"), time);

}