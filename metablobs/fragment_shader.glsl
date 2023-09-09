#version 330 core

#define BLOBS 20

out vec4 FragColor;

uniform vec2 size;
uniform float time;
uniform vec2 mouse;
uniform int btn;

layout (std140) uniform Blobs {
    vec3 pos[BLOBS];  // You can adjust the size as needed
};

uniform vec3 bg, fg, hg;
vec3 lava(float t) {
    t = clamp(t, 0.0, 1.0);
    return mix(mix(bg, fg, log(t)), hg, t > 0.8 ? (t - 0.8) * 5 : 0.0);
}

uniform sampler2D noiseTexture;
uniform vec2 noiseOffset;
void main() {

    float index = 0;
    float dis;
    vec4 noise;
    noise = texture(noiseTexture, vec2(gl_FragCoord) / 256.0 + noiseOffset);
    float x = gl_FragCoord.x + noise.r * 30.f;
    float y = gl_FragCoord.y + noise.g * 30.f;
    for (int i = 0; i < BLOBS; ++i) {
        dis = (
            pow(pos[i].x - x, 2) +
            pow(pos[i].y - y, 2)
        );
        if (dis < 10.0) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
        index += 1000.0 / dis * sqrt(abs(pos[i].z));
    }

    FragColor = vec4(lava(index), 1.0);
}
