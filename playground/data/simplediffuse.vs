#version 430
#pragma sde include "shared.vs"

out vec4 vs_out_colour;
out vec3 vs_out_normal;
out vec4 vs_out_positionLightSpace;
out vec2 vs_out_uv;
out vec3 vs_out_position;
out mat3 vs_out_tbnMatrix;

void main()
{
	vec4 pos = vec4(vs_in_position,1);
	vec4 viewSpacePos = ProjectionViewMatrix * vs_in_instance_modelmat * pos; 
    vs_out_colour = vs_in_instance_colour;
	vs_out_normal = mat3(transpose(inverse(vs_in_instance_modelmat))) * vs_in_normal; 
	vs_out_uv = vs_in_uv;
	vec3 worldSpacePos = vec3(vs_in_instance_modelmat * pos);
	vs_out_position = worldSpacePos;
	vs_out_positionLightSpace = ShadowLightSpaceMatrix * vec4(worldSpacePos,1.0);
	vs_out_tbnMatrix = CalculateTBN(vs_in_instance_modelmat, vs_in_tangent, vs_in_normal);
    gl_Position = viewSpacePos;
}
