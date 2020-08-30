#version 430 
#pragma sde include "shared.fs"

in vec4 vs_out_colour;
in vec3 vs_out_normal;
in vec2 vs_out_uv;
in vec3 vs_out_position;
in mat3 vs_out_tbnMatrix;
out vec4 fs_out_colour;

uniform vec4 MeshAmbient;
uniform vec4 MeshDiffuseOpacity;
uniform vec4 MeshSpecular;	//r,g,b,strength
uniform float MeshShininess;

uniform sampler2D DiffuseTexture;
uniform sampler2D NormalsTexture;
uniform sampler2D SpecularTexture;
 
void main()
{
	// early out if we can
	vec4 diffuseTex = srgbToLinear(texture(DiffuseTexture, vs_out_uv));	
	if(diffuseTex.a == 0.0 || MeshDiffuseOpacity.a == 0.0)
		discard;

	vec3 finalNormal = texture(NormalsTexture, vs_out_uv).rgb;
	vec3 finalColour = vec3(0.0);
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
		vec3 diffuse = MeshDiffuseOpacity.rgb * diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * diffuseFactor;

		// ambient light
		vec3 ambient = MeshAmbient.rgb * diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * Lights[i].ColourAndAmbient.a;

		// specular light 
		vec3 viewDir = normalize(CameraPosition.xyz - vs_out_position);
		vec3 reflectDir = normalize(reflect(-lightDir, finalNormal));  
		float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), MeshShininess);
		vec3 specularColour = MeshSpecular.rgb * Lights[i].ColourAndAmbient.rgb;
		vec3 specular = MeshSpecular.a * specFactor * specularColour * specularTex; 

		finalColour += attenuation * (ambient + diffuse + specular);
	}
	
	// tonemap
	finalColour = Tonemap_ACESFilm(vs_out_colour.rgb * finalColour * HDRExposure);
	fs_out_colour = vec4(linearToSRGB(finalColour),MeshDiffuseOpacity.a * diffuseTex.a);
}