#version 130

in vec3 out_color;
in vec3 wNorm;
in vec3 position;
in float coverage_snow;
in float coverage_rock;
in float coverage_grass;
uniform sampler2DArray Tex;
uniform sampler2D TexDetail;
uniform sampler2D TexBump;
uniform sampler2D TexSnow;
uniform sampler2D TexGrass;
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 texSize=vec2(512*3,512);
vec3 texBounds_rock=vec3(0,0,512/texSize.x-1/texSize.x);
vec3 texBounds_snow=vec3(512/texSize.x,0,512/texSize.x-1/texSize.x);
vec3 texBounds_grass=vec3(1024/texSize.x,0,512/texSize.x-1/texSize.x);

int TEX_ROCK=1;
int TEX_GRASS=2;
int TEX_SNOW=3;

int TEX_BUMP=4;
int TEX_TSNOW=5;
int TEX_TGRASS=6;
int TEX_TDETAIL=7;

/* TriPlanar texture mapping
 *
 * Blends the texture fetch routine between 3 different
 * fetches of the texture to help in avoiding stretches
 * along sloped geometry
 *****************************************************/
vec3 boundCoords(int texType, vec2 coords) {
	vec3 bound;
	if (texType==TEX_ROCK) bound = vec3(coords.xy,4);
	else if (texType==TEX_GRASS) bound = vec3(coords.xy,3);
	else if (texType==TEX_SNOW) bound = vec3(coords.xy,0);
	else return vec3(coords,-1);


	/*vec2 newCoords = bound.xy + vec2(bound.z,1) * fract(coords);*/
	/*vec2 newCoords = vec2(0,0.1) + vec2(bound.z,0.5) * fract(coords);*/
	//vec2 newCoords = vec2(bound.z,1) * fract(coords);
	return bound;


	//if (newCoords.x >= bound.z) newCoords.x=0;
	//if (newCoords.x > bound.x+bound.z) newCoords.x -= bound.z*int(newCoords.x/(bound.x+bound.z));
	///*if (newCoords.y > bound.y+bound.z) newCoords.y -= bound.z*int(newCoords.y/(bound.x+bound.z));*/
	//return newCoords;
}
vec4 texTriPlanarBlend(sampler2D tex, vec2 coordsXY, vec2 coordsXZ, vec2 coordsYZ, vec3 blend) {
	vec4 color;
	vec4 tex1 = texture( tex, coordsXY );
	vec4 tex2 = texture( tex, coordsXZ );
	vec4 tex3 = texture( tex, coordsYZ );
	color = tex1 * blend.z + tex2 * blend.y + tex3 * blend.x;
	return color;
}
vec4 texTriPlanarBlendArr(sampler2DArray tex, int depth, vec2 coordsXY, vec2 coordsXZ, vec2 coordsYZ, vec3 blend) {
	vec4 color;
	vec4 tex1 = texture( tex, vec3(coordsXY,depth) );
	vec4 tex2 = texture( tex, vec3(coordsXZ,depth) );
	vec4 tex3 = texture( tex, vec3(coordsYZ,depth) );
	color = tex1 * blend.z + tex2 * blend.y + tex3 * blend.x;
	return color;
}
sampler2D texFetchFromID(int id) {
	if (id==TEX_BUMP) return TexBump;
	else if (id==TEX_TSNOW) return TexSnow;
	else if (id==TEX_TGRASS) return TexGrass;
	else if (id==TEX_TDETAIL) return TexDetail;
	return TexDetail;
}
vec4 texTriPlanar(int texType, vec3 coords, float scaleCoords, vec3 blend) {
	vec3 coords1 = boundCoords(texType,coords.xy*scaleCoords);
	vec3 coords2 = boundCoords(texType,coords.xz*scaleCoords);
	vec3 coords3 = boundCoords(texType,coords.yz*scaleCoords);
	vec4 color;
	if (coords1.z==-1) {
		color = texTriPlanarBlend(texFetchFromID(texType), coords1.xy, coords2.xy, coords3.xy, blend);
	} else {
		color = texTriPlanarBlendArr(Tex, int(coords1.z), coords1.xy, coords2.xy, coords3.xy, blend);
	}
	return color;
}

void main() {

	vec3 blending;
	if (out_color == vec3(0,0,0)) {

	float scaleTex = 2.5*1;
	float scaleTex2 = -2.5*4;

	// in wNorm is the world-space normal of the fragment
	blending = abs( wNorm );
	blending = normalize(max(blending, 0.00001)); // Force weights to sum to 1.0
	float b = (blending.x + blending.y + blending.z);
	blending /= vec3(b, b, b);

	/* Texture fetching
	 *
	 * Uses a low scale texture fetch to avoid tiling
	 * artifacts
	 ***********************************************/
	vec4 color = texTriPlanar(TEX_ROCK,position,1/scaleTex,blending);
	vec4 colorM = texTriPlanar(TEX_ROCK,position,1/scaleTex2,blending);
	vec4 colorB = texTriPlanar(TEX_BUMP,position,1/scaleTex,blending);
	vec4 colorBM = texTriPlanar(TEX_BUMP,position,1/scaleTex2,blending);
	vec4 colorGrass = texTriPlanar(TEX_GRASS,position,1/scaleTex,blending);
	vec4 colorGrassM = texTriPlanar(TEX_GRASS,position,1/scaleTex2,blending);
	vec4 colorSnow = texTriPlanar(TEX_SNOW,position,1/scaleTex,blending);
	vec4 colorSnowM = texTriPlanar(TEX_SNOW,position,1/scaleTex2,blending);

	float scale=1/(2.5*4);
	color = vec4(color.xyz * colorM.xyz, 1.0) * 4;
	colorGrass = vec4(colorGrass.xyz * colorGrassM.xyz, 1.0) * 4;
	colorSnow = vec4(colorSnow.xyz * colorSnowM.xyz, 1.0) * 1;
	colorB = vec4(colorB.xyz * colorBM.xyz, 1.0) * 2;

	/* TODO: Applied detail texture. Is it necessary to do a
	 * triplanar fetch for something so high scale?
	 ******************************************************/
	/*vec4 detail = texture(TexDetail, vec2(inX*scale,inZ*scale)*8)  * vec4(1.0)*1.5;*/
	vec4 detail = texTriPlanar(TEX_TDETAIL,position,scale*8,blending) * vec4(1.0)*1.5;


	color = vec4(color.rgb
		* texTriPlanar(TEX_ROCK,position,scale*-0.05,blending).rgb
		* detail.rgb * 2, 1.0);
	colorGrass = vec4(colorGrass.rgb
		* texTriPlanar(TEX_GRASS,position,scale*-0.4,blending).rgb
		/** texture(TexGrass,vec2(inX*scale, inZ*scale)*-0.8).rgb * 2.5 */
		/** texture(TexGrass,vec2(inX*scale, inZ*scale)*-0.2).rgb * 2.5 */
		* detail.rgb * 4.5, 1.0);
	colorSnow = vec4(colorSnow.rgb
		* texTriPlanar(TEX_SNOW,position,scale*-0.05,blending).rgb
		/** texture(TexSnow,vec2(inX*scale, inZ*scale)*-0.05).rgb*/
		* detail.rgb * 1.5, 1.0);
	colorB = vec4(colorB.rbg
		* texTriPlanar(TEX_BUMP,position,scale*-0.05,blending).rbg
		/** texture(TexBump,vec2(inX*scale, inZ*scale)*-0.05).rgb*/
	, 1.0);

	// Lighting properties
	vec3 lightCol = vec3(0.9, 0.7, 0.7);
	vec3 light_pos = vec3(-300, 180, -300);
	vec3 light_norm = normalize(light_pos-position);
	float lDotN = max(dot(wNorm, -light_norm), 0.0);
	float lDotNB = max(dot(colorB.xyz, -light_norm), 0.0);

	vec4 color_snow = colorSnow;// color * vec4(0.7, 1.0, 1.0, 1.0) * 9;
	vec4 color_rock = color;
	vec4 color_grass = color;//colorGrass;//color * vec4(0.2, 0.8, 0.0, 1.0) * 5;

	bool splat=true;
	if (splat) {

		// TODO: noisy blend between elavation blends
		float cover_rock = coverage_rock;
		float cover_snow = coverage_snow;
		if (coverage_grass + coverage_rock + coverage_snow < 1) {
			if (coverage_snow == 0 && coverage_grass == 0) {
				/*cover_rock = 1;*/
				/*cover_rock = (1 - (coverage_grass +*/
				/*		coverage_rock + coverage_snow))/2;*/
				/*cover_snow = 1 - (coverage_grass + cover_rock + cover_snow);*/
			}
		}
		color = color_snow * cover_snow + color_rock *
			cover_rock + color_grass * coverage_grass;
	}

	gl_FragColor = vec4((color.rgb*0.9
		+ color.rgb*lDotN*0.8*lightCol
		+ lDotNB*0.8*lightCol) * 0.75
	, 1.0);

	} else {
		gl_FragColor = vec4(out_color,1.0);
	}



	/*gl_FragColor = texTriPlanar(TEX_ROCK, position, 1, blending);*/
	/*gl_FragColor = texture(Tex, vec3(position.xz, 4));*/

	/*gl_FragColor = texture(Tex, fract(position.xz));*/
	/*gl_FragColor = texture(Tex, vec2(1/1536,1/1536) + vec2((512-1)/1536,(512-1)/1536) * fract(position.xz));*/

	/*gl_FragColor = texture(Tex, vec2(513.0f/1536.0f,1.0f/1536.0f) + vec2(510.0f/1536.0f,510.0f/1536.0f) * fract(position.xz));*/

	/*gl_FragColor = texture(Tex, vec2((4.0f*512.0f+1.0f)/4096.0f,0.0f) + vec2(510.0f/4096.0f,1.0f) * fract(position.xz));*/
}

