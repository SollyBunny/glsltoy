
#include <GL/glew.h>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <dlfcn.h>
#include <sys/wait.h>

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

void errorCallback(int id, const char *desc) {
	printf("GL Error %d: %s\n", id, desc);
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

	char *sofile = NULL;

	for (unsigned int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-h") == 0) {
			die("Usage: glsl [tick.so] -h");
		} else if (sofile) {
			die("Too many arguments");
		} else {
			sofile = argv[i];
		}
	}

	if (sofile == NULL) {
		die("Please provide a directory to use");
	}

	// Check if sofile exists and is a directory
	if (access(sofile, F_OK) != 0) {
		die("Folder not found");
	} else if (access(sofile, R_OK) != 0) {
		die("Folder not readable");
	} else if (access(sofile, X_OK) != 0) {
		die("Folder not a folder");
	}

	// Fork to run make file
	pid_t child_pid = fork();
	if (child_pid == -1) {
		die("Failed to fork");
	}

	if (child_pid == 0) { // I'm the child
		#ifdef DEBUG
			execlp("make", "make", "tick-debug", "-C", sofile, NULL);
		#else
			execlp("make", "make", "-C", sofile, NULL);
		#endif
		die("Failed to run make file");
	}
	wait(NULL);

	// Chdir
	chdir(sofile);

	// Load tick function
	#ifdef DEBUG
		handle = dlopen("./tick-debug.so", RTLD_LAZY);
	#else
		handle = dlopen("./tick.so", RTLD_LAZY);
	#endif
	if (!handle) {
		#ifdef DEBUG
			printf("Couldn't open %s/tick-debug.so: %s\n", sofile, dlerror());
			handle = dlopen("./tick-debug.so", RTLD_LAZY);
		#else
			printf("Couldn't open %s/tick.so: %s\n", sofile, dlerror());
			handle = dlopen("./tick.so", RTLD_LAZY);
		#endif
		if (!handle) {
			chdir("..");
			#ifdef DEBUG
				printf("Couldn't open tick-debug.so: %s\n", dlerror());
				die("Coudn't open tick-debug.so");
			#else
				printf("Couldn't open tick.so: %s\n", dlerror());
				die("Coudn't open tick.so");
			#endif
		}
	}

	// Get a pointer to the tick function
	TickFunction tick = (TickFunction)dlsym(handle, "tick");
	if (!tick) {
	   	printf("Error: %s\n", dlerror());
		dlclose(handle);
		die("Coudn't get tick function");
	}

	// Get a pointer to the tick function
	TickFunction init = (TickFunction)dlsym(handle, "init");

	// glfw init
	if (!glfwInit()) {
		die("Failed to initialize GLFW");
	}

	// Set error handler
	glfwSetErrorCallback(errorCallback);

	// TODO: either make window span all monitor space or spawn a window on every monitor
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
	glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
	glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
	glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "GLSLToy", NULL, NULL);
	if (!window) {
		glfwTerminate();
		die("Failed to create GLFW window");
	}
	Display *xDisplay = glfwGetX11Display();
	Window xWindow = glfwGetX11Window(window);
	XSync(xDisplay, False);
	XSetWindowAttributes xAttrs;
	xAttrs.override_redirect = True;
	if (XChangeWindowAttributes(xDisplay, xWindow, CWOverrideRedirect, &xAttrs) < 0) {
		die("Failed to set window attributes");
	}
	XSync(xDisplay, False);
	XMapWindow(xDisplay, xWindow);
	XLowerWindow(xDisplay, xWindow);
	XSync(xDisplay, False);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		die("Failed to initialize GLEW");
	}

	// Vertex Shader
	char* vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec2 position;"
		"void main() {"
			"gl_Position = vec4(position, 0.0, 1.0);"
		"}"
	;

	// Read fragment shader code from file
	char* fragmentShaderSource = readShaderFile("shader.glsl");
	if (!fragmentShaderSource) {
		die("No shader.glsl found");
	}

	// Create and compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const GLchar * const*)&vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	// Check for compile errors in the vertex shader
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	glGetShaderInfoLog(vertexShader, sizeof(buff), NULL, buff);
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

		// printf("\x1b[2J\x1b[H");
		time = glfwGetTime();
		dt = time - lasttime;
		if (1.0 / dt > FPS) {
			sleepTime.tv_nsec = (1.0 / FPS - dt) * 1e9;
			nanosleep(&sleepTime, NULL);
			continue;
		}
		
		// printf("FPS: %f\n", 1.0 / dt);
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
