#version 330 core
//in vec3 uvsOut;
out vec4 colour;

//uniform sampler2DArray MyTexture;
 
void main(){
    //colour = texture(MyTexture, vec3(uvsOut));
	colour = vec4(1,0,0,1);
}