#version 430 
#pragma sde include "shared.fs"

in vec4 out_colour;
out vec4 colour;
 
void main()
{
	colour = linearToSRGB(vec4(Tonemap_ACESFilm(out_colour.rgb),out_colour.a));
}