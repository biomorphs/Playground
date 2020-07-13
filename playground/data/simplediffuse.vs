#version 430
#pragma sde include "shared.vs"

out vec4 out_colour;
out vec3 out_normal;
out vec2 out_uv;
out vec3 out_position;

void main()
{
	vec4 pos = vec4(position,1);
	vec4 v = ProjectionMatrix * ViewMatrix * instance_modelmat * pos; 
    out_colour = instance_colour;
	out_normal = mat3(transpose(inverse(instance_modelmat))) * normal; 
	out_uv = uv;
	out_position = vec3(instance_modelmat * pos);
    gl_Position = v;
}
