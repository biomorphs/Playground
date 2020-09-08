#version 430 
#pragma sde include "shared.fs"

in vec4 vs_out_colour;
in vec3 vs_out_normal;
in vec4 vs_out_positionLightSpace;
in vec2 vs_out_uv;
in vec3 vs_out_position;
in mat3 vs_out_tbnMatrix;
out vec4 fs_out_colour;

uniform vec4 MeshDiffuseOpacity;
uniform vec4 MeshSpecular;	//r,g,b,strength
uniform float MeshShininess;

uniform sampler2D DiffuseTexture;
uniform sampler2D NormalsTexture;
uniform sampler2D SpecularTexture;
uniform sampler2D ShadowMapTexture;

float CalculateShadows(vec3 normal, vec4 lightSpacePos)
{
	// perform perspective divide
	vec3 projCoords = vs_out_positionLightSpace.xyz / vs_out_positionLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;	// transform from ndc space since depth map is 0-1
	if(projCoords.z > 1.0)	// early out if out of range of shadow map 
	{
        return 0.0;
	}
	if(projCoords.x > 1.0f || projCoords.y > 1.0f || projCoords.x < 0.0f || projCoords.y < 0.0f)
	{
		return 0.0;
	}

	float currentDepth = projCoords.z;
	float bias = 0.0005;
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(ShadowMapTexture, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(ShadowMapTexture, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;
	return shadow;
}
 
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

	// shadows 
	float shadowValue = CalculateShadows(finalNormal, vs_out_positionLightSpace);

	for(int i=0;i<LightCount;++i)
	{
		float attenuation;
		vec3 lightDir;

		if(Lights[i].Position.w == 0.0)		// directional light
		{
			attenuation = 1.0;
			lightDir = normalize(Lights[i].Position.xyz);
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
		vec3 ambient = diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * Lights[i].ColourAndAmbient.a;

		// specular light 
		vec3 viewDir = normalize(CameraPosition.xyz - vs_out_position);
		vec3 reflectDir = normalize(reflect(-lightDir, finalNormal));  
		float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), MeshShininess);
		vec3 specularColour = MeshSpecular.rgb * Lights[i].ColourAndAmbient.rgb;
		vec3 specular = MeshSpecular.a * specFactor * specularColour * specularTex; 

		vec3 diffuseSpec = (1.0-shadowValue) * (diffuse + specular);
		finalColour += attenuation * (ambient + diffuseSpec);
	}
	
	// tonemap
	finalColour = Tonemap_ACESFilm(vs_out_colour.rgb * finalColour * HDRExposure);
	fs_out_colour = vec4(linearToSRGB(finalColour),MeshDiffuseOpacity.a * diffuseTex.a);
}