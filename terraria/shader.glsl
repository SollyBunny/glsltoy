#version 330 core
#extension GL_NV_gpu_shader5 : enable

#define BLOCKSIZE 40.0

layout (std140) uniform World {
	uint8_t data[1920 * 1080 / int(BLOCKSIZE) / int(BLOCKSIZE)];
};

out vec4 FragColor;

uniform vec2 size;
uniform float time;
uniform vec3 mouse;
uniform float worldwidth;

uniform sampler2D atlas;
uniform float atlaswidth;
uniform float atlasheight;

//
// GLSL textureless classic 2D noise "cnoise",
// with an RSL-style periodic variant "pnoise".
// Author:	Stefan Gustavson (stefan.gustavson@liu.se)
// Version: 2011-08-22
//
// Many thanks to Ian McEwan of Ashima Arts for the
// ideas for permutation and gradient selection.
//
// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license. See LICENSE file.
// https://github.com/stegu/webgl-noise
//

vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x*34.0)+10.0)*x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }
vec2 fade(vec2 t) { return t*t*t*(t*(t*6.0-15.0)+10.0); }
float cnoise(vec2 P) {
	vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
	vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
	Pi = mod289(Pi); // To avoid truncation effects in permutation
	vec4 ix = Pi.xzxz;
	vec4 iy = Pi.yyww;
	vec4 fx = Pf.xzxz;
	vec4 fy = Pf.yyww;
	vec4 i = permute(permute(ix) + iy);
	vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0 ;
	vec4 gy = abs(gx) - 0.5 ;
	vec4 tx = floor(gx + 0.5);
	gx = gx - tx;
	vec2 g00 = vec2(gx.x,gy.x);
	vec2 g10 = vec2(gx.y,gy.y);
	vec2 g01 = vec2(gx.z,gy.z);
	vec2 g11 = vec2(gx.w,gy.w);
	vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
	g00 *= norm.x;
	g01 *= norm.y;
	g10 *= norm.z;
	g11 *= norm.w;
	float n00 = dot(g00, vec2(fx.x, fy.x));
	float n10 = dot(g10, vec2(fx.y, fy.y));
	float n01 = dot(g01, vec2(fx.z, fy.z));
	float n11 = dot(g11, vec2(fx.w, fy.w));
	vec2 fade_xy = fade(Pf.xy);
	vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
	float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
	return 2.3 * n_xy;
}

void main() {

	float x = floor(gl_FragCoord.x / BLOCKSIZE);
	float y = floor(gl_FragCoord.y / BLOCKSIZE);
	float xs = mod(gl_FragCoord.x, BLOCKSIZE);
	float ys = mod(gl_FragCoord.y, BLOCKSIZE);

	int index = int(
		y * worldwidth + x
	);

	uint8_t block = data[index];
	if (block == uint8_t(0)) {
		float noise = cnoise(vec2(
			x * 100.0,
			y * 100.0
		));
		if (noise > 0.5) {
			data[index] = uint8_t(1);
		} else {
			data[index] = uint8_t(2);
		}
	}

	// FragColor = vec4(1.0 - col, col, y * 10.0 , 1.0);
	// FragColor = vec4(col, x / 10.0, y / 10.0 , 1.0);
	FragColor = texture(atlas, vec2(
		(float(block) * BLOCKSIZE + xs) / atlaswidth,
		(ys) / atlasheight
	));
	FragColor.r = float(block) / 128.0;
	// vec2 dist = vec2(gl_FragCoord) - vec2(mouse);
	// float radius = length(dist);
	// if (radius < 100) {
	// 	coord += dist * cos((1 - radius / 100) * 2 * 3.14);
	// }

	// for (int i = 0; i < BLOBS; ++i) {
	// 	dis = (
	// 		pow(pos[i].x - coord.x, 2) +
	// 		pow(pos[i].y - coord.y, 2)
	// 	);
	// 	if (dis > 5e6) continue;
	// 	// if (dis < 10.0) {
	// 	//	 FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	// 	//	 return;
	// 	// }
	// 	index += 1.0 / dis * pos[i].w;
	// }
	// FragColor = vec4(lava(index), 1.0);
}
