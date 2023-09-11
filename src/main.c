
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

#define FPS 30

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

// Function to read code from a file
char* readFile(const char* filePath) {
	FILE* file = fopen(filePath, "r");
	if (!file) return NULL;
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

// Compile a shader
GLuint compileShader(GLenum shaderType, const char* shaderSource) {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, (const GLchar * const*)&shaderSource, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	glGetShaderInfoLog(shader, sizeof(buff), NULL, buff);
	if (!success) {
		printf("%s\n", buff);
		die("Shader compilation failed");	
	}
	return shader;
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
		execlp("make", "make", "-C", sofile, NULL);
		die("Failed to run make file");
	}
	wait(NULL);

	// Load tick function
	sprintf(buff, "%s/tick.so", sofile);
	handle = dlopen(buff, RTLD_LAZY);
	if (!handle) {
		strcpy(buff, dlerror()); // Save error just in case
		handle = dlopen("./tick.so", RTLD_LAZY);
		if (!handle) {
			printf("%s\n%s\n", buff, dlerror());
			die("Coudn't open tick.so");
		}
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
	Display *xDisplay = glfwGetX11Display();
	Window xWindow = glfwGetX11Window(window);
	XUnmapWindow(xDisplay, xWindow);
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

	if (glewInit() != GLEW_OK)
		die("Failed to initialize GLEW");

	// Vertex Shader
	char* vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec2 position;"
		"void main() {"
			"gl_Position = vec4(position, 0.0, 1.0);"
		"}"
	;
	GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);

	// Fragment Shader
	sprintf(buff, "%s/shader.glsl", sofile);
	char* fragmentShaderSource = readFile(buff);
	if (!fragmentShaderSource) die("No shader.glsl found");
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
	free(fragmentShaderSource);

	// Compute Shader
	sprintf(buff, "%s/compute.glsl", sofile);
	char* computeShaderSource = readFile(buff);
	GLuint computeShader = -1;
	if (computeShaderSource) {
		computeShader = compileShader(GL_COMPUTE_SHADER, computeShaderSource);
		free(computeShaderSource);
	}

	// Check for compile errors in the compute shader
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	glGetShaderInfoLog(vertexShader, sizeof(buff), NULL, buff);
	printf("%s\n", buff);
	if (!success) {
		die("Vertex shader compilation failed");
	}

	/*const char* computeShaderSource = "#version 430 core\n"
    "layout(local_size_x = 1, local_size_y = 1) in;\n"
    "layout(binding = 0) buffer DataBuffer {\n"
    "    vec2 data[];\n"
    "};\n"
    "void main() {\n"
    "    uint id = gl_GlobalInvocationID.x;\n"
    "    data[id].x += 1.0;\n"
    "}n";*/

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	if (computeShader != -1)
		glAttachShader(shaderProgram, computeShader);
	glLinkProgram(shaderProgram);


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
