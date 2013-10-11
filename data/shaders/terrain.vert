#version 130
in vec3 in_Position;
uniform mat4 MVP;
out float dotn;
uniform vec3 in_color;
out vec3 out_color;
void main()
{
	vec3 light_norm = vec3(0.64, 0.64, 0.425);
	float lDotN = max(dot(in_Position, light_norm), 0.0);
	dotn = lDotN;
	out_color = in_color;

	gl_Position = vec4(in_Position,1.0) * MVP;
}

