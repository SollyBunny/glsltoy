glsl: src/main.c
	cc src/main.c -o glsl -lglfw -lGLEW -lGL -ldl -lX11 -O4

tick: src/tick.c
	cc src/tick.c -o tick.so -shared -lm -lglfw -lGLEW -lGL -O4 -fPIC