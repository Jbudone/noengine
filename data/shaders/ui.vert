#version 130
in vec2 in_Position;
in vec2 in_Texcoord;

uniform vec4 borderWidths;
uniform vec2 texSize;
uniform vec2 winSize;

out vec2 out_texCoord;

void main() {
	out_texCoord     = in_Texcoord;
	gl_Position = vec4(in_Position, -1.0, 1.0);
}

