#version 430
#pragma sde include "shared.vs"

out vec3 vs_out_position;

void main()
{
	vec4 pos = vec4(vs_in_position,1);
	vec4 worldPos = vs_in_instance_modelmat * pos;
	vs_out_position = worldPos.xyz;
	gl_Position = ShadowLightSpaceMatrix * worldPos; 
}
