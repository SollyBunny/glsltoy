#version 430 core

#define PARTICLES 100

out vec4 FragColor;

layout(std430, binding = 0) buffer Particles {
	vec4 pos[PARTICLES];
};


// uniform float time;
// uniform vec2 mouse;
// uniform int btn;

uniform sampler2D image;

void main() {
	FragColor = texture(image, gl_FragCoord.xy);
	// for (int i = 0; i < PARTICLES; ++i) {
	//     if ((
	//         pow(pos[i].x - gl_FragCoord.x, 2) +
	//         pow(pos[i].y - gl_FragCoord.y, 2)
	//     ) < 1000) {
	//         FragColor = vec4(0.0, 1.0, 0.0, 1.0);
	//         return;
	//     }
	// }
	// FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	// FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
