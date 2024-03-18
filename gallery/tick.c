
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <png.h>
#include <glob.h>
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/errno.h>

#include "stb_image.h"
#include "stb_image_resize2.h"

#define PI 3.1415
#define PATH "~/.local/share/hydrus/client_files/f*/*"
#define COLUMNS 5
#define MAXTEXTURE 80
#define SLOW 50
#define FAST 500
#define SMOOTHFACTOR 5
#define PADDING 10

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

#define randi(max) (rand() % max)
#define randf(max) ((float)rand() * max / RAND_MAX) 

typedef struct {
	int used;
	int loaded;
	unsigned char *data;
	char *filename;
	unsigned int x;
	float y;
	int w;
	int h;
	GLuint texture_id;
} Picture;

glob_t glob_result;
char **filev;
char **filevstart;
char **filevend;
size_t filec;

static float columnbot[COLUMNS];
static float columntop[COLUMNS];
static float columnpos[COLUMNS];
static float columnspeed[COLUMNS];
static int columnpicked = -1;
static float columnpickedstart = -1;
static float columnpickeddelta[COLUMNS];

static float speed = SLOW, speeddesired = SLOW;

static double mouseX = 0, mouseY = 0;

GLfloat pictureData[MAXTEXTURE * 4];
GLuint ubo;

void filesLoad() {
	int status = glob(PATH, GLOB_TILDE, NULL, &glob_result);
	if (status != 0) {
		die("Unable to glob PATH or find any files");
	}
	filev = glob_result.gl_pathv;
	filec = glob_result.gl_pathc;
	filevstart = filev;
	filevend = filev + filec;
}
void filesShuffle() {
	if (filec <= 1) return;
	filev = filevstart;
	for (size_t i = 0; i < filec - 1; ++i) {
		size_t j = i + rand() / (RAND_MAX / (filec - i) + 1);
		char *file = filevstart[j];
		filevstart[j] = filevstart[i];
		filevstart[i] = file;
	}
}

Picture pictures[MAXTEXTURE];

void* pictureLoadWorker(Picture *picture) {
	// Load texture	
	int channels, tw, th;
	unsigned char *data = stbi_load(picture->filename, &tw, &th, &channels, 4);
	unsigned char *datanew = NULL;
	if (data == NULL) goto l_fail;
	if (tw < 1 || th < 1) goto l_fail;

	// printf("%s %d (%d ch): %d %d (%f) -> %d %d (%f)\n", picture->filename, picture->x, channels, tw, th, (float)tw / (float)th, picture->w, picture->h, (float)picture->w / (float)picture->h);
	datanew = malloc(picture->w * picture->h * 4 * sizeof(char));
	if (datanew == NULL) goto l_fail;
	stbir_resize(
		data, tw, th, 0,
		datanew, picture->w, picture->h, 0,
		STBIR_RGBA_PM, STBIR_TYPE_UINT8,
		STBIR_EDGE_CLAMP, STBIR_FILTER_CUBICBSPLINE
	);
	free(data);
	data = NULL;
	if (datanew == NULL) goto l_fail;
	if (picture->data) return NULL; // OOPS
	if (picture->loaded) return NULL; // OOPS
	picture->data = datanew;
	picture->used = 1; // Just in case
	picture->loaded = 0;
	return NULL;
	l_fail:
		// printf("FAILL\n");
		if (data != NULL) free(data);
		if (datanew != NULL) free(datanew);
		picture->data = NULL;
		picture->loaded = 0;
		picture->used = 1;
		// Can't leave an empty gap, find something
		for (unsigned int i = 0; i < MAXTEXTURE; ++i) {
			if (pictures[i].used == 1 && pictures[i].loaded == 1) {
				picture->texture_id = pictures[i].texture_id;
				picture->loaded = 1;
				return NULL;
			}
		}
		picture->loaded = 0;
		picture->used = 1;
		return NULL;
}

int pictureLoad(float x, float y, int w, int anchorTop) {
	// Find open slot
	unsigned int i = MAXTEXTURE + 1;
	for (unsigned int j = 0; j < MAXTEXTURE; ++j) {
		if (pictures[j].used == 0) {
			i = j;
			break;
		}
	}
	if (i == MAXTEXTURE + 1) return 0;
	pictures[i].used = 1;

	// Find filename
	char* file = *filev;
	++filev;
	if (filev == filevend) filesShuffle();

	// Get info
	int channels, tw, th;
	if (!stbi_info(file, &tw, &th, &channels)) goto l_fail;

	// Set data
	pictures[i].filename = file;
	pictures[i].loaded = 0;
	pictures[i].data = NULL;
	pictures[i].x = x;
	pictures[i].y = y;
	pictures[i].w = w;
	pictures[i].h = (int)round((float)pictures[i].w / (float)tw * (float)th);
	if (anchorTop) pictures[i].y -= pictures[i].h;
	// printf("%s %d %d -> %d %d\n", file, pictures[i].texture_w, pictures[i].texture_h, pictures[i].w, pictures[i].h);

	// Launch loading worker thread
	pthread_t thread;
	if (pthread_create(&thread, NULL, (void* (*)(void*))pictureLoadWorker, (void*)&(pictures[i])) != 0) {
		printf("Thread creation failed.\n");
		goto l_fail;
	}

	return pictures[i].h;

	l_fail:
		pictures[i].used = 0;
		return 0;

}

void scroll_callback(GLFWwindow* window, double x, double y) {
	printf("HII %f %f\n", x, y);
	static int w, h;
	glfwGetWindowSize(window, &w, &h);
	static int column;
	column = mouseX * COLUMNS / w;
	if (columnpicked == column) return;
	columnpickeddelta[column] -= y * FAST;
}

void init(GLFWwindow* window, GLuint shader, int w, int h) {

	glfwSetScrollCallback(window, scroll_callback);

	glShadeModel(GL_FLAT);

	// Load files
	filesLoad();
	filesShuffle();

	// Bind buffer
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	// Bind the UBO to the binding point
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
	// Copy the data into the UBO
	glBufferData(GL_UNIFORM_BUFFER, sizeof(pictureData), pictureData, GL_STATIC_DRAW); 

	// Gen Textures
	for (unsigned int i = 0; i < MAXTEXTURE; ++i) {
		glGenTextures(1, &pictures[i].texture_id);
	}

	// Set heights / speeds
	for (unsigned int i = 0; i < COLUMNS; ++i) {
		columntop[i] = -fmod((float)(unsigned char)filev[i][i], 50.0);
		columnbot[i] = columntop[i];
		columnpos[i] = 0;
		columnspeed[i] = SLOW;
		columnpickeddelta[i] = 0;
	}

}

void deinit(GLFWwindow* window, GLuint shader, int w, int h) {
	globfree(&glob_result);
}

static float timedelta, timelast;

void tick(GLFWwindow* window, GLuint shader, int w, int h) {

	static float colwidth, picwidth;
	colwidth = ((float)w) / COLUMNS;
	picwidth = ((float)w - PADDING * (COLUMNS + 1)) / COLUMNS;
	// printf("%f %f\n", colwidth, (float)w / COLUMNS);

	// Pass mouse
	glfwGetCursorPos(window, &mouseX, &mouseY);
	glUniform2f(glGetUniformLocation(shader, "mouse"), mouseX, mouseY);

	// Pass time
	static float time;
	time = glfwGetTime();
	timedelta = time - timelast;
	timelast = time;
	glUniform1f(glGetUniformLocation(shader, "time"), time);

	// Select Column
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (columnpicked == -1) {
			columnpicked = mouseX * COLUMNS / w;
			columnpickedstart = mouseY + columnpos[columnpicked];
		}
	} else {
		columnpicked = -1;
	}

	// Show Pictures
	glUniform1i(glGetUniformLocation(shader, "showpictures"), glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS ? 0 : 1);

	// Desired Speed
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		speeddesired = FAST;
	} else {
		speeddesired = SLOW;
	}
	speed = (speed * SMOOTHFACTOR + speeddesired) / (SMOOTHFACTOR + 1);

	// Drag Column
	if (columnpicked >= 0) {
		static float before;
		before = columnpos[columnpicked];
		columnpos[columnpicked] = (columnpos[columnpicked] + columnpickedstart - mouseY) / 2;
		columnpickeddelta[columnpicked] = (columnpos[columnpicked] - before) / timedelta;
	}

	// Column Speed
	for (unsigned int i = 0; i < COLUMNS; ++i) {
		if (i == columnpicked) continue;
		static float speednormal;
		static float x;
		x = time / 100 + (float)i;
		speednormal = sqrt(sin(fmod(x, PI))) * speed;
		if (fmod(x, 2 * PI) > PI) speednormal *= -1;
		if (fabs(columnpickeddelta[i] - speednormal) > 1) {
			columnpickeddelta[i] = (columnpickeddelta[i] * SMOOTHFACTOR + speednormal) / (SMOOTHFACTOR + 1);
			columnspeed[i] = columnpickeddelta[i];
		} else {
			columnspeed[i] = speednormal;
		}
		columnpos[i] += columnspeed[i] * timedelta;
	}

	// Remove unseeable textures / Load textures in
	for (unsigned int i = 0; i < MAXTEXTURE; ++i) {
		if (pictures[i].used == 0) continue;
		// printf("%d %f %f\n", i, pictures[i].y, pictures[i].y + pictures[i].h - pos);
		if (pictures[i].loaded == 0 && pictures[i].data != NULL) {
			// Upload texture
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, pictures[i].texture_id);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pictures[i].w, pictures[i].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pictures[i].data);
			pictures[i].loaded = 1;
			free(pictures[i].data);
		} else if (pictures[i].y + pictures[i].h - columnpos[pictures[i].x] < -FAST * 1.5)  { // Fell of bottom
			// printf("did dont\n");
			columnbot[pictures[i].x] = pictures[i].y + pictures[i].h + PADDING;
			pictures[i].used = 0;
		} else if (pictures[i].y - h - columnpos[pictures[i].x] > FAST * 1.5) { // Fell of top
			// printf("did dont top\n");
			columntop[pictures[i].x] = pictures[i].y;
			pictures[i].used = 0;
		}
	}
	// For each column populate
	for (unsigned int i = 0; i < COLUMNS; ++i) {
		// printf("%d at pos %f, %f to %f\n", i, columnpos[i], columnbot[i], columntop[i]);
		if (columntop[i] - h - columnpos[i] < FAST) { // Add to bottom
			// printf("did done at %d, %f\n", i, columntop[i]);
			columntop[i] += pictureLoad(i, columntop[i], picwidth, 0) + PADDING;
		} else if (columnbot[i] - columnpos[i] > -FAST) { // Add to top
			// printf("did done BOTTOM at %d, %f\n", i, columnbot[i]);
			columnbot[i] -= pictureLoad(i, columnbot[i] - PADDING, picwidth , 1) + PADDING;
		}
	}

	// Pass picture data
	for (unsigned int i = 0; i < MAXTEXTURE; ++i) {
		if (pictures[i].used == 0 || pictures[i].loaded == 0) {
			pictureData[i * 4] = -1;
			continue;
		}
		glActiveTexture(GL_TEXTURE0 + i);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, pictures[i].texture_id);
		glUniform1i(glGetUniformLocation(shader, "textures") + i, i);
		// printf("%d: %f %f %d %d\n", pictures[i].texture_id, pictures[i].x, pictures[i].y, pictures[i].w, pictures[i].h);
		pictureData[i * 4] = i;
		pictureData[i * 4 + 1] = (float)pictures[i].x * colwidth + PADDING;
		pictureData[i * 4 + 2] = pictures[i].y - columnpos[pictures[i].x];
		pictureData[i * 4 + 3] = (GLfloat)((pictures[i].w << 16) + pictures[i].h);
		// printf("%d: pos: %f %f - %f = %f: orig: %d %d, new: %f -> %d %d\n", i, pictures[i].x, pictures[i].y, columnpos[i], pictures[i].y - columnpos[i], pictures[i].w, pictures[i].h, pictureData[i * 4 + 3], ((int)pictureData[i * 4 + 3]) >> 16, ((int)pictureData[i * 4 + 3]) & 0xFFFF);
	}
	glBufferData(GL_UNIFORM_BUFFER, sizeof(pictureData), pictureData, GL_STATIC_DRAW); 
	glUniformBlockBinding(shader, glGetUniformBlockIndex(shader, "Pictures"), 0);

	// Pass size
	glUniform2f(glGetUniformLocation(shader, "size"), w, h);

}