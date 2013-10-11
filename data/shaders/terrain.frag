#version 130

in float dotn;
in vec3 out_color;
void main() {
	vec4 color = vec4( out_color.x, out_color.y, out_color.z, 1.0 );
	color *= dotn;

	gl_FragColor = vec4(out_color, 1.0);
}

