tick: tick.c libstb_image.so libstb_image_resize2.so
	cc tick.c -o tick.so -shared -lm -lglfw -lGLEW -lGL -lpthread -Wl,-rpath,'$$ORIGIN' -L. -lstb_image -lstb_image_resize2 -O4 -fPIC

tick-debug: tick.c libstb_image.so libstb_image_resize2.so
	cc tick.c -o tick-debug.so -shared -lm -lglfw -lGLEW -lGL -lpthread -Wl,-rpath,'$$ORIGIN' -L. -lstb_image -lstb_image_resize2 -fPIC -ggdb3 -fsanitize=address -DDEBUG

libstb_image.so: stb_image.h stb_image.c
	cc stb_image.c -o libstb_image.so -lm -shared -O4 -fPIC

libstb_image_resize2.so: stb_image_resize2.h stb_image_resize2.c
	cc stb_image_resize2.c -o libstb_image_resize2.so -lm -shared -O4 -fPIC
