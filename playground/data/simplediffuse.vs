#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in mat4 instance_modelmat;
layout(location = 7) in vec4 instance_colour;

struct LightInfo
{
	vec4 ColourAndAmbient;
	vec3 Position;
};

layout(std140, binding = 0) uniform Globals
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	LightInfo Light;
};

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
