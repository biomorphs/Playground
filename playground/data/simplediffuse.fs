#version 430 

struct LightInfo
{
	vec4 ColourAndAmbient;
	vec3 Position;
};

layout(std140, binding = 0) uniform Globals
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	LightInfo Light;
};

in vec4 out_colour;
in vec3 out_normal;
in vec2 out_uv;
in vec3 out_position;
out vec4 colour;

uniform sampler2D MyTexture;

const float gamma = 2.2;

vec4 srgbToLinear(vec4 v)
{
	return vec4(pow(v.rgb, vec3(gamma)), v.a);
}

vec4 linearToSRGB(vec4 v)
{
	return vec4(pow(v.rgb, vec3(1.0 / gamma)), v.a);
}
 
void main(){
	// diffuse light
	vec4 diffuseTex = srgbToLinear(texture(MyTexture, out_uv));
	vec3 normal = normalize(out_normal);
	vec3 lightDir = normalize(Light.Position - out_position);
	float diffuseFactor = max(dot(normal, lightDir),0.0);
	vec3 diffuse = diffuseTex.rgb * Light.ColourAndAmbient.rgb * diffuseFactor;

	// ambient light
	vec3 ambient = diffuseTex.rgb * Light.ColourAndAmbient.rgb * Light.ColourAndAmbient.a;
	
	//colour = linearToSRGB(diffuseTex * out_colour * vec4(ambient,1.0f));
	colour = linearToSRGB(min(vec4(diffuse + ambient,1.0),1.0));
}