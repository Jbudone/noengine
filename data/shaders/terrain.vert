#version 130
in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_Slope;
uniform mat4 MVP;
uniform vec3 in_color;
out vec3 out_color;
out vec3 position;
out vec3 wNorm;
out float coverage_snow;
out float coverage_rock;
out float coverage_grass;
void main()
{
	if (in_color == vec3(0,0,0)) {
		out_color = in_color;

		position = in_Position;
		wNorm = in_Normal;

		/* Splatting
		 * 
		 * Find the coverage based off of the vert elevation
		 * and elevation bands
		 *
		 * TODO: Blending techniques between bands
		 ******************************************/

		// Bands: <= X means start covering this
		float band_sand = 0;
		float band_grass = 20;
		float band_rock = 50;
		float band_snow = 170;
		coverage_snow = 0.0;
		coverage_rock = 0.0;
		coverage_grass = 0.0;
		float y = in_Position.y;

		float band_grass_min = 0;
		float band_grass_max = 1;
		float band_rock_min = 1;
		float band_rock_max = 75;
		float band_snow_min = 65;
		float band_snow_max = 80;
		/*if (abs(in_Normal.y)>0.7) {*/
			coverage_grass = max(0.0,
					(band_grass_max-band_grass_min-abs(y-band_grass_max))/(band_grass_max-band_grass_min));
		/*}*/
		if (y<=10) coverage_grass = 1;
		coverage_rock = max(0.0,
		(band_rock_max-band_rock_min-abs(y-band_rock_max))/(band_rock_max-band_rock_min));
		if (y>=band_rock_max) {
			coverage_snow = 1;
		} else {
			coverage_snow = max(0.0,
					(band_snow_max-band_snow_min-abs(y-band_snow_max))/(band_snow_max-band_snow_min));
		}
		if (coverage_grass + coverage_rock + coverage_snow <
		1) {
			coverage_rock = 1 - (coverage_grass +
			coverage_rock + coverage_snow);
		}

		coverage_grass = 1.0;
		coverage_rock = 0.0;
		

	} else {
		out_color = in_color;
	}

	gl_Position = vec4(in_Position*vec3(1,-1,1),1.0) * MVP;
}

