#version 430
#pragma sde include "shared.vs"

out vec4 vs_out_colour;
out vec3 vs_out_normal;
out vec2 vs_out_uv;
out vec3 vs_out_position;
out mat3 vs_out_tbnMatrix;

void main()
{
	vec4 pos = vec4(vs_in_position,1);
	vec4 viewSpacePos = ProjectionMatrix * ViewMatrix * vs_in_instance_modelmat * pos; 
    vs_out_colour = vs_in_instance_colour;
	vs_out_normal = mat3(transpose(inverse(vs_in_instance_modelmat))) * vs_in_normal; 
	vs_out_uv = vs_in_uv;
	vs_out_position = vec3(vs_in_instance_modelmat * pos);
	vs_out_tbnMatrix = CalculateTBN(vs_in_instance_modelmat, vs_in_tangent, vs_in_normal);
    gl_Position = viewSpacePos;
}
