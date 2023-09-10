# GLSL Toy

GLSL Toy is a [ShaderToy](https://www.shadertoy.com/) clone meant with more freedoms.
1. Runs natively ⚡
2. Works as a live background 🖼️ *(Linux only)*
3. Can execute any other code along with GLSL shaders 🤯

## Usage

1. Install dependencies  
	* `GL`, `GLFW`, `GLEW`, `X11`
	* `make`, `cc` (for compiling)
	* `libpng` (for metablobs)
2. Compile (with make)  
	```sh
	$ make
	```
3. Run
	```sh
	./glsl metablobs
	```

## Making Shaders

Each shader project is stored independently in a folder.

### `shader.glsl`
This is the only required file: The fragment shader is used to determine each pixel's color.

Here is an example:
```glsl
// shader.glsl
#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;
uniform vec3 mouse;

void main() {

    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = (vec2(mouse) + vec2(gl_FragCoord)) / resolution;

    // Varying pixel color
    vec3 color;
	color = vec3(uv, 1) * vec3(1, 1, 1) * 0.5 * sin(time) + 0.5;

    // Output to screen
    FragColor = vec4(color, 1.0);

}
```

Usefull data such as the cursor position can be imported using `uniform`, here are the default available, a custom `tick.so` can add or remove these:

* `vec2 resolution` - Screen size
* `float time` - Time (s) since start
* `vec3 mouse` - Mouse x, y and btn

### `Makefile`

The `Makefile` is responsible for compiling any CPU code to an SO file called `tick.so`. Although the projects here use `c`, theoretically any language can be used as long as they export the right tokens.

Here is an example:
```Makefile
# Makefile
tick.so: tick.c
	cc tick.c -o tick.so -shared -lm -lglfw -lGLEW -lGL -O4 -fPIC
```
```c
// tick.c
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>

float x, y;
float xv, yv;

void init(GLFWwindow* window, GLuint shader, int w, int h) {
	// Optional
	// Called on first frame
	x = rand() / (float)RAND_MAX * w;
	y = rand() / (float)RAND_MAX * h;
	xv = 2;
	yv = 2;
}

void tick(GLFWwindow* window, GLuint shader, int w, int h) {
	// Optional
	// Called every frame
	x += xv;
	y += yv;
	if (x < 0) {
		x = 0;
		xv = 2;
	} else if (x > w) {
		x = w;
		xv = -2
	}
	if (y < 0) {
		y = 0;
		yv = 2;
	} else if (y > h) {
		y = h;
		yv = -2;
	}
	// Push data to GPU
	glUniform2f(glGetUniformLocation(shader, "pos"), x, y);
}
```