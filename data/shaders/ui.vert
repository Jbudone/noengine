#version 130
in vec2 in_Position;
out vec3 out_Color;

void main() {
	out_Color = vec3(1.0, 0.0, 0.0);
	gl_Position = vec4(in_Position, -1.0, 1.0);
}

