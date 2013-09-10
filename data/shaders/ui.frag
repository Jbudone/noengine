#version 130
in vec2 out_texCoord;

uniform sampler2D Tex2;

void main() {
	gl_FragColor = texture(Tex2, out_texCoord);
}

