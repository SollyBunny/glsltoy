tick: tick.c
	cc tick.c -o tick.so -shared -lm -lglfw -lGLEW -lGL -lpng -O4 -fPIC

tick-debug: tick.c
	cc tick.c -o tick-debug.so -shared -lm -lglfw -lGLEW -lGL -lpng -fPIC -ggdb3 -fsanitize=address -DDEBUG
