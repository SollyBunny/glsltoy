#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;
uniform vec3 mouse;

vec3 a = vec3(1.0, 0.0, 0.0);
vec3 b = vec3(0.0, 1.0, 0.0);
vec3 c = vec3(0.0, 0.0, 1.0);
vec3 d = vec3(0.0, 1.0, 1.0);

float cellsize = 40.0;

void main() {

    float xc = gl_FragCoord.x;
    float yc = gl_FragCoord.y;
    float xi = 0.0;
    float yi = 0.0;
    while (xc > 0.0) {
        float s = cellsize + sin(xi * tan(xi) + xi) * 10.0;
        xi += 1;
        xc -= s;
    }
    while (yc > 0.0) {
        float s = cellsize + cos(yi * tan(yi) + yi) * 10.0;
        yi += 1;
        yc -= s;
    }

    if (-xc -yc > cellsize) {
        xi -= 3.5;
    }

    float rand2 = mod(pow(yi * xi + sin(xi * yi + xi), 2), 1.1) / 1.1;

    FragColor.rgb = mix(mix(a, b, xi / (resolution.x / cellsize)), mix(c, d, yi / (resolution.y / cellsize)), 0.5);
    FragColor.rgb += rand2 / 10.0;

    // // Normalized pixel coordinates (from 0 to 1)
    // vec2 uv = (vec2(mouse) + vec2(gl_FragCoord)) / resolution;

    // // Varying pixel color
    // vec3 color;
	// color = vec3(uv, 1) * vec3(1, 1, 1) * 0.5 * sin(time) + 0.5;

    // // Output to screen
    // FragColor = vec4(color, 1.0);
	
}