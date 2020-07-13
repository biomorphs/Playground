// smol renderer shared vertex shader data

// Shared mesh layout
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in mat4 instance_modelmat;
layout(location = 7) in vec4 instance_colour;

#pragma sde include "global_uniforms.h"