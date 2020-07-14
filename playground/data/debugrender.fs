#version 430 
#pragma sde include "shared.fs"

in vec4 out_colour;
out vec4 colour;
 
void main()
{
	colour = linearToSRGB(out_colour);
}