#include "k_terrain.hmap.h"

Terrain::Terrain(uint gl) {
	this->gl = gl;
	construct();
	headQuad = new Quad(this);
	headQuad->parent = 0;


	// Generate sample points
	int x = 0, z = 0;
	float taco = 2;
	float inc = 1/taco;
	width = 80;
	depth = width;
	float persistence = 0.25;
	float octaves = 16;
	errorThreshold = 0.02;
	minBase = 0.05;
	maxEdgeLength = 40;
	scaleDepth = 4.0f;
	drawLines = false;
	/*
	while (x<width/taco) {
		z=0;
		while (z<depth/taco) {
			float y = perlinNoise( x, z, persistence, octaves );
			samples.push_back({x*taco,y,z*taco});
			z+=inc;
		}
		x+=inc;
	}
	*/




	/*
		int hmapWidth, hmapHeight;
		int channels = SOIL_LOAD_L;
		unsigned char* imageData = SOIL_load_image( "heightmap.png", &hmapWidth, &hmapHeight, 0, channels );
		width = 256;
		depth = 256;
		this->hmapHeight = hmapHeight;
		x = 0; z = 0;
		float minY=(imageData[0]/16)+100;
		float maxY=(imageData[0]/16)+100;
		while (x<hmapWidth) {
			z=0;
			while (z<hmapHeight) {
				float y = imageData[(x*hmapHeight+z)];
				y /= 16;
				y += 100;
				samples.push_back({(float)x,y,(float)z});
				vertices.push_back({(float)x,y,(float)z});
				++z;
				if (y < minY) minY = y;
				if (y > maxY) maxY = y;
			}
			++x;
		}
		SOIL_free_image_data( imageData );

		Log(str(format("Min Y: %1%")%minY));
		Log(str(format("Max Y: %1%")%maxY));
		*/
		this->hmapHeight = 257;
		width = 256;
		depth = 256;
		srand (time(NULL));
		// samples = (SampledVertex*)malloc(sizeof(SampledVertex)*(width*(hmapHeight)+depth+1));
		// TODO: set corners
		int maxHeight = 256;
		// float height = rand()%(maxHeight*2)-maxHeight; if (height<0) height=0;
		// samples[(uint)(0*hmapHeight+0)] = { 0, height, 0 };
		// height = rand()%(maxHeight*2)-maxHeight; if (height<0) height=0;
		// samples[(uint)(width*hmapHeight+0)] = { width, height, 0 };
		// height = rand()%(maxHeight*2)-maxHeight; if (height<0) height=0;
		// samples[(uint)(width*hmapHeight+depth)] = { width, height, depth };
		// height = rand()%(maxHeight*2)-maxHeight; if (height<0) height=0;
		// samples[(uint)(0*hmapHeight+depth)] = { 0, height, depth };
		float minY=-1, maxY=-1;
		generateTerrain2(width);
		for (x=0; x<hmapHeight; ++x) {
			for (z=0; z<hmapHeight; ++z) {

				float y = sampleHeights[(uint)(x*hmapHeight+z)];
				if (y<minY||minY==-1) minY=y;
				else if (y>maxY) maxY=y;
				vertices.push_back({ (float)(x)*scaleDepth, y, (float)(z)*scaleDepth });
				if (x<hmapHeight && z<hmapHeight && x>0 && z>0) {

					// Normal Calculation
					float   y = sampleHeights[(uint)(x*hmapHeight+z)],
							Yr = (sampleHeights[(uint)((x+1)*hmapHeight+z)]-y)/scaleDepth,
							Yu = (sampleHeights[(uint)(x*hmapHeight+(z+1))]-y)/scaleDepth,
							Yl = (sampleHeights[(uint)((x-1)*hmapHeight+z)]-y)/scaleDepth,
							Yd = (sampleHeights[(uint)(x*hmapHeight+(z-1))]-y)/scaleDepth,
							nLenRU = sqrt(Yr*Yr + Yu*Yu + 1),
							nLenLD = sqrt(Yl*Yl + Yd*Yd + 1);
					float nx = (Yr/nLenRU + Yl/nLenLD)/2,
						  ny = -2/(nLenRU + nLenLD),
						  nz = (Yu/nLenRU + Yd/nLenLD)/2;
					vertices.back().nx = nx;
					vertices.back().ny = ny;
					vertices.back().nz = nz;
					// vertices.back().nx = Yr/nLen;
					// vertices.back().ny = -1/nLen;
					// vertices.back().nz = Yu/nLen;

					// Tangent Calculation
					vertices.back().dx = (Yr+Yl)/2;
					vertices.back().dz = (Yu+Yd)/2;
				}
				// SampledVertex sample = samples[(uint)(x*hmapHeight+z)];
				// vertices.push_back({ sample.x*1.5, sample.y, sample.z*1.5 });
				// if (x>0 && z>0) {
				// 	vertices.back().normal( vertices[(uint)(x*hmapHeight+(z-1))], vertices[(uint)((x-1)*hmapHeight+z)] );
				// 	if (x<hmapHeight && z<hmapHeiht) {
				// 		vertices.back().tangent( vertices[(
				// 	}
				// }
			}
		}
		Log(str(format("Min Y: %1%")%minY));
		Log(str(format("Max Y: %1%")%maxY));

		headQuad->buildQuad(0, 0, width, depth);




	vector<Quad*> parents;
	parents.push_back(headQuad);
	ushort width=1;
	while(true) {
		width *= 2;

		// Setup grid of quads (parents children)
		ushort childCount = parents.size()*4;
		Quad* children[childCount];
		ushort i=0;
		for (auto quad : parents) {
			children[i]         = quad->childSW;
			children[i+1]       = quad->childSE;
			children[i+1+width] = quad->childNE;
			children[i+width]   = quad->childNW;

			i+=2;
			if (i%width==0) i+=width;
		}

		// setup neighbourhood
		for (i=0; i<childCount; ++i) {
			if (i+width < childCount) children[i]->north = children[i+width]; // North
			if (i%width != 0)         children[i]->west  = children[i-1];     // West
			if (i-width >= 0)         children[i]->south = children[i-width]; // South
			if ((i+1)%width != 0)     children[i]->east  = children[i+1];     // East
		}
		
		// setup dependencies
		for (i=0; i<childCount; ++i) {
			Quad* quad = children[i];
			if (quad->parent->childNW == quad) {
				// NorthWest
				if (quad->north) quad->dependsNS = quad->north->parent;
				if (quad->west)  quad->dependsWE = quad->west->parent;
			} else if (quad->parent->childNE == quad) {
				// NorthEast
				if (quad->north) quad->dependsNS = quad->north->parent;
				if (quad->east)  quad->dependsWE = quad->east->parent;
			} else if (quad->parent->childSE == quad) {
				// SouthEast
				if (quad->south) quad->dependsNS = quad->south->parent;
				if (quad->east)  quad->dependsWE = quad->east->parent;
			} else if (quad->parent->childSW == quad) {
				// SouthWest
				if (quad->south) quad->dependsNS = quad->south->parent;
				if (quad->west)  quad->dependsWE = quad->west->parent;
			} else {
				assert(false);
			}
		}

		// Set children as parents (if they have children)
		if (children[0]->childNW) { // if one quad has a child, they all have children
			parents.clear();
			for (i=0; i<childCount; ++i) {
				parents.push_back( children[i] );
			}
		} else {
			break;
		}
	}


}

Terrain::~Terrain() {

}

void Terrain::generateTerrain3(int size) {
	float baseHeight = 1;
	float frequency,
		  result=0.f,
		  signal,
		  weight=0.f,
		  remainder=0.0f,
		  H=0.25f, // TODO: ???
		  lacunarity=2.0f, // Gap between successive frequencies
		  octaves=8.0f, // # Frequencies of fBM
		  offset=10.0f; // TODO: ???
	auto noise3 = [&](Vertex& point)->float{
		int n = point.x + point.z * 57;
		// return randomFloat(n);
		// n = pow((n<<13), n);
		n = (n<<13) ^ n;
		return ( 1.0 - (float)( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / (float)1073741824.0);  
	};
	sampleHeights = (float*)malloc(sizeof(float)*(size*hmapHeight+size+1));

	float* exponent_array = (float*)malloc(sizeof(float)*(octaves+1));
	frequency = 1.0f;
	for (int i=0; i<=octaves; ++i) {
		exponent_array[i] = pow(frequency, -H);
		frequency *= lacunarity;
	}

	for (uint z=0; z<=size; ++z) {
		for (uint x=0; x<=size; ++x) {
			Vertex point(x,baseHeight,z);
			result = ( noise3(point) + offset ) * exponent_array[0];
			weight = result;

			point.x *= lacunarity;
			point.y *= lacunarity;
			point.z *= lacunarity;

			uint i;
			for(i=1; i<octaves; ++i) {
				if (weight>1) weight=1.0f;
				signal = ( noise3(point) + offset ) * exponent_array[i];
				result += weight * signal;
				weight *= signal;

				point.x *= lacunarity;
				point.y *= lacunarity;
				point.z *= lacunarity;
			}

			remainder = octaves - (int)octaves;
			if (remainder) {
				result += remainder * noise3(point) * exponent_array[i];
			}

			if (result<0) result=0;
			sampleHeights[(uint)(x*hmapHeight+z)] = result;
		}
	}
}

void Terrain::generateTerrain2(int size) {
	
	sampleHeights = (float*)malloc(sizeof(float)*(size*hmapHeight+size+1));
	float baseHeight = 100;
	for (int x=0; x<=size; ++x) {
		for (int z=0; z<=size; ++z) {
			sampleHeights[(uint)(x*hmapHeight+z)] = baseHeight;
		}
	}
		float stepDisplace = pow(2, -0.8);
	float maxDisplace = size/2 * stepDisplace;

	// TODO: set corner points
	float height = maxDisplace * (randf()-0.5f)*2.0f; if(height<0)height=0;
	sampleHeights[0] = height;
	height = maxDisplace * (randf()-0.5f)*2.0f; if(height<0)height=0;
	sampleHeights[(uint)(size*hmapHeight)] = height;
	height = maxDisplace * (randf()-0.5f)*2.0f; if(height<0)height=0;
	sampleHeights[(uint)(size*hmapHeight+size)] = height;
	height = maxDisplace * (randf()-0.5f)*2.0f; if(height<0)height=0;
	sampleHeights[size] = height;

	// TODO: from x=0:N, z=0:N : displace n/c/s
	int step=size;
	float disp;
	while (step>=1) {

		auto randDisp = [&](float height, int max)->float{
			float r = (randf()-0.5f)*2.0f;
			if (height>90) {
				if (rand()%100>90) return height+(r*4*max);
				if (rand()%100>80) return height+(r*3*max);
				return height+(r*max);
			} else {
				if (rand()%100>90) return height+(r*1.25*max);
				return height+(r*max);
			}	
		};

		for (int x=0; x<=size; x+=step) {
			for (int z=0; z<=size; z+=step) {

				// Set Center/South
				if (x!=size) {
					height = (sampleHeights[(uint)(x*hmapHeight+z)] +
							sampleHeights[(uint)((x+step)*hmapHeight+z)])/2.0f;
					height = randDisp(height, maxDisplace);
					if (height<0) height=0;
					sampleHeights[(uint)((x+step/2)*hmapHeight+z)] = height; // Center
					if (z>0) {
						height = (sampleHeights[(uint)(x*hmapHeight+z)] +
								sampleHeights[(uint)((x+step)*hmapHeight+(z-step))])/2.0f;
						height = randDisp(height, maxDisplace);
						if (height<0) height=0;
						sampleHeights[(uint)((x+step/2)*hmapHeight+(z-step/2))] = height; // South
					}
				}

				// Set North
				if (z!=size) {
					height = (sampleHeights[(uint)(x*hmapHeight+z)] +
							sampleHeights[(uint)(x*hmapHeight+(z+step))])/2.0f;
					// disp = (rand()%100>90?maxDisplace*4:maxDisplace/2);
					// height += disp * (randf()-0.5f)*2.0f;
					height = randDisp(height, maxDisplace);
					if (height<0) height=0;
					sampleHeights[(uint)(x*hmapHeight+(z+step/2))] = height; // North
				}
			}
		}

		maxDisplace *= stepDisplace;
		step/=2;
	}

	/*
	int cx=150, cz=150;
	float radius = 40;
	for (int x=0; x<=size; ++x) {
		for (int z=0; z<=size; ++z) {
			float dist = sqrt(x*x+z*z);
			if (dist<radius) {
				float inc = (radius-dist)/4;
				sampleHeights[(uint)(x*hmapHeight+z)] *= inc;
			}
		}
	}
	*/
}

void Terrain::generateTerrain(int size) {

	auto randHeight = [&](float height, float max){
		if (max<0.2) return height;
		float r = ((float)(rand()%((int)(200*max))))/100-max;
		height += r;
		if (height<0) height = 0;
		return height;
	};

	struct SampleQuad { int x; int z; int size; };
	vector<SampleQuad> quads;
	quads.push_back({ 0, 0, size });
	float maxRand = size/5;
	while (size>=1) {
		vector<SampleQuad> children;

		while ( quads.size() > 0 ) {
			int index = rand()%(quads.size());
			SampleQuad quad = quads[index];
		
		// for ( auto quad : quads ) {
			int x = quad.x,
				z = quad.z,
				size = quad.size;

			// TODO: find indices to all 4 points
			SampledVertex nw = samples[(uint)(x*hmapHeight+(z+size))],
						  ne = samples[(uint)((x+size)*hmapHeight+(z+size))],
						  se = samples[(uint)((x+size)*hmapHeight+z)],
						  sw = samples[(uint)(x*hmapHeight+z)];
			ushort neighbours = 0;
			const static ushort NEIGHBOUR_NW_UP    = 1<<0,
				  NEIGHBOUR_NW_LEFT  = 1<<1,
				  NEIGHBOUR_NE_UP    = 1<<2,
				  NEIGHBOUR_NE_RIGHT = 1<<3,
				  NEIGHBOUR_SE_DOWN  = 1<<4,
				  NEIGHBOUR_SE_RIGHT = 1<<5,
				  NEIGHBOUR_SW_DOWN  = 1<<6,
				  NEIGHBOUR_SW_LEFT  = 1<<7,
				  NW_SET             = 1<<8,
				  NE_SET             = 1<<9,
				  SE_SET             = 1<<10,
				  SW_SET             = 1<<11;
			float nw_up, nw_left,
				  ne_up, ne_right,
				  se_down, se_right,
				  sw_down, sw_left,
				  nw_h, ne_h, se_h, sw_h;
			struct SampleNeighbour { int x; int z; float* height; ushort neighbour; }; // NOTE: use int not uint (overflow)
			vector<SampleNeighbour> neighboursToFind;
			if (x-size>=0) {
				neighboursToFind.push_back({ (x-size), z, &sw_left, NEIGHBOUR_SW_LEFT });
				neighboursToFind.push_back({ (x-size), (z+size), &nw_left, NEIGHBOUR_NW_LEFT });
			}
			if (x+2*size<=hmapHeight) {
				neighboursToFind.push_back({ (x+2*size), z, &se_right, NEIGHBOUR_SE_RIGHT });
				neighboursToFind.push_back({ (x+2*size), (z+size), &ne_right, NEIGHBOUR_NE_RIGHT });
			}
			if (z-size>=0) {
				neighboursToFind.push_back({ x, (z-size), &sw_down, NEIGHBOUR_SW_DOWN });
				neighboursToFind.push_back({ (x+size), (z-size), &se_down, NEIGHBOUR_SE_DOWN });
			}
			if (z+2*size<=hmapHeight) {
				neighboursToFind.push_back({ x, (z+2*size), &nw_up, NEIGHBOUR_NW_UP });
				neighboursToFind.push_back({ (x+size), (z+2*size), &ne_up, NEIGHBOUR_NE_UP });
			}
			neighboursToFind.push_back({ x, z, &sw_h, SW_SET });
			neighboursToFind.push_back({ (x+size), z, &se_h, SE_SET });
			neighboursToFind.push_back({ (x+size), (z+size), &ne_h, NE_SET });
			neighboursToFind.push_back({ x, (z+size), &nw_h, NW_SET });


			for ( auto neighbourToFind : neighboursToFind ) {
				SampledVertex sample = samples[(uint)((neighbourToFind.x)*hmapHeight+neighbourToFind.z)];
				if (sample.x==(neighbourToFind.x) && sample.z==(neighbourToFind.z)) {
					(*neighbourToFind.height) = sample.y;
					neighbours |= neighbourToFind.neighbour;
				}
			}


			// TODO: for each vert which isn't yet defined: find its neighbours, interpolate between each
			if (sw.x != x || sw.z != z) { // Southwest
				uchar numHeights = 0;
				float height = 0;
				if (neighbours & NEIGHBOUR_SW_LEFT) { height += sw_left; ++numHeights; } // Left
				if (neighbours & NEIGHBOUR_SW_DOWN) { height += sw_down; ++numHeights; } // Down
				if (neighbours & NW_SET)            { height += nw.y;    ++numHeights; } // Up
				if (neighbours & SE_SET)            { height += se.y;    ++numHeights; } // Right
				if (numHeights == 0) numHeights = 1; // Just set height to 0
				float heightRand = 0;
				if (maxRand>=1) {
					heightRand = rand()%((int)(maxRand)) - maxRand/2;
				}
				height = (height+heightRand)/numHeights;
				// height = randHeight(height/numHeights, maxRand);
				samples[(uint)(x*hmapHeight+z)] = { (float)x, height, (float)z };
				sw = samples[(uint)(x*hmapHeight+z)];
				neighbours |= SW_SET;
			}
			if (ne.x != (x+size) || ne.z != (z+size)) { // Northeast
				uchar numHeights = 0;
				float height = 0;
				if (neighbours & NEIGHBOUR_NE_RIGHT) { height += ne_right; ++numHeights; } // Right
				if (neighbours & NEIGHBOUR_NE_UP)    { height += ne_up;    ++numHeights; } // Up
				if (neighbours & NW_SET)             { height += nw.y;     ++numHeights; } // Left
				if (neighbours & SE_SET)             { height += se.y;     ++numHeights; } // Down
				if (numHeights == 0) numHeights = 1; // Just set height to 0
				float heightRand = 0;
				if (maxRand>=1) {
					heightRand = rand()%((int)(maxRand)) - maxRand/2;
				}
				height = (height+heightRand)/numHeights;
				// height = randHeight(height/numHeights, maxRand);
				samples[(uint)((x+size)*hmapHeight+(z+size))] = { (float)(x+size), height, (float)(z+size) };
				ne = samples[(uint)((x+size)*hmapHeight+(z+size))];
				neighbours |= NE_SET;
			}
			if (nw.x != x || nw.z != (z+size)) { // Northwest
				uchar numHeights = 0;
				float height = 0;
				if (neighbours & NEIGHBOUR_NW_LEFT) { height += nw_left; ++numHeights; } // Left
				if (neighbours & NEIGHBOUR_NW_UP)   { height += nw_up;   ++numHeights; } // Up
				if (neighbours & NE_SET)            { height += ne.y;    ++numHeights; } // Right
				if (neighbours & SW_SET)            { height += sw.y;    ++numHeights; } // Down
				if (numHeights == 0) numHeights = 1; // Just set height to 0
				float heightRand = 0;
				if (maxRand>=1) {
					heightRand = rand()%((int)(maxRand)) - maxRand/2;
				}
				height = (height+heightRand)/numHeights;
				// height = randHeight(height/numHeights, maxRand);
				samples[(uint)(x*hmapHeight+(z+size))] = { (float)x, height, (float)(z+size) };
				nw = samples[(uint)(x*hmapHeight+(z+size))];
				neighbours |= NW_SET;
			}
			if (se.x != (x+size) || se.z != z) { // Southeast
				uchar numHeights = 0;
				float height = 0;
				if (neighbours & NEIGHBOUR_SE_RIGHT) { height += se_right; ++numHeights; } // Right
				if (neighbours & NEIGHBOUR_SE_DOWN)  { height += se_down;  ++numHeights; } // Down
				if (neighbours & SW_SET)             { height += sw.y;     ++numHeights; } // Left
				if (neighbours & NE_SET)             { height += ne.y;     ++numHeights; } // Up
				if (numHeights == 0) numHeights = 1; // Just set height to 0
				float heightRand = 0;
				if (maxRand>=1) {
					heightRand = rand()%((int)(maxRand)) - maxRand/2;
				}
				height = (height+heightRand)/numHeights;
				// height = randHeight(height/numHeights, maxRand);
				samples[(uint)((x+size)*hmapHeight+z)] = { (float)(x+size), height, (float)z };
				se = samples[(uint)((x+size)*hmapHeight+z)];
				neighbours |= SE_SET;
			}



			// TODO: find & interpolate center point
			if (size/2 >= 1) {
				uint midX = x+size/2,
					 midZ = z+size/2;
				float midRand=0;
				if (maxRand>=1) {
					midRand = rand()%((int)(8*maxRand)) - 4*maxRand;
					if (size<=16) midRand /= size;
					// else {
					// 	if (rand()%100>95) {
					// 		midRand *= 15;
					// 	}
					// }
				}
				float height = (midRand + nw.y + ne.y + sw.y + se.y)/4;
				// height = randHeight(height, maxRand*2);
				samples[(uint)(midX*hmapHeight+midZ)] = { (float)midX, height, (float)midZ };


				// TODO: generate each quad
				// generateTerrain(x, z, size/2); // Southwest
				// generateTerrain(x+size/2, z, size/2); // Southeast
				// generateTerrain(x+size/2, z+size/2, size/2); // Northeast
				// generateTerrain(x, z+size/2, size/2); // Northwest
				children.push_back({ x, z, size/2 });
				children.push_back({ x+size/2, z+size/2, size/2 });
				children.push_back({ x, z+size/2, size/2 });
				children.push_back({ x+size/2, z, size/2 });
			}
			// TODO: add childrens


			quads.erase( quads.begin()+index );
		}

		quads.clear();
		for ( auto child : children ) {
			quads.push_back(child);
		}

		size /= 2;
		maxRand /= 1+0.25+(float)(rand()%10)/10;
	}



	/*
	if (size < 1) return;

	// TODO: find indices to all 4 points
	SampledVertex nw = samples[(uint)(x*hmapHeight+(z+size))],
				  ne = samples[(uint)((x+size)*hmapHeight+(z+size))],
				  se = samples[(uint)((x+size)*hmapHeight+z)],
				  sw = samples[(uint)(x*hmapHeight+z)];
	ushort neighbours = 0;
	const static ushort NEIGHBOUR_NW_UP    = 1<<0,
		  			    NEIGHBOUR_NW_LEFT  = 1<<1,
					    NEIGHBOUR_NE_UP    = 1<<2,
					    NEIGHBOUR_NE_RIGHT = 1<<3,
					    NEIGHBOUR_SE_DOWN  = 1<<4,
					    NEIGHBOUR_SE_RIGHT = 1<<5,
					    NEIGHBOUR_SW_DOWN  = 1<<6,
					    NEIGHBOUR_SW_LEFT  = 1<<7,
						NW_SET             = 1<<8,
						NE_SET             = 1<<9,
						SE_SET             = 1<<10,
						SW_SET             = 1<<11;
	float nw_up, nw_left,
		  ne_up, ne_right,
		  se_down, se_right,
		  sw_down, sw_left;
	struct SampleNeighbour { int x; int z; float* height; ushort neighbour; }; // NOTE: use int not uint (overflow)
	vector<SampleNeighbour> neighboursToFind;
	if (x-size>=0) {
		neighboursToFind.push_back({ (x-size), z, &sw_left, NEIGHBOUR_SW_LEFT });
		if (z+size<hmapHeight) neighboursToFind.push_back({ (x-size), (z+size), &nw_left, NEIGHBOUR_NW_LEFT });
	}
	if (x+size<=hmapHeight) {
		neighboursToFind.push_back({ (x+size), z, &se_right, NEIGHBOUR_SE_RIGHT });
		if (z+size<hmapHeight) neighboursToFind.push_back({ (x+size), (z+size), &ne_right, NEIGHBOUR_NE_RIGHT });
	}
	if (z-size>=0) {
		neighboursToFind.push_back({ x, (z-size), &sw_down, NEIGHBOUR_SW_DOWN });
		if (x+size<hmapHeight) neighboursToFind.push_back({ (x+size), (z-size), &se_down, NEIGHBOUR_SE_DOWN });
	}
	if (z+size<=hmapHeight) {
		neighboursToFind.push_back({ x, (z+size), &nw_up, NEIGHBOUR_NW_UP });
		if (x+size<hmapHeight) neighboursToFind.push_back({ x, (z+size), &ne_up, NEIGHBOUR_NE_UP });
	}


	for ( auto neighbourToFind : neighboursToFind ) {
		SampledVertex sample = samples[(uint)((neighbourToFind.x)*hmapHeight+neighbourToFind.z)];
		if (sample.x==(neighbourToFind.x) && sample.z==(neighbourToFind.z)) {
			(*neighbourToFind.height) = sample.y;
			neighbours |= neighbourToFind.neighbour;
		}
	}


	// TODO: for each vert which isn't yet defined: find its neighbours, interpolate between each
	if (nw.x != x || nw.z != (z+size)) { // Northwest
		uchar numHeights = 0;
		float height = 0;
		if (neighbours & NEIGHBOUR_NW_LEFT) { height += nw_left; ++numHeights; } // Left
		if (neighbours & NEIGHBOUR_NW_UP)   { height += nw_up;   ++numHeights; } // Up
		if (neighbours & NE_SET)            { height += ne.y;    ++numHeights; } // Right
		if (neighbours & SW_SET)            { height += sw.y;    ++numHeights; } // Down
		if (numHeights == 0) numHeights = 1; // Just set height to 0
		samples[(uint)(x*hmapHeight+(z+size))] = { x, height/numHeights, z+size };
		nw = samples[(uint)(x*hmapHeight+(z+size))];
		neighbours |= NW_SET;
	}
	if (ne.x != (x+size) || ne.z != (z+size)) { // Northeast
		uchar numHeights = 0;
		float height = 0;
		if (neighbours & NEIGHBOUR_NE_RIGHT) { height += ne_right; ++numHeights; } // Right
		if (neighbours & NEIGHBOUR_NE_UP)    { height += ne_up;    ++numHeights; } // Up
		if (neighbours & NW_SET)             { height += nw.y;     ++numHeights; } // Left
		if (neighbours & SE_SET)             { height += se.y;     ++numHeights; } // Down
		if (numHeights == 0) numHeights = 1; // Just set height to 0
		samples[(uint)((x+size)*hmapHeight+(z+size))] = { x+size, height/numHeights, z+size };
		ne = samples[(uint)((x+size)*hmapHeight+(z+size))];
		neighbours |= NE_SET;
	}
	if (se.x != (x+size) || se.z != z) { // Southeast
		uchar numHeights = 0;
		float height = 0;
		if (neighbours & NEIGHBOUR_SE_RIGHT) { height += se_right; ++numHeights; } // Right
		if (neighbours & NEIGHBOUR_SE_DOWN)  { height += se_down;  ++numHeights; } // Down
		if (neighbours & SW_SET)             { height += sw.y;     ++numHeights; } // Left
		if (neighbours & NE_SET)             { height += ne.y;     ++numHeights; } // Up
		if (numHeights == 0) numHeights = 1; // Just set height to 0
		samples[(uint)((x+size)*hmapHeight+z)] = { x+size, height/numHeights, z };
		se = samples[(uint)((x+size)*hmapHeight+z)];
		neighbours |= SE_SET;
	}
	if (sw.x != x || sw.z != z) { // Southwest
		uchar numHeights = 0;
		float height = 0;
		if (neighbours & NEIGHBOUR_SW_LEFT) { height += sw_left; ++numHeights; } // Left
		if (neighbours & NEIGHBOUR_SW_DOWN) { height += se_down; ++numHeights; } // Down
		if (neighbours & NW_SET)            { height += nw.y;    ++numHeights; } // Up
		if (neighbours & SE_SET)            { height += se.y;    ++numHeights; } // Right
		if (numHeights == 0) numHeights = 1; // Just set height to 0
		samples[(uint)(x*hmapHeight+z)] = { x, height/numHeights, z };
		sw = samples[(uint)(x*hmapHeight+z)];
		neighbours |= SW_SET;
	}


		
	// TODO: find & interpolate center point
	if (size/2 >= 1) {
		uint midX = x+size/2,
			 midZ = z+size/2;
		float height = (nw.y + ne.y + sw.y + se.y)/4;
		height += rand() % 4 - 2;
		if (height<0) height=0;
		samples[(uint)(midX*hmapHeight+midZ)] = { midX, height, midZ };


		// TODO: generate each quad
		generateTerrain(x, z, size/2); // Southwest
		generateTerrain(x+size/2, z, size/2); // Southeast
		generateTerrain(x+size/2, z+size/2, size/2); // Northeast
		generateTerrain(x, z+size/2, size/2); // Northwest
	}
	*/

}

float Terrain::getElevation(float x, float z) {
	return samples[(uint)((int)(x)*hmapHeight+(int)z)].y;
}

void Quad::buildQuad(int x, int z, int width, int depth) {
	assert(width==depth);

	center = {x+(width/2), z+(depth/2)};
	radius = sqrt(width*width + depth*depth);

	uint triangleBufferLengths[9];

	// TODO: If necessary, build children
	// TODO: Build self based off children (if no children, use sampling points; if children, interpolate between 4
	// 										points and keep track of error -- later on can use adaptive style)

	// Stitching policy:
	//
	// 	1->1
	// 	1: North
	// 	2: South
	// 	3: West
	// 	4: East
	//
	// 	1->2
	// 	5: North
	// 	6: South
	// 	7: West
	// 	8: East
	

	// TODO: adaptive quadtree w/ error threshold
	if (width <= min_width) {
		// Use sampling points to build Quad
		//
		int j, k;
		for (j=x+1; j<x+width-1; ++j) {
			for (k=z+1; k<z+depth-1; ++k) {
				
				triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+1)*terrain->hmapHeight+k), (uint)(j*terrain->hmapHeight+(k+1)) }); // sw-se-nw
				triangles.push_back({ (uint)((j+1)*terrain->hmapHeight+k), (uint)((j+1)*terrain->hmapHeight+(k+1)), (uint)(j*terrain->hmapHeight+(k+1)) }); // se-ne-nw
			}
		}
		triangleBufferLengths[0] = triangles.size();

		// Build stitches
		//////////////////////

		float offset = triangleBufferLengths[0];

		// stitching 1->1
		k=z+depth-1;
		triangles.push_back({ (uint)((x+1)*terrain->hmapHeight+k), (uint)((x+1)*terrain->hmapHeight+(k+1)),
				(uint)(x*terrain->hmapHeight+(k+1)) }); // top left corner
		for (j=x+1; j<x+width-1; ++j) { // North stitch
			uint sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+1)*terrain->hmapHeight+k),
				   ne = (uint)((j+1)*terrain->hmapHeight+(k+1)),
				   nw = (uint)(j*terrain->hmapHeight+(k+1));
			triangles.push_back({ sw, se, nw }); // sw-se-nw
			triangles.push_back({ se, ne, nw }); // se-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+1)*terrain->hmapHeight+(k+1)),
				(uint)(j*terrain->hmapHeight+(k+1)) }); // top right corner
		triangleBufferLengths[1] = triangles.size() - offset;
		offset += triangleBufferLengths[1];

		k=z;
		triangles.push_back({ (uint)((x+1)*terrain->hmapHeight+k), (uint)((x+1)*terrain->hmapHeight+(k+1)),
				(uint)(x*terrain->hmapHeight+k) }); // bottom left corner
		for (j=x+1; j<x+width-1; ++j) { // South stitch
			uint sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+1)*terrain->hmapHeight+k),
				   ne = (uint)((j+1)*terrain->hmapHeight+(k+1)),
				   nw = (uint)(j*terrain->hmapHeight+(k+1));
			triangles.push_back({ sw, se, nw }); // sw-se-nw
			triangles.push_back({ se, ne, nw }); // se-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+1)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+1)*terrain->hmapHeight+k) }); // bottom right corner
		triangleBufferLengths[2] = triangles.size() - offset;
		offset += triangleBufferLengths[2];

		j=x;
		triangles.push_back({ (uint)((j+1)*terrain->hmapHeight+(z+1)), (uint)(j*terrain->hmapHeight+(z+1)),
				(uint)(j*terrain->hmapHeight+z) }); // bottom left corner
		for (k=z+1; k<z+depth-1; ++k) { // West stitch
			uint sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+1)*terrain->hmapHeight+k),
				   ne = (uint)((j+1)*terrain->hmapHeight+(k+1)),
				   nw = (uint)(j*terrain->hmapHeight+(k+1));
			triangles.push_back({ sw, se, nw }); // sw-se-nw
			triangles.push_back({ se, ne, nw }); // se-ne-nw
		}
		triangles.push_back({ (uint)((j+1)*terrain->hmapHeight+k), (uint)(j*terrain->hmapHeight+(k+1)),
				(uint)(j*terrain->hmapHeight+k) }); // top left corner
		triangleBufferLengths[3] = triangles.size() - offset;
		offset += triangleBufferLengths[3];

		j=x+width-1;
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(z+1)), (uint)((j+1)*terrain->hmapHeight+z),
				(uint)((j+1)*terrain->hmapHeight+(z+1)) }); // bottom right corner
		for (k=z+1; k<z+depth-1; ++k) { // East stitch
			uint sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+1)*terrain->hmapHeight+k),
				   ne = (uint)((j+1)*terrain->hmapHeight+(k+1)),
				   nw = (uint)(j*terrain->hmapHeight+(k+1));
			triangles.push_back({ sw, se, nw }); // sw-se-nw
			triangles.push_back({ se, ne, nw }); // se-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+1)*terrain->hmapHeight+k),
				(uint)((j+1)*terrain->hmapHeight+(k+1)) }); // top right corner
		triangleBufferLengths[4] = triangles.size() - offset;
		offset += triangleBufferLengths[4];


		// Stitching 1->2
		k=z+depth-1;
		triangles.push_back({ (uint)(x*terrain->hmapHeight+(k+1)), (uint)((x+1)*terrain->hmapHeight+k),
				(uint)((x+2)*terrain->hmapHeight+(k+1)) }); // top left corner
		triangles.push_back({ (uint)((x+1)*terrain->hmapHeight+k), (uint)((x+2)*terrain->hmapHeight+k),
				(uint)((x+2)*terrain->hmapHeight+(k+1)) });
		for (j=x+2; j<x+width-2; j+=2) { // North stitch
			uint s  = (uint)((j+1)*terrain->hmapHeight+k),
				   sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+2)*terrain->hmapHeight+k),
				   ne = (uint)((j+2)*terrain->hmapHeight+(k+1)),
				   nw = (uint)(j*terrain->hmapHeight+(k+1));
			triangles.push_back({ s, se, ne }); // s-se-ne
			triangles.push_back({ s, ne, nw }); // s-ne-nw
			triangles.push_back({ s, nw, sw }); // s-nw-sw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+1)), (uint)((j+1)*terrain->hmapHeight+k),
				(uint)((j+2)*terrain->hmapHeight+(k+1)) }); // top right corner
		triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+1)*terrain->hmapHeight+k),
				(uint)(j*terrain->hmapHeight+(k+1)) });
		triangleBufferLengths[5] = triangles.size() - offset;
		offset += triangleBufferLengths[5];

		k=z;
		triangles.push_back({ (uint)((x+1)*terrain->hmapHeight+(k+1)), (uint)(x*terrain->hmapHeight+k),
				(uint)((x+2)*terrain->hmapHeight+k) }); // bottom left corner
		triangles.push_back({ (uint)((x+1)*terrain->hmapHeight+(k+1)), (uint)((x+2)*terrain->hmapHeight+k),
				(uint)((x+2)*terrain->hmapHeight+(k+1)) });
		for (j=x+2; j<x+width-2; j+=2) { // South stitch
			uint n  = (uint)((j+1)*terrain->hmapHeight+(k+1)),
				   sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+2)*terrain->hmapHeight+k),
				   ne = (uint)((j+2)*terrain->hmapHeight+(k+1)),
				   nw = (uint)(j*terrain->hmapHeight+(k+1));
			triangles.push_back({ sw, se, n }); // sw-se-n
			triangles.push_back({ sw, n, nw }); // sw-n-nw
			triangles.push_back({ se, ne, n }); // se-ne-n
		}
		triangles.push_back({ (uint)((j+1)*terrain->hmapHeight+(k+1)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+2)*terrain->hmapHeight+k) }); // bottom right corner
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+1)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+1)*terrain->hmapHeight+(k+1)) });
		triangleBufferLengths[6] = triangles.size() - offset;
		offset += triangleBufferLengths[6];

		j=x;
		triangles.push_back({ (uint)(j*terrain->hmapHeight+z), (uint)((j+1)*terrain->hmapHeight+(z+1)),
				(uint)(j*terrain->hmapHeight+(z+2)) }); // bottom left corner
		triangles.push_back({ (uint)((j+1)*terrain->hmapHeight+(z+1)), (uint)((j+1)*terrain->hmapHeight+(z+2)),
				(uint)(j*terrain->hmapHeight+(z+2)) });
		for (k=z+2; k<z+depth-2; k+=2) { // West stitch
			uint e  = (uint)((j+1)*terrain->hmapHeight+(k+1)),
				   sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+1)*terrain->hmapHeight+k),
				   ne = (uint)((j+1)*terrain->hmapHeight+(k+2)),
				   nw = (uint)(j*terrain->hmapHeight+(k+2));
			triangles.push_back({ sw, e, nw }); // sw-e-nw
			triangles.push_back({ sw, se, e }); // sw-se-e
			triangles.push_back({ e, ne, nw }); // e-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+1)*terrain->hmapHeight+(k+1)),
				(uint)(j*terrain->hmapHeight+(k+2)) }); // top left corner
		triangles.push_back({ (uint)((j+1)*terrain->hmapHeight+(k+1)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+1)*terrain->hmapHeight+k) });
		triangleBufferLengths[7] = triangles.size() - offset;
		offset += triangleBufferLengths[7];

		j=x+width-1;
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(z+1)), (uint)((j+1)*terrain->hmapHeight+z),
				(uint)((j+1)*terrain->hmapHeight+(z+2)) }); // bottom right corner
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(z+1)), (uint)((j+1)*terrain->hmapHeight+(z+2)),
				(uint)(j*terrain->hmapHeight+(z+2)) });
		for (k=z+2; k<z+depth-2; k+=2) { // East stitch
			uint w  = (uint)(j*terrain->hmapHeight+(k+1)),
				   sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+1)*terrain->hmapHeight+k),
				   ne = (uint)((j+1)*terrain->hmapHeight+(k+2)),
				   nw = (uint)(j*terrain->hmapHeight+(k+2));
			triangles.push_back({ se, ne, w }); // se-ne-w
			triangles.push_back({ sw, se, w }); // sw-se-w
			triangles.push_back({ w, ne, nw }); // w-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+1)), (uint)((j+1)*terrain->hmapHeight+k),
				(uint)((j+1)*terrain->hmapHeight+(k+2)) }); // top right corner
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+1)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+1)*terrain->hmapHeight+k) });
		triangleBufferLengths[8] = triangles.size() - offset;
		offset += triangleBufferLengths[8];
		
		error = 0;
	} else { // TODO: only build if lod < max_lod's
		// Build children and use children's points to build Quad (NOTE: don't bother if LOD too low)
		childNW = new Quad(terrain, this);
		childNE = new Quad(terrain, this);
		childSE = new Quad(terrain, this);
		childSW = new Quad(terrain, this);


		childNW->buildQuad(x, z+(depth/2), width/2, depth/2);
		childNE->buildQuad(x+(width/2), z+(depth/2), width/2, depth/2);
		childSE->buildQuad(x+(width/2), z, width/2, depth/2);
		childSW->buildQuad(x, z, width/2, depth/2);

		// Build Quad by interpolating from children points
		float inc = width/min_width;
		int j, k;
		for (j=x; j<x+width-inc; j+=inc) {
			for (k=z; k<z+depth-inc; k+=inc) {
				// TODO: interpolate from children instead
				triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+inc)*terrain->hmapHeight+k), (uint)(j*terrain->hmapHeight+(k+inc)) }); // sw-se-nw
				triangles.push_back({ (uint)((j+inc)*terrain->hmapHeight+k), (uint)((j+inc)*terrain->hmapHeight+(k+inc)), (uint)(j*terrain->hmapHeight+(k+inc)) }); // se-ne-nw
			}
		}
		triangleBufferLengths[0] = triangles.size();

		auto interpolate = [&](float ix, float iz)->float {
			// Find neighbour values (from next LOD down)
			float step   = inc/2,
				  height = 0,
				  numPoints = 0;
			if (ix-step>=0) { height += terrain->vertices[(uint)((ix-step)*terrain->hmapHeight+iz)].y; ++numPoints; }
			// TODO: all steps
			// TODO: center point
			// TODO: interpolate
			// TODO: create point, return {error, interpolatedPoint}
			return 0.0f;
		};

		// Build stitches
		//////////////////////

		float offset = triangleBufferLengths[0];

		// stitching 1->1
		k=z+depth-inc;
		triangles.push_back({ (uint)((x+inc)*terrain->hmapHeight+k), (uint)((x+inc)*terrain->hmapHeight+(k+inc)),
				(uint)(x*terrain->hmapHeight+(k+inc)) }); // top left corner
		for (j=x+inc; j<x+width-inc; j+=inc) { // North stitch
			uint sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+inc)*terrain->hmapHeight+k),
				   ne = (uint)((j+inc)*terrain->hmapHeight+(k+inc)),
				   nw = (uint)(j*terrain->hmapHeight+(k+inc));
			triangles.push_back({ sw, se, nw }); // sw-se-nw
			triangles.push_back({ se, ne, nw }); // se-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+inc)*terrain->hmapHeight+(k+inc)),
				(uint)(j*terrain->hmapHeight+(k+inc)) }); // top right corner
		triangleBufferLengths[1] = triangles.size() - offset;
		offset += triangleBufferLengths[1];

		k=z;
		triangles.push_back({ (uint)((x+inc)*terrain->hmapHeight+k), (uint)((x+inc)*terrain->hmapHeight+(k+inc)),
				(uint)(x*terrain->hmapHeight+k) }); // bottom left corner
		for (j=x+inc; j<x+width-inc; j+=inc) { // South stitch
			uint sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+inc)*terrain->hmapHeight+k),
				   ne = (uint)((j+inc)*terrain->hmapHeight+(k+inc)),
				   nw = (uint)(j*terrain->hmapHeight+(k+inc));
			triangles.push_back({ sw, se, nw }); // sw-se-nw
			triangles.push_back({ se, ne, nw }); // se-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+inc)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+inc)*terrain->hmapHeight+k) }); // bottom right corner
		triangleBufferLengths[2] = triangles.size() - offset;
		offset += triangleBufferLengths[2];

		j=x;
		triangles.push_back({ (uint)((j+inc)*terrain->hmapHeight+(z+inc)), (uint)(j*terrain->hmapHeight+(z+inc)),
				(uint)(j*terrain->hmapHeight+z) }); // bottom left corner
		for (k=z+inc; k<z+depth-inc; k+=inc) { // West stitch
			uint sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+inc)*terrain->hmapHeight+k),
				   ne = (uint)((j+inc)*terrain->hmapHeight+(k+inc)),
				   nw = (uint)(j*terrain->hmapHeight+(k+inc));
			triangles.push_back({ sw, se, nw }); // sw-se-nw
			triangles.push_back({ se, ne, nw }); // se-ne-nw
		}
		triangles.push_back({ (uint)((j+inc)*terrain->hmapHeight+k), (uint)(j*terrain->hmapHeight+(k+inc)),
				(uint)(j*terrain->hmapHeight+k) }); // top left corner
		triangleBufferLengths[3] = triangles.size() - offset;
		offset += triangleBufferLengths[3];

		j=x+width-inc;
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(z+inc)), (uint)((j+inc)*terrain->hmapHeight+z),
				(uint)((j+inc)*terrain->hmapHeight+(z+inc)) }); // bottom right corner
		for (k=z+inc; k<z+depth-inc; k+=inc) { // East stitch
			uint sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+inc)*terrain->hmapHeight+k),
				   ne = (uint)((j+inc)*terrain->hmapHeight+(k+inc)),
				   nw = (uint)(j*terrain->hmapHeight+(k+inc));
			triangles.push_back({ sw, se, nw }); // sw-se-nw
			triangles.push_back({ se, ne, nw }); // se-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+inc)*terrain->hmapHeight+k),
				(uint)((j+inc)*terrain->hmapHeight+(k+inc)) }); // top right corner
		triangleBufferLengths[4] = triangles.size() - offset;
		offset += triangleBufferLengths[4];


		// Stitching 1->2
		k=z+depth-inc;
		triangles.push_back({ (uint)(x*terrain->hmapHeight+(k+inc)), (uint)((x+inc)*terrain->hmapHeight+k),
				(uint)((x+2*inc)*terrain->hmapHeight+(k+inc)) }); // top left corner
		triangles.push_back({ (uint)((x+inc)*terrain->hmapHeight+k), (uint)((x+2*inc)*terrain->hmapHeight+k),
				(uint)((x+2*inc)*terrain->hmapHeight+(k+inc)) });
		for (j=x+2*inc; j<x+width-2*inc; j+=2*inc) { // North stitch
			uint s  = (uint)((j+inc)*terrain->hmapHeight+k),
				   sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+2*inc)*terrain->hmapHeight+k),
				   ne = (uint)((j+2*inc)*terrain->hmapHeight+(k+inc)),
				   nw = (uint)(j*terrain->hmapHeight+(k+inc));
			triangles.push_back({ s, se, ne }); // s-se-ne
			triangles.push_back({ s, ne, nw }); // s-ne-nw
			triangles.push_back({ s, nw, sw }); // s-nw-sw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+inc)), (uint)((j+inc)*terrain->hmapHeight+k),
				(uint)((j+2*inc)*terrain->hmapHeight+(k+inc)) }); // top right corner
		triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+inc)*terrain->hmapHeight+k),
				(uint)(j*terrain->hmapHeight+(k+inc)) });
		triangleBufferLengths[5] = triangles.size() - offset;
		offset += triangleBufferLengths[5];

		k=z;
		triangles.push_back({ (uint)((x+inc)*terrain->hmapHeight+(k+inc)), (uint)(x*terrain->hmapHeight+k),
				(uint)((x+2*inc)*terrain->hmapHeight+k) }); // bottom left corner
		triangles.push_back({ (uint)((x+inc)*terrain->hmapHeight+(k+inc)), (uint)((x+2*inc)*terrain->hmapHeight+k),
				(uint)((x+2*inc)*terrain->hmapHeight+(k+inc)) });
		for (j=x+2*inc; j<x+width-2*inc; j+=2*inc) { // South stitch
			uint n  = (uint)((j+inc)*terrain->hmapHeight+(k+inc)),
				   sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+2*inc)*terrain->hmapHeight+k),
				   ne = (uint)((j+2*inc)*terrain->hmapHeight+(k+inc)),
				   nw = (uint)(j*terrain->hmapHeight+(k+inc));
			triangles.push_back({ sw, se, n }); // sw-se-n
			triangles.push_back({ sw, n, nw }); // sw-n-nw
			triangles.push_back({ se, ne, n }); // se-ne-n
		}
		triangles.push_back({ (uint)((j+inc)*terrain->hmapHeight+(k+inc)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+2*inc)*terrain->hmapHeight+k) }); // bottom right corner
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+inc)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+inc)*terrain->hmapHeight+(k+inc)) });
		triangleBufferLengths[6] = triangles.size() - offset;
		offset += triangleBufferLengths[6];

		j=x;
		triangles.push_back({ (uint)(j*terrain->hmapHeight+z), (uint)((j+inc)*terrain->hmapHeight+(z+inc)),
				(uint)(j*terrain->hmapHeight+(z+2*inc)) }); // bottom left corner
		triangles.push_back({ (uint)((j+inc)*terrain->hmapHeight+(z+inc)), (uint)((j+inc)*terrain->hmapHeight+(z+2*inc)),
				(uint)(j*terrain->hmapHeight+(z+2*inc)) });
		for (k=z+2*inc; k<z+depth-2*inc; k+=2*inc) { // West stitch
			uint e  = (uint)((j+inc)*terrain->hmapHeight+(k+inc)),
				   sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+inc)*terrain->hmapHeight+k),
				   ne = (uint)((j+inc)*terrain->hmapHeight+(k+2*inc)),
				   nw = (uint)(j*terrain->hmapHeight+(k+2*inc));
			triangles.push_back({ sw, e, nw }); // sw-e-nw
			triangles.push_back({ sw, se, e }); // sw-se-e
			triangles.push_back({ e, ne, nw }); // e-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+k), (uint)((j+inc)*terrain->hmapHeight+(k+inc)),
				(uint)(j*terrain->hmapHeight+(k+2*inc)) }); // top left corner
		triangles.push_back({ (uint)((j+inc)*terrain->hmapHeight+(k+inc)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+inc)*terrain->hmapHeight+k) });
		triangleBufferLengths[7] = triangles.size() - offset;
		offset += triangleBufferLengths[7];

		j=x+width-inc;
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(z+inc)), (uint)((j+inc)*terrain->hmapHeight+z),
				(uint)((j+inc)*terrain->hmapHeight+(z+2*inc)) }); // bottom right corner
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(z+inc)), (uint)((j+inc)*terrain->hmapHeight+(z+2*inc)),
				(uint)(j*terrain->hmapHeight+(z+2*inc)) });
		for (k=z+2*inc; k<z+depth-2*inc; k+=2*inc) { // East stitch
			uint w  = (uint)(j*terrain->hmapHeight+(k+inc)),
				   sw = (uint)(j*terrain->hmapHeight+k),
				   se = (uint)((j+inc)*terrain->hmapHeight+k),
				   ne = (uint)((j+inc)*terrain->hmapHeight+(k+2*inc)),
				   nw = (uint)(j*terrain->hmapHeight+(k+2*inc));
			triangles.push_back({ se, ne, w }); // se-ne-w
			triangles.push_back({ sw, se, w }); // sw-se-w
			triangles.push_back({ w, ne, nw }); // w-ne-nw
		}
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+inc)), (uint)((j+inc)*terrain->hmapHeight+k),
				(uint)((j+inc)*terrain->hmapHeight+(k+2*inc)) }); // top right corner
		triangles.push_back({ (uint)(j*terrain->hmapHeight+(k+inc)), (uint)(j*terrain->hmapHeight+k),
				(uint)((j+inc)*terrain->hmapHeight+k) });
		triangleBufferLengths[8] = triangles.size() - offset;
		offset += triangleBufferLengths[8];
		error = 0; // TODO: set error from interpolated points
	}

	// construct vao, ibo from triangles
	glUseProgram(terrain->gl);
	glGenBuffers( 1, &terrain->vbo );
	glBindBuffer( GL_ARRAY_BUFFER, terrain->vbo );
	glBufferData( GL_ARRAY_BUFFER, ( terrain->vertices.size() ) * sizeof(Vertex), terrain->vertices.data(), GL_DYNAMIC_DRAW ); // TODO: dynamic drawing? multiple VBO's per LOD & page?


	// enable display list
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	// setup buffer object
	glBindBuffer( GL_ARRAY_BUFFER, terrain->vbo );

	// load data into shader
	GLint glVertex = glGetAttribLocation( terrain->gl, "in_Position" );
	glEnableVertexAttribArray( glVertex );
	glVertexAttribPointer( glVertex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0 );

	GLint glNormal = glGetAttribLocation( terrain->gl, "in_Normal" );
	glEnableVertexAttribArray( glNormal );
	glVertexAttribPointer( glNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)) );

	GLint glSlope = glGetAttribLocation( terrain->gl, "in_Slope" );
	glEnableVertexAttribArray( glSlope );
	glVertexAttribPointer( glSlope, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)) );


	// Generate a buffer for the indices
	iboList[0] = { 0, triangleBufferLengths[0], 0 };
	glGenBuffers(1, &iboList[0].ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboList[0].ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangleBufferLengths[0] * sizeof(iTriangle), triangles.data(), GL_DYNAMIC_DRAW);

		uint offset = triangleBufferLengths[0];

		for (int i=1; i<=8; ++i) {
			iboList[i] = { offset, triangleBufferLengths[i], 0 };
			offset += triangleBufferLengths[i];
			glGenBuffers(1, &iboList[i].ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboList[i].ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangleBufferLengths[i] * sizeof(iTriangle), triangles.data() + iboList[i].offset, GL_DYNAMIC_DRAW);
		}
	glActiveTexture( GL_TEXTURE3 );
	glEnable( GL_TEXTURE_2D_ARRAY );
	glBindTexture( GL_TEXTURE_2D_ARRAY, terrain->glTexture );
	glUniform1i( glGetUniformLocation( terrain->gl, "Tex" ), 3 ); // GL_TEXTURE2 vs. 2 ?
	glActiveTexture( GL_TEXTURE4 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, terrain->glTextureDetail );
	glUniform1i( glGetUniformLocation( terrain->gl, "TexDetail" ), 4 ); // GL_TEXTURE2 vs. 2 ?
	glActiveTexture( GL_TEXTURE5 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, terrain->glTextureBumpy );
	glUniform1i( glGetUniformLocation( terrain->gl, "TexBump" ), 5 ); // GL_TEXTURE2 vs. 2 ?


	// cleanup
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray( 0 );
}

void Quad::enableRender() {
	enabled = true;
	if (parent && !parent->enabled) parent->enableRender();
	if (dependsNS && !dependsNS->enabled) dependsNS->enableRender();
	if (dependsWE && !dependsWE->enabled) dependsWE->enableRender();

	if (childNW) {
		float x = camera.position.x/terrain->scaleDepth,
			  y = camera.position.y,
			  z = camera.position.z/terrain->scaleDepth;
		// Determine children enabling
		Vertex camPos(-x, y, -z);
		Vertex distance = Vertex(childNW->center.first, y, childNW->center.second) - camPos;
		if (sqrt(distance.dot(distance)) < childNW->radius) {
			// Inside radius of child
			childNW->enableRender();
		}

		distance = Vertex(childNE->center.first, y, childNE->center.second) - camPos;
		if (sqrt(distance.dot(distance)) < childNE->radius) {
			// Inside radius of child
			childNE->enableRender();
		}

		distance = Vertex(childSE->center.first, y, childSE->center.second) - camPos;
		if (sqrt(distance.dot(distance)) < childSE->radius) {
			// Inside radius of child
			childSE->enableRender();
		}

		distance = Vertex(childSW->center.first, y, childSW->center.second) - camPos;
		if (sqrt(distance.dot(distance)) < childSW->radius) {
			// Inside radius of child
			childSW->enableRender();
		}
	}

	// Enable siblings
	if (parent) {
		if (parent->childNW != this && !parent->childNW->enabled) parent->childNW->enableRender();
		if (parent->childNE != this && !parent->childNE->enabled) parent->childNE->enableRender();
		if (parent->childSE != this && !parent->childSE->enabled) parent->childSE->enableRender();
		if (parent->childSW != this && !parent->childSW->enabled) parent->childSW->enableRender();
	}
}

void Quad::clearRender() {
	enabled = false;
	if (childNW) {
		if (childNW->enabled) childNW->clearRender();
		if (childNE->enabled) childNE->clearRender();
		if (childSE->enabled) childSE->clearRender();
		if (childSW->enabled) childSW->clearRender();
	}
}

void Quad::render() {

	bool renderChildren = childNW && (childNW->enabled || childNE->enabled || childSE->enabled || childSW->enabled);
	if (renderChildren) {
		// Render chilren
		childNW->render();
		childNE->render();
		childSE->render();
		childSW->render();
	} else {

		float r=0.2f, g=0.3f, b=0.0f;

		if ( triangles.size() == 0 ) return;

		glUseProgram(terrain->gl);
		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboList[0].ibo);

		GLint glMVP = glGetUniformLocation( terrain->gl, "MVP" );

		glm::mat4 mvp = camera.perspectiveView;
		mvp = glm::transpose(mvp);
		glUniformMatrix4fv( glMVP, 1, GL_FALSE, glm::value_ptr(mvp) );

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUniform3f( glGetUniformLocation( terrain->gl, "in_color" ), 0, 0, 0 );
		glDrawElements( GL_TRIANGLES, (iboList[0].length)*3, GL_UNSIGNED_INT, (void*)0 );

		// TODO: Do we have to do this here? Can't we do this in the construction?
		glActiveTexture( GL_TEXTURE3 );
		glBindTexture( GL_TEXTURE_2D_ARRAY, terrain->glTexture );
		glActiveTexture( GL_TEXTURE4 );
		glBindTexture( GL_TEXTURE_2D, terrain->glTextureDetail );
		glActiveTexture( GL_TEXTURE5 );
		glBindTexture( GL_TEXTURE_2D, terrain->glTextureBumpy );
		glActiveTexture( GL_TEXTURE7 );
		glBindTexture( GL_TEXTURE_2D, terrain->glTextureGrass );
		glActiveTexture( GL_TEXTURE6 );
		glBindTexture( GL_TEXTURE_2D, terrain->glTextureSnow );

		if (terrain->drawLines) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glUniform3f( glGetUniformLocation( terrain->gl, "in_color" ), 1.0f, 1.0f, 1.0f );
			glDrawElements( GL_TRIANGLES, (iboList[0].length)*3, GL_UNSIGNED_INT, (void*)0 );
		}

			uchar ibo_north = ((!north || north->enabled)?1:5),
				  ibo_south = ((!south || south->enabled)?2:6),
				  ibo_west  = ((!west  || west->enabled)? 3:7),
				  ibo_east  = ((!east  || east->enabled)? 4:8);
			vector<uchar> ibos;
			ibos.push_back(ibo_north);
			ibos.push_back(ibo_south);
			ibos.push_back(ibo_west);
			ibos.push_back(ibo_east);
			for (auto ibo_dir : ibos) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboList[ibo_dir].ibo);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glUniform3f( glGetUniformLocation( terrain->gl, "in_color" ), 0, 0, 0 );
				glDrawElements( GL_TRIANGLES, (iboList[ibo_dir].length)*3, GL_UNSIGNED_INT, (void*)0 );

				if (terrain->drawLines) {
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glUniform3f( glGetUniformLocation( terrain->gl, "in_color" ), 1.0f, 1.0f, 1.0f );
					glDrawElements( GL_TRIANGLES, (iboList[ibo_dir].length)*3, GL_UNSIGNED_INT, (void*)0 );
				}
			}

		glBindVertexArray(0);
	}

}

Terrain::CircularInterpolation Terrain::circularSelection(float x, float z, float radius) {
	float min = 0;
	float max = 0;
	float y = 0;
	float total = 0;
	bool foundSamples = false;
	// if (radius<1) radius=1;
	// radius=1;
	return { samples[(uint)((int)(x)*hmapHeight+(int)z)].y, samples[(uint)((int)(x)*hmapHeight+(int)z)].y, samples[(uint)((int)(x)*hmapHeight+(int)z)].y, true };
	// if (radius>=1) {
	/*
		for ( auto sample : samples ) {
			float Dx = x - sample.x,
				  Dy = z - sample.z,
				  distance = Dx*Dx + Dy*Dy;
			if (distance < radius*radius) {
				// vertex inside of circle area
				float interpolation = 1 - sqrt(distance)/radius; // (0,1]
				float ySample = sample.y;
				if (ySample < min || !foundSamples) min = ySample;
				if (ySample > max) max = ySample;
				y +=  interpolation * ySample;
				total += interpolation;
				foundSamples = true;
			}
		}
		*/
	// }
	// y = samples[(int)(x)*hmapHeight+(int)z].y;
	// if (!foundSamples) return circularSelection(x,z,radius*2);
	return { y/total, min, max, foundSamples };
}

void Terrain::construct() {

	// load texture image
	Texture* texture = Texture::loadTexture( "data/textures/terrain.png" );
	Texture* atlasSnow = Texture::loadTexture( "data/textures/snow.jpg" );
	Texture* atlasFault = Texture::loadTexture( "data/textures/faultzone.jpg" );
	Texture* atlasGrass = Texture::loadTexture( "data/textures/grass.jpg" );
	Texture* atlasBison = Texture::loadTexture( "data/textures/justaddbison.jpg" );
	Texture* atlasRocky = Texture::loadTexture( "data/textures/rocky.jpg" );
	Texture* atlasVolcano = Texture::loadTexture( "data/textures/slumberingvolcano.jpg" );
	Texture* atlasCave = Texture::loadTexture( "data/textures/deepcave.jpg" );
	Texture* atlasBarren = Texture::loadTexture( "data/textures/barrenreds.jpg" );
	Texture* atlasCanyon = Texture::loadTexture( "data/textures/ageofcanyon.jpg" );
	Texture* atlasFrost = Texture::loadTexture( "data/textures/magnifiedfrost.jpg" );
	Texture* textureGrass = Texture::loadTexture( "data/textures/grass.jpg" );
	Texture* textureSnow = Texture::loadTexture( "data/textures/snow.jpg" );
	Texture* textureBump = Texture::loadTexture( "data/textures/rockybumpy.png" );
	Texture* textureDetail = Texture::loadTexture( "data/textures/gravel.jpg" );
	glGenTextures(1, &glTexture);
	glGenTextures(1, &glTextureSnow);
	glGenTextures(1, &glTextureGrass);
	glGenTextures(1, &glTextureBumpy);
	glGenTextures(1, &glTextureDetail);

	// copy file to opengl
	glUseProgram(gl);
	glActiveTexture( GL_TEXTURE3 ); // GL_TEXTURE0 for different gl program?
	glEnable( GL_TEXTURE_2D_ARRAY );
	glBindTexture( GL_TEXTURE_2D_ARRAY, glTexture );
	// glTexImage2D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, texture->width, texture->height, 0,
	// 		GL_RGB, GL_UNSIGNED_BYTE, texture->imageData );
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, atlasSnow->width, atlasSnow->height, 10, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	// copy over images
	vector<Texture*> atlas;
	atlas.push_back( atlasSnow );
	atlas.push_back( atlasFault );
	atlas.push_back( atlasGrass );
	atlas.push_back( atlasBison );
	atlas.push_back( atlasRocky );
	atlas.push_back( atlasVolcano );
	atlas.push_back( atlasCave );
	atlas.push_back( atlasBarren );
	atlas.push_back( atlasCanyon );
	atlas.push_back( atlasFrost );
	int i=0;
	for (auto tex : atlas) {
		glTexSubImage3D( GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
				tex->width, tex->height, 1, GL_RGB, GL_UNSIGNED_BYTE, tex->imageData );
		/*
void glTexSubImage3D(	GLenum target,
 	GLint level,
 	GLint xoffset,
 	GLint yoffset,
 	GLint zoffset,
 	GLsizei width,
 	GLsizei height,
 	GLsizei depth,
 	GLenum format,
 	GLenum type,
 	const GLvoid * data);
		 */
		++i;
	}

	glUniform1i( glGetUniformLocation( gl, "Tex" ), 3 ); // GL_TEXTURE2 vs. 2 ?

	// repeat texture
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT );

	// mipmapping
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4); // pick mipmap level 7 or lower
	// glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );

	glActiveTexture( GL_TEXTURE4 ); // GL_TEXTURE0 for different gl program?
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, glTextureDetail );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureDetail->width, textureDetail->height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, textureDetail->imageData );
	glUniform1i( glGetUniformLocation( gl, "TexDetail" ), 4 ); // GL_TEXTURE2 vs. 2 ?

	// repeat texture
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );

	// mipmapping
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D );



	glActiveTexture( GL_TEXTURE5 ); // GL_TEXTURE0 for different gl program?
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, glTextureBumpy );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureBump->width, textureBump->height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, textureBump->imageData );
	glUniform1i( glGetUniformLocation( gl, "TexBump" ), 5 ); // GL_TEXTURE2 vs. 2 ?

	// repeat texture
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );

	// mipmapping
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D );

	glActiveTexture( GL_TEXTURE6 ); // GL_TEXTURE0 for different gl program?
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, glTextureSnow );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureSnow->width, textureSnow->height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, textureSnow->imageData );
	glUniform1i( glGetUniformLocation( gl, "TexSnow" ), 6 ); // GL_TEXTURE2 vs. 2 ?

	// repeat texture
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );

	// mipmapping
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D );


	glActiveTexture( GL_TEXTURE7 ); // GL_TEXTURE0 for different gl program?
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, glTextureGrass );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureGrass->width, textureGrass->height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, textureGrass->imageData );
	glUniform1i( glGetUniformLocation( gl, "TexGrass" ), 7 ); // GL_TEXTURE2 vs. 2 ?

	// repeat texture
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );

	// mipmapping
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D );


	// cleanup
	glActiveTexture(0);
	Log( str( format("Loaded texture grass.jpg") ) );
}

void Terrain::render() {

	// Figure out render tree
	headQuad->clearRender();
	headQuad->enableRender();

	headQuad->render();
}

