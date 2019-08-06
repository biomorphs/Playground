#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in mat4 instance_modelmat;
layout(location = 5) in vec4 instance_colour;

uniform mat4 ProjectionMat;
out vec4 out_colour;
out vec2 out_uv;

void main()
{
	vec4 pos = vec4(position,0,1);
	vec4 v = ProjectionMat * instance_modelmat * pos; 
    out_colour = instance_colour;
	out_uv = position;	// uvs can be took straight from the position as we always draw a (0,0)-(1,1) quad
    gl_Position = v;
}
