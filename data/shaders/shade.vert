#version 130
in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_Texcoord;
out vec3 out_Color;
out vec2 out_texCoord;
out float out_ldotn;
out vec3 out_Position;
out vec3 out_Normal;
uniform vec3 ambient = vec3(0.0, 1.0, 0.3);
uniform vec3 diffuse = vec3(1.0, 1.0, 0.3);
uniform vec3 specular = vec3(0.0, 0.0, 1.0);
uniform mat4 MVP;
struct Light {
	vec3 position;
	vec3 diffuse;
};
uniform Light lights[10];
void main()
{
	vec3 light_Norm = vec3(0.64, 0.64, .425);
	float lDotN = max(dot(in_Normal, light_Norm), 0.0);
	out_ldotn = lDotN;
	out_Normal = in_Normal;

	out_texCoord = in_Texcoord;
	out_Color = ambient * lDotN;
	gl_Position = vec4(in_Position,1.0) * MVP;
	out_Position = normalize(in_Position).xyz;
}

