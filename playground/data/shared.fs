// smol renderer shared fragment shader data

#pragma sde include "global_uniforms.h"

// utility functions
const float c_gamma = 2.2;

vec4 srgbToLinear(vec4 v)
{
	return vec4(pow(v.rgb, vec3(c_gamma)), v.a);
}

vec4 linearToSRGB(vec4 v)
{
	return vec4(pow(v.rgb, vec3(1.0 / c_gamma)), v.a);
}