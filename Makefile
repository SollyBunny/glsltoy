glsl: src/main.c
	cc src/main.c -o glsl -lglfw -lGLEW -lGL -ldl -lX11 -O4
