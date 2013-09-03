#version 130
in vec3 out_Color;

uniform sampler2D Tex;
uniform sampler2D Bump;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
void main()
{
	gl_FragColor = vec4(1.0,0.8,0.0,0.6);
}

