glsl: src/main.c
	cc src/main.c -o glsl -lglfw -lGLEW -lGL -lGLU -ldl -lX11 -O4

glsl-debug: src/main.c
	cc src/main.c -o glsl-debug -lglfw -lGLEW -lGL -lGLU -ldl -lX11 -ggdb3 -fsanitize=address -DDEBUG

tick: src/tick.c
	cc src/tick.c -o tick.so -shared -lm -lglfw -lGLEW -lGL -fPIC -O4

tick-debug: src/tick.c
	cc src/tick.c -o tick-debug.so -shared -lm -lglfw -lGLEW -lGL -fPIC -ggdb3 -fsanitize=address -DDEBUG
