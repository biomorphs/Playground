#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in mat4 instance_modelmat;
layout(location = 6) in vec4 instance_colour;

uniform mat4 ProjectionMat;
uniform mat4 ViewMat;
out vec4 out_colour;
out vec2 out_uv;

void main()
{
	vec4 pos = vec4(position,1);
	vec4 v = ProjectionMat * ViewMat * instance_modelmat * pos; 
    out_colour = instance_colour;
	out_uv = uv;	// doesn't work for cubes!
    gl_Position = v;
}