#version 330 core
layout(location = 0) in vec2 position;

uniform mat4 ProjectionMat;
uniform mat4 ModelMat;
uniform vec4 QuadColour;

out vec4 out_colour;

void main()
{
	vec4 v = ProjectionMat * ModelMat * vec4(position,0,1); 
    gl_Position = v;
	out_colour = QuadColour;
}
