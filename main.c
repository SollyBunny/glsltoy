
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
#include <poll.h>
#include <fcntl.h>

#include <dlfcn.h>
#include <sys/wait.h>
#include <sys/inotify.h>

#define FPS 20

typedef void (*TickFunction)(GLFWwindow* window, GLuint shader, int w, int h);
static TickFunction tick = NULL, init = NULL;

static char* dir = NULL;

static void* handle = NULL;

static int embed = 1;

int inotify_fd = 0;
static int watch_fd[2] = { 0, 0 };

static int windowWidth = 1920, windowHeight = 1080;
static GLFWwindow* window;

GLuint shaderProgram = 0, vertexShader = 0, fragmentShader = 0;

static GLint success;
static char buf[1024];
static char cwd[256];

#define die(...) { printf("\033[1;31mError:\033[0m "); printf(__VA_ARGS__); putchar('\n'); exit(errno || 1); goto end; }
#define warn(...) { printf("\033[1;33mWarn:\033[0m "); printf(__VA_ARGS__); putchar('\n'); }

static void errorCallback(int id, const char *desc) {
	warn("GL Error %d: %s", id, desc);
}

// Function to read shader code from a file
static char* readShaderFile(const char* filePath) {
	FILE* file = fopen(filePath, "r");
	if (!file) {
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	size_t length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = (char*)malloc(length + 1);
	if (!buffer) {
		warn("Memory allocation error");
		fclose(file);
		return NULL;
	}

	fread(buffer, 1, length, file);
	buffer[length] = '\0';
	fclose(file);

	return buffer;
}

static char* load() {
	if (chdir(cwd) != 0)
		return "Failed to chdir to cwd";

	// Check if dir exists and is a directory
	if (access(dir, F_OK) != 0) {
		return "Folder not found";
	} else if (access(dir, R_OK) != 0) {
		return "Folder not readable";
	} else if (access(dir, X_OK) != 0) {
		return "Folder not a folder";
	}

	// Fork to run make file
	pid_t child_pid = fork();
	if (child_pid == -1) return "Failed to fork";

	if (child_pid == 0) { // I'm the child
		#ifdef DEBUG
			execlp("make", "make", "tick-debug", "-C", dir, NULL);
		#else
			execlp("make", "make", "-C", dir, NULL);
		#endif
		exit(1);
	}
	if (wait(NULL) + 1 == child_pid)
		return "Failed to run make file";

	// Chdir
	if (chdir(dir) != 0)
		return "Failed to chdir";

	// Load tick function
	#ifdef DEBUG
		handle = dlopen("./tick-debug.so", RTLD_LAZY);
	#else
		handle = dlopen("./tick.so", RTLD_LAZY);
	#endif
	if (!handle) {
		#ifdef DEBUG
			printf("Couldn't open %s/tick-debug.so: %s\n", dir, dlerror());
			handle = dlopen("./tick-debug.so", RTLD_LAZY);
		#else
			printf("Couldn't open %s/tick.so: %s\n", dir, dlerror());
			handle = dlopen("./tick.so", RTLD_LAZY);
		#endif
		if (!handle) {
			chdir("..");
			#ifdef DEBUG
				printf("Couldn't open tick-debug.so: %s\n", dlerror());
				return "Coudn't open tick-debug.so";
			#else
				printf("Couldn't open tick.so: %s\n", dlerror());
				return "Coudn't open tick.so";
			#endif
		}
	}

	// Get a pointer to the tick function
	tick = (TickFunction)dlsym(handle, "tick");
	if (!tick) {
	   	printf("Error: %s\n", dlerror());
		return "Coudn't get tick function";
	}

	// Get a pointer to the tick function
	init = (TickFunction)dlsym(handle, "init");

	// Vertex Shader
	const char* vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec2 position;"
		"void main() {"
			"gl_Position = vec4(position, 0.0, 1.0);"
		"}"
	;

	// Read fragment shader code from file
	char* fragmentShaderSource = readShaderFile("shader.glsl");
	if (!fragmentShaderSource)
		return "No shader.glsl found";

	// Create and compile vertex shader
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const GLchar * const*)&vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	// Check for compile errors in the vertex shader
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	glGetShaderInfoLog(vertexShader, sizeof(buf), NULL, buf);
	if (!success) {
		printf("%s\n", buf);
		free(fragmentShaderSource);
		return "Vertex shader compilation failed";
	}

	// Create and compile fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, (const GLchar * const*)&fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	// Check for compile errors in the fragment shader
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	glGetShaderInfoLog(fragmentShader, sizeof(buf), NULL, buf);
	if (!success) {
		printf("%s\n", buf);
		return "Vertex shader compilation failed";
	}

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// Free the allocated memory for shader source
	free(fragmentShaderSource);
	fragmentShaderSource = NULL;

	if (init)
		init(window, shaderProgram, windowWidth, windowHeight);

	glUseProgram(shaderProgram);

	return NULL;

}

static void unload() {
	glUseProgram(0);
	if (handle) {
		dlclose(handle);
		handle = NULL;
	}
	if (shaderProgram) {
		glDeleteProgram(shaderProgram);
		shaderProgram = 0;
	}
	if (vertexShader) {
		glDeleteShader(vertexShader);
		vertexShader = 0;
	}
	if (fragmentShader) {
		glDeleteShader(fragmentShader);
		fragmentShader = 0;
	}
	init = tick = NULL;
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	(void)window;
	glViewport(0, 0, width, height);
	windowWidth = width;
	windowHeight = height;
}

static void help() {
	printf("Usage: glsl [directory] -h --no-embed\n");
	exit(0);
}

int main(int argc, char *argv[]) {

	for (unsigned int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			help();
		} else if (strcmp(argv[i], "--no-embed") == 0) {
			embed = 0;
		} else if (dir) {
			die("Too many arguments");
		} else {
			dir = argv[i];
		}
	}

	if (dir == NULL)
		help();

	printf("Using directory \"%s\", %sembedding into background\n", dir, embed ? "" : "not ");

	// glfw init
	if (!glfwInit())
		die("Failed to initialize GLFW");

	// Set error handler
	glfwSetErrorCallback(errorCallback);

	if (embed) {
		// TODO: either make window span all monitor space or spawn a window on every monitor
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
		glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
		glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
		glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
		glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);
		glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);

		int init = 0;
		int x1, y1, x2, y2;
		int x1mon, y1mon, x2mon, y2mon;
	
		int monitorCount;
		GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
	
		for (int i = 0; i < monitorCount; ++i) {
			GLFWmonitor* monitor = monitors[i];
	
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwGetMonitorPos(monitor, &x1mon, &x2mon);
			x2mon = x1mon + mode->width;
			y2mon = y1mon + mode->height;
	
			if (!init) {
				x1 = x1mon; y1 = y1mon;
				x2 = x2mon; y2 = x2mon;
				init = 1;
			} else {
				if (x1 > x1mon) x1 = x1mon;
				if (y1 > y1mon) y1 = y1mon;
				if (x2 < x2mon) x2 = x2mon;
				if (y2 < y2mon) y2 = y2mon;
			}
		}
		if (init) {
			windowWidth = x2 - x1;
			windowHeight = y2 - y1;
		}
		
		window = glfwCreateWindow(windowWidth, windowHeight, "GLSLToy", NULL, NULL);
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
	} else {
		window = glfwCreateWindow(windowWidth, windowHeight, "GLSLToy", NULL, NULL);
		if (!window) {
			glfwTerminate();
			die("Failed to create GLFW window");
		}
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK)
		die("Failed to init GLEW");

	srand(time(NULL));
	getcwd(cwd, sizeof(cwd));

	// Add watches
	inotify_fd = inotify_init();
	if (inotify_fd < 0) { inotify_fd = 0; die("Failed to init inotify"); }
	int flags = fcntl(inotify_fd, F_GETFL, 0);
    if (flags == -1) die("Failed to get inoitfy fnctl flags");
    if (fcntl(inotify_fd, F_SETFL, flags | O_NONBLOCK) == -1) die("Failed to set inoitfy fnctl flags to non blocking");

	snprintf(buf, sizeof(buf), "%s/%s", dir, "Makefile");
	watch_fd[0] = inotify_add_watch(inotify_fd, buf, IN_MODIFY | IN_CREATE);
	if (watch_fd[0] < 0) {
		watch_fd[0] = 0;
		warn("Failed to add Makefile watch");
	}
	snprintf(buf, sizeof(buf), "%s/%s", dir, "shader.glsl");
	watch_fd[1] = inotify_add_watch(inotify_fd, buf, IN_MODIFY | IN_CREATE);
	if (watch_fd[1] < 0) {
		watch_fd[1] = 0;
		warn("Failed to add shader.glsl watch");
	}

	char* error = load();
	if (error) {
		unload();
		die(error);
	}

	double time, lasttime, dt;
	lasttime = glfwGetTime();
	// nanosleep setup
	struct timespec sleepTime;
	sleepTime.tv_sec = 0;
	sleepTime.tv_nsec = 0;

	size_t frame = 0;
	
	read(inotify_fd, buf, sizeof(buf));
	while (!glfwWindowShouldClose(window)) {

		time = glfwGetTime();
		dt = time - lasttime;
		if (1.0 / dt > FPS) {
			sleepTime.tv_nsec = (1.0 / FPS - dt) * 1e9;
			nanosleep(&sleepTime, NULL);
			continue;
		}

		frame++;

		if (frame % 30 == 0 && read(inotify_fd, buf, sizeof(buf)) > 0) {
			printf("Reloading\n");
			unload();
			char* error = load();
			if (error) {
				printf("Failed\n");
				unload();
			} else {
				printf("Success\n");
			}
			read(inotify_fd, buf, sizeof(buf)); // prevent build from creating more events
		}
		
		// printf("FPS: %f\n", 1.0 / dt);
		lasttime = time;
		
		glClear(GL_COLOR_BUFFER_BIT);

		// Tick
		if (tick) tick(window, shaderProgram, windowWidth, windowHeight);

		// Draw a fullscreen quad
		glBegin(GL_TRIANGLES);
		glVertex2f(-1.0f, -1.0f);
		glVertex2f( 3.0f, -1.0f);
		glVertex2f(-1.0f,  3.0f);
		glEnd();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	end:

	if (watch_fd[0]) {
		inotify_rm_watch(inotify_fd, watch_fd[0]);
		watch_fd[0] = 0;
	}
	if (watch_fd[1]) {
		inotify_rm_watch(inotify_fd, watch_fd[1]);
		watch_fd[1] = 0;
	}
	close(inotify_fd);

	unload();
	glfwTerminate();

	return 0;
}
