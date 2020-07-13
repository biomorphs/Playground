#version 430 
#pragma sde include "shared.fs"

in vec4 out_colour;
in vec3 out_normal;
in vec2 out_uv;
in vec3 out_position;
out vec4 colour;

uniform sampler2D MyTexture;
 
void main()
{
	vec3 finalColour = vec3(0.0);
	for(int i=0;i<LightCount;++i)
	{
		// diffuse light
		vec4 diffuseTex = srgbToLinear(texture(MyTexture, out_uv));
		vec3 normal = normalize(out_normal);
		vec3 lightDir = normalize(Lights[i].Position.xyz - out_position);
		float diffuseFactor = max(dot(normal, lightDir),0.0);
		vec3 diffuse = diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * diffuseFactor;

		// ambient light
		vec3 ambient = diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * Lights[i].ColourAndAmbient.a;

		finalColour += ambient + diffuse;
	}
	
	colour = linearToSRGB(min(vec4(finalColour,1.0),1.0));
}