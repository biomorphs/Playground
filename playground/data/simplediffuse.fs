#version 430 
#pragma sde include "shared.fs"

in vec4 out_colour;
in vec3 out_normal;
in vec2 out_uv;
in vec3 out_position;
out vec4 colour;

uniform sampler2D DiffuseTexture;
uniform sampler2D NormalsTexture;
uniform sampler2D SpecularTexture;
 
void main()
{
	vec3 finalColour = vec3(0.0);
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
			float lightDistance = length(Lights[i].Position.xyz - out_position);
			attenuation = 1.0 / (Lights[i].Attenuation[0] + 
								(Lights[i].Attenuation[1] * lightDistance) + 
								(Lights[i].Attenuation[2] * (lightDistance * lightDistance)));
			lightDir = normalize(Lights[i].Position.xyz - out_position);
		}

		// diffuse light
		vec4 diffuseTex = srgbToLinear(texture(DiffuseTexture, out_uv));
		vec3 normal = normalize(out_normal);
		float diffuseFactor = max(dot(normal, lightDir),0.0);
		vec3 diffuse = diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * diffuseFactor;

		// ambient light
		vec3 ambient = diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * Lights[i].ColourAndAmbient.a;

		// specular light 
		float specularStrength = 0.5;
		float shininess = 24.0;
		vec3 viewDir = normalize(CameraPosition.xyz - out_position);
		vec3 reflectDir = reflect(-lightDir, normal);  
		float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
		vec3 specularColour = Lights[i].ColourAndAmbient.rgb;
		vec3 specular = specularStrength * specFactor * specularColour * texture(SpecularTexture, out_uv).rgb; 

		finalColour += attenuation * (ambient + diffuse + specular);
	}
	
	colour = linearToSRGB(min(vec4(finalColour,1.0),1.0));
}