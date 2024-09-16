glsl: main.c
	cc main.c -o glsl -lglfw -lGLEW -lGL -lGLU -ldl -lX11 -O4

glsl-debug: main.c
	cc main.c -o glsl-debug -lglfw -lGLEW -lGL -lGLU -ldl -lX11 -ggdb3 -fsanitize=address -DDEBUG
