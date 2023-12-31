
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <png.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/errno.h>

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

#define BLOBS 150
GLfloat pos[BLOBS * 4];
float vel[BLOBS * 4];

float z = 6.28;
int xypad = 10;

typedef int ChunkObj;
#define chunkx 32
#define chunky 32
ChunkObj blobs[BLOBS];
int chunks[chunkx * chunky];

GLuint ubo;

typedef struct {
	double h; // Hue in range [0, 1]
	double s; // Saturation in range [0, 1]
	double l; // Lightness in range [0, 1]
} HSL;

typedef struct {
	double r; // Red in range [0, 1]
	double g; // Green in range [0, 1]
	double b; // Blue in range [0, 1]
} RGB;

double hue2rgb(double p, double q, double t) {
	if (t < 0.0) t += 1.0;
	if (t > 1.0) t -= 1.0;
	if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
	if (t < 1.0 / 2.0) return q;
	if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
	return p;
}

RGB HSLtoRGB(HSL hsl) {
	RGB rgb;
	if (hsl.s == 0) {
		// Achromatic (gray)
		rgb.r = rgb.g = rgb.b = hsl.l;
	} else {
		double q = (hsl.l < 0.5) ? (hsl.l * (1.0 + hsl.s)) : (hsl.l + hsl.s - hsl.l * hsl.s);
		double p = 2.0 * hsl.l - q;
		rgb.r = hue2rgb(p, q, hsl.h + 1.0 / 3.0);
		rgb.g = hue2rgb(p, q, hsl.h);
		rgb.b = hue2rgb(p, q, hsl.h - 1.0 / 3.0);
	}

	return rgb;
}

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

void init(GLFWwindow* window, GLuint shader, GLuint compute, int w, int h) {

	timeOffset = fmod(rand(), 1e5);

	// Init chunk data
	for (unsigned int i = 0; i < chunkx * chunky; ++i)
		chunks[i] = -1;
	for (unsigned int i = 0; i < BLOBS; ++i)
		blobs[i] = -1;

	for (unsigned int i = 0; i < BLOBS * 4; i += 4) {
		// Init vel
		vel[i]     = (randf() * 0.5 + 1) * rands();
		vel[i + 1] = (randf() * 0.5 + 1) * rands();
		vel[i + 2] = (randf() * 0.01 + 0.01) * rands();
		// Init pos
		pos[i]     = randf() * w;
		pos[i + 1] = randf() * h;
		pos[i + 2] = randf() * (z - 0.5) + 0.5;
		// Add to chunk
		static int *chunk;
		chunk = &chunks[
			(int)(pos[i + 1] * chunky / h) * chunkx +
			(int)(pos[i] * chunky / w)
		];
		if (*chunk == -1) {
			*chunk = i / 4;
		} else {
			blobs[i / 4] = *chunk;
			*chunk = i / 4;
		}
	}

	/*for (unsigned int i = 0; i < chunkx * chunky; ++i) {
		static int *chunk;
		chunk = &chunks[i];
		if (*chunk == -1) continue;
		static ChunkObj blob;
		blob = *chunk;
		printf("In chunk %d: ", i);
		while (blob != -1) {
			printf("%d ", blob);
			fflush(stdout);
			blob = blobs[blob];
		}
		printf("\n");
	}*/

	// Pass pos data
	GLuint ubo;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo); // Binding pt = 0
	glBufferData(GL_UNIFORM_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW); 

	// Noise texture
	int width, height;
	png_bytep image_data;
	if (loadPNG("metablobs/noise.png", &width, &height, &image_data)) {
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

		glUniform1i(glGetUniformLocation(shader, "noiseTexture"), textureID);

		// Free the image data
		free(image_data);
	}
}

void tick(GLFWwindow* window, GLuint shader, GLuint compute, int w, int h) {

	static float time;
	time = glfwGetTime() + timeOffset;

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

	// Check if each element is in its chunk
	for (unsigned int i = 0; i < chunkx * chunky; ++i) {
		static int *chunk;
		chunk = &chunks[i];
		if (*chunk == -1) continue;
		static ChunkObj blob, blobold;
		blobold = -1;
		blob = *chunk;
		while (blob != -1) {
			static int *newchunk;
			newchunk = &chunks[(
				(int)(pos[blob * 4 + 1] * chunky / h) * chunkx +
				(int)(pos[blob * 4] * chunky / w)
			) % (chunkx * chunky)];
			// Check if mismatch
			if (chunk != newchunk) {
				// Remove from old chunk
				if (blobold == -1) { // First element
					*chunk = blobs[blob];
				} else {
					blobs[blobold] = blobs[blob];
				}
				// Add to new chunk
				blobs[blobold] = *newchunk;
				*newchunk = blob;
				blobs[blob] = -1;
			}
			// printf("%d ", blob);
			// fflush(stdout);
			blobold = blob;
			blob = blobs[blob];
		}
	}

	for (unsigned int i = 0; i < BLOBS * 4; i += 4) {
		// printf("%.2f %.2f %.2f\n", pos[i], pos[i + 1], pos[i + 2]);
		pos[i] += vel[i];
		pos[i + 1] += vel[i + 1];
		pos[i + 2] += vel[i + 2];
		pos[i + 3] = pow(sin(pos[i + 2]) + 5.5, 4);
		if (pos[i] < -xypad) { // x
			vel[i] = fabs(vel[i]);
			pos[i] = -xypad;
		} else if (pos[i] > w + xypad) {
			vel[i] = -fabs(vel[i]);
			pos[i] = w + xypad;
		} else if (pos[i + 1] < -xypad) { // y
			vel[i + 1] = fabs(vel[i + 1]);;
			pos[i + 1] = -xypad;
		} else if (pos[i + 1] > h + xypad) {
			vel[i + 1] = -fabs(vel[i + 1]);
			pos[i + 1] = h + xypad;
		}
		if (pos[i + 2] < 0.1) { // z
			vel[i + 2] = fabs( vel[i + 2]);
			pos[i + 2] = 0.1;
		} else if (pos[i + 2] > z) {
			vel[i + 2] = fabs(vel[i + 2]) * -1;
			pos[i + 2] = z;
		}
		// Gravity
		static ChunkObj blob;
		blob = chunks[(
			(int)(pos[blob * 4 + 1] * chunky / h) * chunkx +
			(int)(pos[blob * 4] * chunky / w)
		) % (chunkx * chunky)];
		while (blob != -1) {
			static int j;
			j = blob * 4;
			static float dis;
			dis = sqrt(
				pow(pos[i] - pos[j], 2) +
				pow(pos[i + 1] - pos[j + 1], 2)
			) - 1e4;
			if (dis < 0.1) dis = 0.1;
			// 10-\sqrt{\frac{30}{x-5}}
			dis = 10 - sqrt(30 / dis);
			dis /= 1e4;
			pos[i] += (pos[j] - pos[i]) * dis;
			pos[i + 1] += (pos[j + 1] - pos[i + 1]) * dis;
			blob = blobs[blob];
		}
		// Mouse Gravity
		static float dis;
		dis = (
			pow(pos[i] - mouseX, 2) +
			pow(pos[i + 1] - mouseY, 2)
		) - 1e3;
		if (dis < 0.1) dis = 0.1;
		dis = 10 - sqrt(30 / dis);
		dis /= 5e4;
		pos[i] += (mouseX - pos[i]) * dis;
		pos[i + 1] += (mouseY - pos[i + 1]) * dis;
	}
	glBufferData(GL_UNIFORM_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW); 

	// Pass texture offset
	glUniform2f(glGetUniformLocation(shader, "noiseOffset"),
		sin(time / 1000.0) * 50.0,
		cos(time / 1000.0) * 50.0
	);

	// Pass colors
	static HSL hsl;
	static RGB rgb;
	// printf("\x1b[2J\x1b[H");
	// Bg
	hsl.h = fmod(time / 100, 1);
	hsl.s = sin(5 + time / 200);
	hsl.l = sin(10 + time / 100);
	rgb = HSLtoRGB(hsl);
	// printf("bg: %d %d %d\n", (int)(rgb.r * 255), (int)(rgb.g * 255), (int)(rgb.b *255));
	glUniform3f(glGetUniformLocation(shader, "bg"), rgb.r, rgb.g, rgb.b);
	// Fg
	hsl.h = fmod(5 + time / 150, 1);
	hsl.s = sin(10 + time / 250);
	hsl.l = sin(15 + time / 150);
	rgb = HSLtoRGB(hsl);
	// printf("fg: %d %d %d\n", (int)(rgb.r * 255), (int)(rgb.g * 255), (int)(rgb.b *255));
	glUniform3f(glGetUniformLocation(shader, "fg"), rgb.r, rgb.g, rgb.b);
	// Hg
	hsl.h = fmod(20 + time / 200, 1);
	hsl.s = sin(25 + time / 300);
	hsl.l = sin(30 + time / 200);
	rgb = HSLtoRGB(hsl);
	// printf("hg: %d %d %d\n", (int)(rgb.r * 255), (int)(rgb.g * 255), (int)(rgb.b *255));
	glUniform3f(glGetUniformLocation(shader, "hg"), rgb.r, rgb.g, rgb.b);


}