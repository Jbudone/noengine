#version 130
in vec2 out_texCoord;

uniform vec4 borderWidths;
uniform vec2 texSize;
uniform vec2 winSize;


uniform sampler2D Tex2;

void main() {
	gl_FragColor = texture(Tex2, out_texCoord);
}

