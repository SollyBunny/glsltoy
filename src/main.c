
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <time.h>

#define FPS 20

extern void tick(GLFWwindow* window, GLuint shader, int w, int h);

static int windowWidth = 1920, windowHeight = 1080;

GLint success;
char buff[1024];

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

// Function to read shader code from a file
char* readShaderFile(const char* filePath) {
	FILE* file = fopen(filePath, "r");
	if (!file) {
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = (char*)malloc(length + 1);
	if (!buffer) {
		fprintf(stderr, "Memory allocation error\n");
		fclose(file);
		return NULL;
	}

	fread(buffer, 1, length, file);
	buffer[length] = '\0';
	fclose(file);

	return buffer;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	(void)window;
	glViewport(0, 0, width, height);
	windowWidth = width;
	windowHeight = height;
}

typedef void (*TickFunction)(GLFWwindow*, GLuint, int, int);
void *handle;

int main(int argc, char *argv[]) {

	if (argc != 2) {
		die("Usage: glsl [tick.so]");
	}

	// Check if argv[1] exists and is a directory
	if (access(argv[1], F_OK) != 0) {
		die("Folder not found");
	} else if (access(argv[1], R_OK) != 0) {
		die("Folder not readable");
	} else if (access(argv[1], X_OK) != 0) {
		die("Folder not a folder");
	}

	// Fork to run make file
	pid_t child_pid = fork();
	if (child_pid == -1) {
		die("Failed to fork");
	}

	if (child_pid == 0) { // I'm the child
		execlp("make", "make", "-C", argv[1], NULL);
		die("Failed to run make file");
	}
	wait(NULL);

	// Load tick function
	sprintf(buff, "%s/tick.so", argv[1]);
	handle = dlopen(buff, RTLD_LAZY);
	if (!handle) {
		printf("%s\n", dlerror());
		die("Coudn't open tick.so");
	}

	// Get a pointer to the tick function
	TickFunction tick = (TickFunction)dlsym(handle, "tick");
	if (!tick) {
	   	printf("%s\n", dlerror());
		dlclose(handle);
		die("Coudn't get tick function");
	}

	// Get a pointer to the tick function
	TickFunction init = (TickFunction)dlsym(handle, "init");

	// glfw init
	if (!glfwInit()) {
		die("Failed to initialize GLFW");
	}

	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "GLSLToy", NULL, NULL);
	if (!window) {
		glfwTerminate();
		die("Failed to create GLFW window");
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (glewInit() != GLEW_OK) {
		die("Failed to initialize GLEW");
	}

	// Read vertex shader code from file
	sprintf(buff, "%s/vertex_shader.glsl", argv[1]);
	char* vertexShaderSource = readShaderFile(buff);
	if (!vertexShaderSource) {
		die("No vertex_shader.glsl found");
	}

	// Read fragment shader code from file
	sprintf(buff, "%s/fragment_shader.glsl", argv[1]);
	char* fragmentShaderSource = readShaderFile(buff);
	if (!fragmentShaderSource) {
		free(vertexShaderSource);
		die("No fragment_shader.glsl found");
	}

	// Create and compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const GLchar * const*)&vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	// Check for compile errors in the vertex shader
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	glGetShaderInfoLog(vertexShader, sizeof(buff), NULL, buff);
	printf("%s\n", buff);
	if (!success) {
		die("Vertex shader compilation failed");
	}

	// Create and compile fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, (const GLchar * const*)&fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	// Check for compile errors in the fragment shader
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	glGetShaderInfoLog(fragmentShader, sizeof(buff), NULL, buff);
	printf("%s\n", buff);
	if (!success) {
		die("Vertex shader compilation failed");
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// Free the allocated memory for shader source
	free(vertexShaderSource);
	free(fragmentShaderSource);

	srand(time(NULL));

	if (init) {
		init(window, shaderProgram, windowWidth, windowHeight);
	}

	glUseProgram(shaderProgram);

	double time, lasttime, dt;
	lasttime = glfwGetTime();
	// nanosleep setup
	struct timespec sleepTime;
	sleepTime.tv_sec = 0;
	sleepTime.tv_nsec = 0;
	
	while (!glfwWindowShouldClose(window)) {
		printf("\x1b[2J\x1b[H");
		time = glfwGetTime();
		dt = time - lasttime;
		if (1.0 / dt > FPS) {
			sleepTime.tv_nsec = (1.0 / FPS - dt) * 1e9;
			nanosleep(&sleepTime, NULL);
			continue;
		}
		
		printf("FPS: %f\n", 1.0 / dt);
		lasttime = time;
		
		glClear(GL_COLOR_BUFFER_BIT);

		// Tick
		tick(window, shaderProgram, windowWidth, windowHeight);

		// Draw a fullscreen quad
		glBegin(GL_TRIANGLES);
		glVertex2f(-1.0f, -1.0f);
		glVertex2f( 3.0f, -1.0f);
		glVertex2f(-1.0f,  3.0f);
		glEnd();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	dlclose(handle);
	
	return 0;
}
