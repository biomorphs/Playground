#version 430
#pragma sde include "global_uniforms.h"

layout(location = 0) in vec4 pos_modelSpace;
layout(location = 1) in vec4 colour;
layout(location = 2) in mat4 instance_modelmat;
layout(location = 6) in vec4 instance_colour;

out vec4 out_colour;

void main()
{
	vec4 pos = vec4(pos_modelSpace.xyz,1);
	vec4 v = ProjectionMatrix * ViewMatrix * instance_modelmat * pos; 
    out_colour = colour;
    gl_Position = v;
}
