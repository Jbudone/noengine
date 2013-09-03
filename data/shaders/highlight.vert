#version 130
in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_Texcoord;
out vec3 out_Color;

uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform mat4 MVP;

void main()
{
	out_Color = ambient;	
	gl_Position = vec4(in_Position,1.0) * MVP;
}

