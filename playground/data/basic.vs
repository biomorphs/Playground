#version 430
#pragma sde include "shared.vs"

out vec4 out_colour;
out vec2 out_uv;

void main()
{
	vec4 pos = vec4(position,1);
	vec4 v = ProjectionMatrix * ViewMatrix * instance_modelmat * pos; 
    out_colour = instance_colour;
	out_uv = uv;	// doesn't work for cubes!
    gl_Position = v;
}
