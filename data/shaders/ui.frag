#version 130
in vec3 out_Color;

void main() {
	gl_FragColor = vec4(out_Color.x, out_Color.y,
	out_Color.z, 1.0);
}

