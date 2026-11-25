// SPDX-License-Identifier: MIT
shader_type spatial;

uniform vec3 stone : source_color = vec3(0.30, 0.33, 0.34);
uniform vec3 aggregate : source_color = vec3(0.63, 0.56, 0.46);
uniform float tile_width = 0.60;
varying vec3 world_position;

vec2 hash2(vec2 point) {
	return fract(sin(vec2(dot(point, vec2(127.1, 311.7)), dot(point, vec2(269.5, 183.3)))) * 43758.5453);
}

void vertex() {
	world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

void fragment() {
	vec2 grid = world_position.xz / tile_width;
	vec2 tile_edge = min(fract(grid), 1.0 - fract(grid));
	float grout = 1.0 - smoothstep(0.005, 0.018, min(tile_edge.x, tile_edge.y));
	float tile_tone = hash2(floor(grid)).x;
	vec2 grains = world_position.xz * 127.0;
	vec2 seed = hash2(floor(grains));
	float chip = 1.0 - smoothstep(0.26, 0.41, length(fract(grains) - (0.20 + 0.60 * seed)));
	float detail = 1.0 - smoothstep(0.45, 0.95, length(fwidth(grains)));
	vec3 base = stone * (0.92 + 0.15 * tile_tone);
	ALBEDO = mix(mix(base, aggregate * (0.45 + 0.55 * seed.y), chip * detail * 0.60),
		stone * 0.46, grout);
	ROUGHNESS = mix(0.50 + 0.20 * seed.y * detail, 0.91, grout);
}
