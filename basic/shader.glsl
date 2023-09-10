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