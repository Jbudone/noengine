
No Engine
===============

JB's engine collection. Until the engine and core is in a working and usable state, this will be
known as No Engine (ie. incomplete). The various components will be completely scalable with the
possibility of multiple implementations which can be choosen at compile-time.


Requirements
----------

	+ freeglut3-dev
	+ boost 1.5.3+
	+ freetype 2.5.0+


Terrain
----------

There are currently two procedural terrain implementations. A heighmap approach, and a polysoup
based approach. The polysoup implementation was created as a way to allow a completely dynamic
landscape, and solves issues such as digging tunnels, caverns, cliffs and overhangs, etc. The
heightmap approach is a standard implementation for games because of its easy, straightforward and
performance friendly concept.

	+ Heightmap Terrain

![heightmap](/screenshots/ubXqwr4.png) ![heightmap](/screenshots/hmap-short.png)
	
		The terrain is split into Quads to allow varying levels of LOD, and only rendering Quads in
		view. To avoid cross-LOD holes between Quads, a stitch is given for each LOD to each corner
		and edge. This way a Quad is rendered and its edges are rendered according to its neighbour
		LOD level. I've implemented a perlin noise generator to procedurally build a heightmap on
		startup.

![no-stitch](/screenshots/YGylbMq.png) ![no-stitch](/screenshots/JPOjvVo.png)

		Notice the holes between chunks renderedf at different levels of LOD. This is an artifact of
		not stitching Quads together using edge strips.

![popping](/screenshots/popping.gif)

		One problem is the "popping" effect from Quads swapping between LOD levels. This drastic
		change in geometry is particularly noticeable if an error threshold isn't taken into account
		and if the Quad is too close to the viewer. 

![normals](/screenshots/3btgCqK.png) ![normals](/screenshots/A9yFidG.png)

		Normals are used to give the terrain a natural look.

![textures](/screenshots/hmap-long.png) ![textures](/screenshots/hmap-long3.png)

		I'm using a variety of textures in order to procedurally generate textures for the terrain.
		A heightband is used to determine the coverage of each given terrain type (dirt, snow) on a
		per-vertex basis. The fragment shader interpolates the coverage between each terrain type
		and calculates the texture. To avoid tiling artifacts, multiple scales of the same texture
		are multiplied with each other to give a more natural look. Also a detail texture is used in
		case the viewer gets really close to the ground; this gives a more realistic look of pebbles
		in the dirt or grains in the snow. Trilinear texture mapping is implemented to avoid any
		artifacts in texturing steep slopes. Lastly, a normal map is used to provide a bumpy look to
		rocky textures.

![details](/screenshots/rQ9Wizl.png)

		Notice the high detail texture when looking closely at the terrain.

	+ Polysoup Terrain
	
![polysoup](/screenshots/poly.png) ![polysoup](/screenshots/DSw74C7.png)

		The polysoup based terrain is a less constrained and more scalable, though less efficient,
		implementation for the terrain. It works by keeping track of every triangle, every edge and
		every vertex, and making a network of these elements so that it can be queried and modified.
		Where the heightmap terrain splits regions into quads, the polysoup terrain splits regions
		into 3d chunks. You can imagine splitting the terrain into a 3d grid, where each triangle
		belongs in one chunk. You'll see this 3d grid in the images where each collection of
		triangles has similar colour to indicate which chunk they belong to.

![tessellation](/screenshots/fJ1M0ka.png) ![tessellation](/screenshots/ikQqOHo.png)

		The generation process first creates a heightmap of points and then triangulates the terrain
		to represent that heightmap in a polysoup form. The heightmap is traversed and triangles are
		selected and added into the polysoup (the chunk hextree); the triangle may or may not be
		tessellated across multiple chunks. The triangle is then added to the triangle-tree network.
		Any triangly who's edge has been split and is shared with a neighbour triangle will split
		that neighbour triangle to avoid any T-junctions. 

![journal](/screenshots/journal.gif)

		To help with debugging I've created a journaling system which keeps track of changes made to
		the terrain. This way if a deformed triangle or a hole is found, I can select a triangle
		near that region and undo all of the changes back to that point in time, then I can forward
		step through the journal to re-create the terrain. This drastically helps with finding
		issues.


UI
---------

	TODO: Explanation/Images

Networking
----------

	TODO: Explanations/Images

