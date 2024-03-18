#version 330 core

#define BLOBS 100
#define PI 3.1415

out vec4 FragColor;

precision highp float;

uniform vec2 size;
uniform float time;
uniform vec2 mouse;

layout (std140) uniform Blobs {
    vec4 pos[BLOBS];
};

float hue2rgb(float p, float q, float t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}

vec3 HSLtoRGB(vec3 hsl) {
    vec3 rgb;
    if (hsl.g == 0) {
        rgb.r = rgb.g = rgb.b = hsl.b;
    } else {
        float q = (hsl.b < 0.5) ? (hsl.b * (1.0 + hsl.g)) : (hsl.b + hsl.g - hsl.b * hsl.g);
        float p = 2.0 * hsl.b - q;
        rgb.r = hue2rgb(p, q, hsl.r + 1.0 / 3.0);
        rgb.g = hue2rgb(p, q, hsl.r);
        rgb.b = hue2rgb(p, q, hsl.r - 1.0 / 3.0);
    }
    return rgb;
}

uniform vec3 bg, fg, hg;
uniform sampler2D noiseTexture;
uniform vec2 noiseOffset;

float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

vec3 lava(float t) {
    float tmp = (sin(gl_FragCoord.x / size.x * 3.0 + time / 20.0) + 1.0) / 2.0;
    float tmp2 = (cos(gl_FragCoord.y / size.x * 3.0 - time / 30.0) + 1.0) / 2.0;
    vec4 noise = texture(noiseTexture, vec2(tmp, tmp2));
    #define BG bg + (vec3(noise.r, noise.a, 0.5) - 0.5) / 2.0
    #define FG fg + (vec3(noise.g, noise.r, 0.5) - 0.5) / 2.0
    #define HG hg + (vec3(noise.b, noise.g, 0.5) - 0.5) / 2.0
    vec3 color;
    t = clamp(t, 0.0, 1.0);
    t *= t;
    color = mix(HSLtoRGB(BG), HSLtoRGB(FG), smoothstep(tmp / 5.0 - 0.1, tmp / 3.0 + 0.1, t));
    color = mix(color, HSLtoRGB(HG), smoothstep(tmp / 3.0 + tmp2 / 3.0 + 0.1, tmp / 3.0 + tmp2 / 6.0 + 0.5, t));
    color = clamp(color, 0.3, 0.8);
    return color;
}

void main() {

    float index = 0;
    float dis;

    vec4 noise;
    noise = texture(noiseTexture, vec2(gl_FragCoord) / 256.0 + noiseOffset);
    vec2 coord = vec2(
        gl_FragCoord.x + noise.r * 30.0,
        gl_FragCoord.y + noise.g * 30.0
    );
    // coord = (1.0 - cos(PI * coord / size)) / 2.0 * size;
    // coord += vec2(1, 1) * sin(atan(coord.y, coord.x) / 5.0 + noise.r) * 5.0;
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
    dis = (
    	pow(mouse.x - gl_FragCoord.x, 2) +
        pow(mouse.y - gl_FragCoord.y, 2)
    );
    index += 1.0 / dis * (1000.0 + 500.0 * sin((time + cos(coord.x * coord.y)) / 20.0));
	
	noise = texture(noiseTexture, coord / 1024.0 + noiseOffset);
    FragColor.rgb = (lava(index) + vec3(noise) / 1.2) * 0.8;
}
