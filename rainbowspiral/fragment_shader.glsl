#version 330 core

out vec4 FragColor;

uniform vec2 size;
uniform float time;
uniform vec2 mouse;
uniform int btn;

vec3 rainbow(float t) {
    float r, g, b;
    t = mod(t, 1);

    if (t >= 0.0 && t < 1.0 / 6.0) {
        t = t / (1.0 / 6.0);
        r = 1;
        g = t;
        b = 0;
    } else if (t >= 1.0 / 6.0 && t < 2.0 / 6.0) {
        t = (t - 1.0 / 6.0) / (1.0 / 6.0);
        r = 1 - t;
        g = 1;
        b = 0;
    } else if (t >= 2.0 / 6.0 && t < 3.0 / 6.0) {
        t = (t - 2.0 / 6.0) / (1.0 / 6.0);
        r = 0;
        g = 1;
        b = t;
    } else if (t >= 3.0 / 6.0 && t < 4.0 / 6.0) {
        t = (t - 3.0 / 6.0) / (1.0 / 6.0);
        r = 0;
        g = 1 - t;
        b = 1;
    } else if (t >= 4.0 / 6.0 && t < 5.0 / 6.0) {
        t = (t - 4.0 / 6.0) / (1.0 / 6.0);
        r = t;
        g = 0;
        b = 1;
    } else {
        t = (t - 5.0 / 6.0) / (1.0 / 6.0);
        r = 1;
        g = 0;
        b = 1 - t;
    }
    return vec3(r, g, b);
}

void main() {

    float index =
    	gl_FragCoord.x
    	+ time
    	+ sin((mouse.x - gl_FragCoord.x) / (mouse.y - gl_FragCoord.y))
    ;
    FragColor = vec4(rainbow(index), 1.0);

}
