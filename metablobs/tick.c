
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

#define BLOBS 100
GLfloat pos[BLOBS * 4];
float vel[BLOBS * 4];

float z = 6.28;
int xypad = 10;

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

void init(GLFWwindow* window, GLuint shader, int w, int h) {

	timeOffset = fmod(rand(), 1e3);

	for (unsigned int i = 0; i < BLOBS * 4; i += 4) {
		vel[i]     = (randf() * 0.5 + 1) * rands();
		vel[i + 1] = (randf() * 0.5 + 1) * rands();
		vel[i + 2] = (randf() * 0.01 + 0.01) * rands();
		pos[i]     = randf() * w;
		pos[i + 1] = randf() * h;
		pos[i + 2] = randf() * (z - 0.5) + 0.5;
	}

	// Create and bind a uniform buffer object (UBO)
    GLuint ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);

    // Bind the UBO to the binding point
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);  // Binding point 0

    // Copy the data into the UBO
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

void tick(GLFWwindow* window, GLuint shader, int w, int h) {

	for (unsigned int i = 0; i < BLOBS * 4; i += 4) {
		// printf("%.2f %.2f %.2f\n", pos[i], pos[i + 1], pos[i + 2]);
		pos[i] += vel[i];
		pos[i + 1] += vel[i + 1];
		pos[i + 2] += vel[i + 2];
		pos[i + 3] = pow(sin(pos[i + 2]) + 0.5, 2) * 20;
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
			vel[i + 2] = fabs(vel[i + 2]);
			pos[i + 2] = 0.1;
		} else if (pos[i + 2] > z) {
			vel[i + 2] = fabs(vel[i + 2]) * -1;
			pos[i + 2] = z;
		}
		// Gravity
		for (int j = 0; j < i; j += 4) {
			static float dis;
			dis = sqrt(
				pow(pos[i] - pos[j], 2) +
				pow(pos[i + 1] - pos[j + 1], 2)
			) - 1e3;
			if (dis < 0.1) dis = 0.1;
			// 10-\sqrt{\frac{30}{x-5}}
			dis = 10 - sqrt(30 / dis);
			dis /= 1e5;
			pos[i] += (pos[j] - pos[i]) * dis;
			pos[i + 1] += (pos[j + 1] - pos[i + 1]) * dis;
		}
		// // Clamp
		// #define CLAMP 1
		// if (vel[i] < -CLAMP) vel[i] = -CLAMP;
		// else if (vel[i] > CLAMP) vel[i] = CLAMP;
		// if (vel[i + 1] < -CLAMP) vel[i + 1] = -CLAMP;
		// else if (vel[i + 1] > CLAMP) vel[i + 1] = CLAMP;
		// // Friction
		// vel[i] *= 0.9;
		// vel[i + 1] *= 0.9;
	}
	glBufferData(GL_UNIFORM_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW); 
	GLuint blockIndex = glGetUniformBlockIndex(shader, "Blobs");
    glUniformBlockBinding(shader, blockIndex, 0);  // Binding point 0

	// Pass time
    static float time;
    time = glfwGetTime() + timeOffset;
    glUniform1f(glGetUniformLocation(shader, "time"), time);

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
	hsl.h = fmod(time / 100, 0.5);
	hsl.s = fmod(5 + time / 200, 0.5) + 0.25;
	hsl.l = fmod(10 + time / 300, 0.5) + 0.25;
	rgb = HSLtoRGB(hsl);
	// printf("bg: %d %d %d\n", (int)(rgb.r * 255), (int)(rgb.g * 255), (int)(rgb.b *255));
	glUniform3f(glGetUniformLocation(shader, "bg"), rgb.r, rgb.g, rgb.b);
	// Fg
	hsl.h = fmod(5 + time / 150, 0.6);
	hsl.s = fmod(10 + time / 250, 0.6) + 0.2;
	hsl.l = fmod(15 + time / 350, 0.6) + 0.2;
	rgb = HSLtoRGB(hsl);
	// printf("fg: %d %d %d\n", (int)(rgb.r * 255), (int)(rgb.g * 255), (int)(rgb.b *255));
	glUniform3f(glGetUniformLocation(shader, "fg"), rgb.r, rgb.g, rgb.b);
	// Hg
	hsl.h = fmod(20 + time / 200, 0.9);
	hsl.s = fmod(25 + time / 300, 0.8) + 0.2;
	hsl.l = fmod(30 + time / 400, 0.8) + 0.2;
	rgb = HSLtoRGB(hsl);
	// printf("hg: %d %d %d\n", (int)(rgb.r * 255), (int)(rgb.g * 255), (int)(rgb.b *255));
	glUniform3f(glGetUniformLocation(shader, "hg"), rgb.r, rgb.g, rgb.b);

	// Pass size
    glUniform2f(glGetUniformLocation(shader, "size"), w, h);

    // Pass mouse
    static double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glUniform2f(glGetUniformLocation(shader, "mouse"), mouseX, h - mouseY);

	// Pass btn
	static int btn;
	if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		btn = 1;
	else if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		btn = 2;
	else if (getMouseButtonState(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
		btn = 3;
	else
		btn = 0;
	glUniform1i(glGetUniformLocation(shader, "btn"), btn);

}