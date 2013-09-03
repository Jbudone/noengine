#version 130
in vec2 out_texCoord;
varying vec3 out_Color;
varying float out_ldotn;
varying vec3 out_Position;

uniform sampler2D Tex;
uniform sampler2D Bump;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
struct Light {
	vec3 position;
	vec3 diffuse;
};
uniform Light lights[10];
void main() {

	vec3 norm = vec3(texture(Bump, out_texCoord));
	vec3 light_Norm = vec3(0.64, 0.64, 0.425);
	float ldotn = max(dot(norm, light_Norm), 0.0);
	vec4 mossTexColor = ldotn * texture(Tex, out_texCoord);

	vec3 color;
	color = ambient * 0.2;
	for (int i=0; i<10; i++) {
		vec3 lightDir = normalize(vec3(vec4(vec3(lights[i].position - out_Position), 1.0)));
		float lDotN = max(dot(lightDir, norm), 0.0);

		color += lDotN * lights[i].diffuse;
	}

	//vec3 color = ldotn * diffuse + ambient;
	gl_FragColor = vec4(color,1.0) + mossTexColor;
}


