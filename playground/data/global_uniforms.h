// Global uniforms

struct LightInfo
{
	vec4 ColourAndAmbient;
	vec4 Position;
	vec3 Attenuation;
};

layout(std140, binding = 0) uniform Globals
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	vec4 CameraPosition;	// World Space
	LightInfo Lights[64];
	int LightCount;
	float HDRExposure;
};