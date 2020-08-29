#version 430 
#pragma sde include "shared.fs"

in vec4 vs_out_colour;
in vec3 vs_out_normal;
in vec2 vs_out_uv;
in vec3 vs_out_position;
in mat3 vs_out_tbnMatrix;
out vec4 fs_out_colour;

uniform sampler2D DiffuseTexture;
uniform sampler2D NormalsTexture;
uniform sampler2D SpecularTexture;
 
void main()
{
	vec3 finalNormal = texture(NormalsTexture, vs_out_uv).rgb;
	vec3 finalColour = vec3(0.0);
	vec4 diffuseTex = srgbToLinear(texture(DiffuseTexture, vs_out_uv));
	vec3 specularTex = texture(SpecularTexture, vs_out_uv).rgb;

	// transform normal map to world space
	finalNormal = finalNormal * 2.0 - 1.0;   
	finalNormal = normalize(vs_out_tbnMatrix * finalNormal);

	for(int i=0;i<LightCount;++i)
	{
		float attenuation;
		vec3 lightDir;

		if(Lights[i].Position.w == 0.0)		// directional light
		{
			attenuation = 1.0;
			lightDir = normalize(-Lights[i].Position.xyz);
		}
		else	// point light
		{
			float lightDistance = length(Lights[i].Position.xyz - vs_out_position);
			attenuation = 1.0 / (Lights[i].Attenuation[0] + 
								(Lights[i].Attenuation[1] * lightDistance) + 
								(Lights[i].Attenuation[2] * (lightDistance * lightDistance)));
			lightDir = normalize(Lights[i].Position.xyz - vs_out_position);
		}

		// diffuse light
		float diffuseFactor = max(dot(finalNormal, lightDir),0.0);
		vec3 diffuse = diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * diffuseFactor;

		// ambient light
		vec3 ambient = diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * Lights[i].ColourAndAmbient.a;

		// specular light 
		float specularStrength = 0.5;
		float shininess = 24.0;
		vec3 viewDir = normalize(CameraPosition.xyz - vs_out_position);
		vec3 reflectDir = reflect(-lightDir, finalNormal);  
		float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
		vec3 specularColour = Lights[i].ColourAndAmbient.rgb;
		vec3 specular = specularStrength * specFactor * specularColour * specularTex; 

		finalColour += attenuation * (ambient + diffuse + specular);
	}
	
	// tonemap
	float exposure = 1.0;
	finalColour = Tonemap_ACESFilm(vs_out_colour.rgb * finalColour * exposure);
	fs_out_colour = vec4(linearToSRGB(finalColour),1.0);
}