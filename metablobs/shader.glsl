#version 330 core

#define BLOBS 100

out vec4 FragColor;

uniform vec2 size;
uniform float time;
uniform vec3 mouse;

layout (std140) uniform Blobs {
    vec4 pos[BLOBS];  // You can adjust the size as needed
};

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

uniform vec3 bg, fg, hg;
vec3 lava(float t) {
    vec3 color;
    t = clamp(t, 0.0, 1.0);
    color = mix(bg, fg, tan(t * 2));
    if (t > 0.5) {
        color = mix(hg, color, (t - 0.5) * 2);
    }
    return color;
}

uniform sampler2D noiseTexture;
uniform vec2 noiseOffset;
void main() {

    float index = 0;
    float dis;
    vec4 noise;
    noise = texture(noiseTexture, vec2(gl_FragCoord) / 256.0 + noiseOffset);
    vec2 coord = vec2(
        gl_FragCoord.x + noise.r * 30.0,
        gl_FragCoord.y + noise.g * 30.0
    );
    vec2 dist = vec2(gl_FragCoord) - vec2(mouse);
    float radius = length(dist);
    if (radius < 100) {
        coord += dist * sin((1 - radius / 100) * 2 * 3.14);
    }
    
    for (int i = 0; i < BLOBS; ++i) {
        dis = (
            pow(pos[i].x - coord.x, 2) +
            pow(pos[i].y - coord.y, 2)
        );
        // if (dis < 10.0) {
        //     FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        //     return;
        // }
        index += 1.0 / dis * pos[i].w;
    }

    FragColor = vec4(lava(index), 1.0);
}
