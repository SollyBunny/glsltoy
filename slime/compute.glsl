#version 430 core

#define PARTICLES 100
#define SPEED 10
#define PAD 100

#define PI 3.1415

layout(local_size_x = PARTICLES, local_size_y = 1, local_size_z = 1) in;
layout (std430, binding = 0) buffer Particles {
	vec4 pos[PARTICLES];
};

uniform vec2 size;
uniform sampler2D textureIn;
layout(rgba8) uniform image2D textureOut;

float bounce(float dir, int side) {
	dir = mod(dir, 2 * PI);
	if (side == 0)
		return -dir;
	return PI - dir;
}

void main() {
	uint id = gl_GlobalInvocationID.x;
	// Move particle
	pos[id].x += sin(pos[id].z) * SPEED;
	pos[id].y += cos(pos[id].z) * SPEED;
	// Check bounds
	if (pos[id].x < PAD) {
		pos[id].x = PAD;
		pos[id].z = bounce(pos[id].z, 0);
	} else if (pos[id].x > size.x - PAD) {
		pos[id].x = size.x - PAD;
		pos[id].z = bounce(pos[id].z, 0);
	} else if (pos[id].y < PAD) {
		pos[id].y = PAD;
		pos[id].z = bounce(pos[id].z, 1);
	} else if (pos[id].y > size.y - PAD) {
		pos[id].y = size.y - PAD;
		pos[id].z = bounce(pos[id].z, 1);
	}
	// Dot
	ivec2 px = ivec2(
		int(pos[id].x / size.x),
		int(pos[id].y / size.y)
	);
	vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
	imageStore(textureOut, px, color);
}