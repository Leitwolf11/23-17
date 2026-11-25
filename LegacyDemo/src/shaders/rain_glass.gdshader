// SPDX-License-Identifier: MIT
shader_type spatial;
render_mode unshaded, cull_disabled, shadows_disabled;

float hash(float x) {
	return fract(sin(x * 51.71) * 43758.5453);
}

void fragment() {
	float column = UV.x * 34.0;
	float lane = floor(column);
	float phase = hash(lane);
	float drop = fract(UV.y * (2.0 + phase * 2.2) - TIME * (0.13 + 0.21 * phase) + phase);
	float head = smoothstep(0.79, 0.96, drop) * (1.0 - smoothstep(0.978, 1.0, drop));
	float center = 0.17 + 0.67 * hash(lane + 2.7);
	float streak = 1.0 - smoothstep(0.022, 0.13, abs(fract(column) - center));
	float water = streak * (head + 0.16 * smoothstep(0.33, 0.9, drop));
	float edge = pow(1.0 - clamp(abs(dot(normalize(NORMAL), normalize(VIEW))), 0.0, 1.0), 2.1);
	ALBEDO = mix(vec3(0.14, 0.23, 0.28), vec3(0.48, 0.65, 0.72), min(1.0, water * 1.18 + edge * 0.24));
	ALPHA = min(0.65, 0.14 + edge * 0.26 + water * 0.50);
}
