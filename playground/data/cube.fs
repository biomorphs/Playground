#version 330 core
in vec4 out_colour;
in vec2 out_uv;
out vec4 colour;

uniform sampler2D MyTexture;
 
void main(){
	colour = texture(MyTexture, out_uv) * out_colour;
}