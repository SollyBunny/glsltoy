#version 330 core

precision highp float;

#define MAXTEXTURE 80

#define REPEATS 2.0
#define PI 3.1415
#define BLURRADIUS 0.1
#define BLURSTEP 0.05

out vec4 FragColor;

uniform vec2 size;
uniform vec2 mouse;
uniform float time;
uniform bool showpictures;

layout (std140) uniform Pictures {
    vec4 pictures[MAXTEXTURE];
};

uniform sampler2D textures[MAXTEXTURE];



vec3 colorAt(vec2 coord) {
	for (int i = 0; i < MAXTEXTURE; ++i) {
		if (pictures[i][0] == -1.0) continue;
		float x = coord.x - pictures[i][1];
		if (x < 0.0) continue;
		float y = coord.y - pictures[i][2];
		if (y < 0.0) continue;
		float w = float(int(pictures[i][3]) >> 16);
		float h = float(int(pictures[i][3]) & 0xffff);
		if (x > w) continue;
		if (y > h) continue;
		vec2 pos = vec2(x / w, y / h);
		return texture(textures[int(pictures[i][0])], pos).rgb;
	}
	vec3 color = vec3(0.0, 0.0, 0.0);
	float total = 0;
	if (showpictures) {
		vec2 coordold = coord;
		for (float i = 0.0; i < 3.0; ++i) {
			coord = coordold;
			coord.x += sin(coord.y / size.y + (i / 3.0) * 2.0 * PI) * 15.0;
			coord.y += cos(coord.x / size.x + (i / 3.0) * 2.0 * PI) * 15.0;
			for (int i = 0; i < MAXTEXTURE; ++i) {
				if (pictures[i][0] == -1.0) continue;
				float x = coord.x - pictures[i][1];
				if (x < 0.0) continue;
				float y = coord.y - pictures[i][2];
				if (y < 0.0) continue;
				float w = float(int(pictures[i][3]) >> 16);
				float h = float(int(pictures[i][3]) & 0xffff);
				if (x > w) continue;
				if (y > h) continue;
				vec2 pos = vec2(x / w, y / h);
				total += 1;
				color += texture(textures[int(pictures[i][0])], pos).rgb;
				break;
			}
		}
	} else {
		for (int i = 0; i < MAXTEXTURE; ++i) {
			if (pictures[i][0] == -1.0) continue;
			float w = float(int(pictures[i][3]) >> 16);
			float h = float(int(pictures[i][3]) & 0xffff);
			float x = coord.x - pictures[i][1] + w / 2.0;
			float y = coord.y - pictures[i][2] + h / 2.0;
			float dissquared = x * x + y * y;
			float dis = sqrt(dissquared);
			total += (1 / dis);
			color += texture(textures[int(pictures[i][0])], normalize(vec2(x, y))).rgb * (1 / dis);
		}
	}
	color /= total * 2;
	return color;
}

void main() {
	vec2 coord = vec2(gl_FragCoord.x, size.y - gl_FragCoord.y);
	FragColor.rgb = colorAt(coord);
}
