#version 430
#pragma sde include "shared.vs"

void main()
{
	vec4 pos = vec4(vs_in_position,1);
	gl_Position = ShadowLightSpaceMatrix * vs_in_instance_modelmat * pos; 
}
