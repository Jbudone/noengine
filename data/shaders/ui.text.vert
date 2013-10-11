#version 130
attribute vec4 coord;
varying vec2 texcoord;

void main() {
	texcoord = coord.zw;
	gl_Position = vec4(coord.x, coord.y, -1.0, 1.0);
}
