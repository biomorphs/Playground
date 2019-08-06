#version 330 core
layout(location = 0) in vec2 position;

uniform mat4 ProjectionMat;
uniform mat4 ModelMat;
//out vec3 uvsOut;

void main()
{
	vec4 v = ProjectionMat * ModelMat * vec4(position,0,1); 
    gl_Position = v;
}
