#version 130

varying vec2 texcoord;
uniform sampler2D Tex4;

void main() {
	vec4 tex = texture2D(Tex4, texcoord);
	//gl_FragColor = texture(Tex4, texcoord);
	gl_FragColor = vec4(0.0, 0.0, 0.0, tex.r);
	//gl_FragColor = vec4(1, 1, 1, 1);
}
