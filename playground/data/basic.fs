#version 430 
#pragma sde include "shared.fs"

in vec4 out_colour;
in vec2 out_uv;
out vec4 colour;
uniform sampler2D MyTexture;
 
void main(){
	colour = linearToSRGB(srgbToLinear(texture(MyTexture, out_uv)) * out_colour);
}