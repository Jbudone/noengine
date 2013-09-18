#include "kernel/k_ui.h"

GLint UIElement::gl = 0;
GLint UIElement::gl_text = 0;
UIText::UIWord::UIGlyph::CharacterInfo UIText::UIWord::UIGlyph::characters[FONT_NUM_FACES * FONT_NUM_SIZES][ASCII_HIGH_CODE - ASCII_LOW_CODE] = { };
GLuint UIText::UIWord::UIGlyph::atlasTex = 0;
uint UIText::UIWord::UIGlyph::atlasWidth  = 0;
uint UIText::UIWord::UIGlyph::atlasHeight = 0;



// ============================================== //
UIManager::UIManager(GLint gl, GLint gl_text, float width, float height) 
	: gl(gl), width(width), height(height) {
	
	UIElement::gl = gl;
	UIElement::gl_text = gl_text;

	UIText::UIWord::UIGlyph::initFonts();

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Setup a test UI environment
	UIWindow* window = this->addWindow( UIWindow( UIElement( 0, 0 ), 1400, 100 ) );
	window->construct();
	window->addText( "Testing, testing.. 123!" );


	UIWindow* window2 = this->addWindow( UIWindow( UIElement( 250, 300 ), 82, 43 ) );
	window2->construct();
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

}
// ============================================== //


// ============================================== //
void UIManager::render() {
	
	glUseProgram(gl);
	for ( auto element : elements ) {
		element->render();
	}
	glUseProgram(0);
}
// ============================================== //


// ============================================== //
void UIManager::windowResize() {
	
	// Reconstruct every element to match the new window size
	for ( auto element : elements ) {
		element->createVertexBuffer();
	}
}
// ============================================== //


// ============================================== //
UIWindow* UIManager::addWindow(const UIWindow& baseWindow, UIElement** addedElement) {
	UIElement* element = new UIElement();
	UIWindow*  window  = new UIWindow();

	window->x        = baseWindow.x;
	window->y        = baseWindow.y;
	window->z        = baseWindow.z;
	window->width    = baseWindow.width;
	window->height   = baseWindow.height;
	window->visible  = baseWindow.visible;
	window->children = baseWindow.children;

	window->manager     = this;
	window->elementType = UI_ELEMENT_WINDOW;
	window->element     = (void*)window;
	window->construct();


	elements.push_back( window );
	if ( addedElement ) addedElement = &element;
	return window;
}
// ============================================== //


// ============================================== //
UIElement::UIElement(const UIElement& element) {
	x = element.x;
	y = element.y;
	z = element.z;
	visible = element.visible;
	parent = element.parent;
	evtmask = element.evtmask;
	elementType = element.elementType;
	manager = element.manager;
	this->element = element.element;
	vao = element.vao;
	vbo = element.vbo;
}
// ============================================== //


// ============================================== //
void UIWindow::construct() {

	this->createVertexBuffer();

	glUseProgram(gl);
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	glGenBuffers( 1, &vbo );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(UIVertexBuffer), vertexBuffer.data(), GL_STATIC_DRAW );

	GLint glVertex   = glGetAttribLocation( gl, "in_Position" );
	GLint glTexcoord = glGetAttribLocation( gl, "in_Texcoord" );
	glEnableVertexAttribArray( glVertex );
	glEnableVertexAttribArray( glTexcoord );
	glVertexAttribPointer( glVertex, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertexBuffer), (void*)0 );
	glVertexAttribPointer( glTexcoord, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertexBuffer), (void*)( sizeof(float) * 2 ) );


	// load texture image
	Texture* texture = Texture::loadTexture( "data/textures/windowtex.png", Texture::TEX_CHAN_ALPHA );
	uint borderLeft   = 100,
		 borderRight  = 90,
		 borderTop    = 90,
		 borderBottom = 100;

	// copy file to opengl
	GLuint tex;
	glGenTextures( 1, &tex );
	glActiveTexture( GL_TEXTURE2 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, tex );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, texture->imageData );
	glUniform1i( glGetUniformLocation( gl, "Tex2" ), 2 );
	glUniform4fv( glGetUniformLocation( gl, "borderWidths" ), 1, glm::value_ptr(glm::vec4( borderLeft, borderRight, borderTop, borderBottom )) );
	glUniform2fv( glGetUniformLocation( gl, "texSize" ), 1, glm::value_ptr(glm::vec2( texture->width, texture->height )) );
	glUniform2fv( glGetUniformLocation( gl, "winSize" ), 1, glm::value_ptr(glm::vec2( this->manager->width, this->manager->width )) );

	// mipmapping
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D );

	glActiveTexture( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );
}
// ============================================== //


// ============================================== //
void UIWindow::createVertexBuffer() {
	vertexBuffer.clear();
	float screenWidth  = this->manager->width,
		  screenHeight = this->manager->height;
	float left   = 2 * (int)x / screenWidth - 1,
		  right  = 2 * (int)(x+width) / screenWidth - 1,
		  top    = -2 * (int)y / screenHeight + 1,
		  bottom = -2 * (int)(y+height) / screenHeight + 1;
	float texLeft   = 0.0f,
		  texRight  = 1.0f,
		  texTop    = 0.0f,
		  texBottom = 1.0f;
	vertexBuffer.push_back( UIVertexBuffer( left, top, texLeft, texTop ) );
	vertexBuffer.push_back( UIVertexBuffer( left, bottom, texLeft, texBottom ) );
	vertexBuffer.push_back( UIVertexBuffer( right, bottom, texRight, texBottom ) );
	vertexBuffer.push_back( UIVertexBuffer( right, top, texRight, texTop ) );
}
// ============================================== //


// ============================================== //
void UIWindow::render() {

	glUseProgram(gl);
	glBindVertexArray( vao );

	// glUniform1i( glGetUniformLocation( gl, "Tex2" ), GL_TEXTURE3 - GL_TEXTURE0 );
	glDrawArrays( GL_QUADS, 0, vertexBuffer.size() );
	glBindVertexArray( 0 );

	for ( auto child : children ) {
		child->render();
	}
}
// ============================================== //



// ============================================== //
void UIWindow::addText(const char* text) {
	UIText* uitext = new UIText( text );
	uitext->parent = this;
	uitext->manager = manager;

	uitext->elementType = UI_ELEMENT_TEXT;
	uitext->element     = (void*)uitext;
	uitext->y           = this->y + 0; // padding
	uitext->x           = this->x + 50; // padding
	uitext->construct();
	this->children.push_back( uitext );
}




	/******************************************************************************/
	/*
									Glyphs & Fonts (Text)
																				   */
	/******************************************************************************/
	/*

	   		Glyphs, fonts, styles and text.. Oh, my!

																					*/





// ============================================== //
void UIText::UIWord::UIGlyph::initFonts() {

	FT_Library ft;
	if ( FT_Init_FreeType( &ft ) ) throw exception();

	// const char* font_faces[] = { "data/fonts/freefont/FreeSerif.ttf",
	// 							 "data/fonts/droid/DroidSans.ttf",
	// 							 "data/fonts/freefont/FreeSans.ttf" };
	// int font_sizes[]    = { 12, 14, 48, 24, 54, 64 };
	FT_Face face;

	// Find the boundaries of the entire texture atlas
	for ( uchar iFont = 0; iFont < FONT_NUM_FACES; ++iFont ) {
		for ( uchar iSize = 0; iSize < FONT_NUM_SIZES; ++iSize ) {

			if ( FT_New_Face( ft, font_faces[iFont], 0, &face ) ) throw exception();
			if ( FT_Set_Pixel_Sizes( face, 0, font_sizes[iSize] ) ) throw exception();
			std::pair<uint,uint> boundaries = getFontBoundaries( face );
			atlasWidth  = std::max( atlasWidth, boundaries.first );
			atlasHeight += boundaries.second;

		}
	}
	
	// Draw up the texture atlas
	glGenTextures(1, &atlasTex);
	glActiveTexture(GL_TEXTURE3);
	glEnable( GL_TEXTURE_2D );
	glBindTexture(GL_TEXTURE_2D, atlasTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// NOTE: use one channel since its the most efficient storage (1byte)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, 0);


	// Draw up the texture atlas (for each individual face/style/size combination)
	uint yOffset = 0,
		 glyphFontIndex = 0;
	for ( uchar iFont = 0; iFont < FONT_NUM_FACES; ++iFont ) {
		for ( uchar iSize = 0; iSize < FONT_NUM_SIZES; ++iSize ) {

			if ( FT_New_Face( ft, font_faces[iFont], 0, &face ) ) throw exception();
			FT_Set_Pixel_Sizes( face, 0, font_sizes[iSize] );
			setupFont( face, glyphFontIndex, yOffset );
			++glyphFontIndex;

		}
	}
	glGenerateMipmap( GL_TEXTURE_2D );
}
// ============================================== //


// ============================================== //
// What are the atlas width/height we need for this fontface?
std::pair<uint,uint> UIText::UIWord::UIGlyph::getFontBoundaries(FT_Face face) {
	int  width  = 0,
		 height = 0;
	FT_GlyphSlot g = face->glyph;
	for( int i = ASCII_LOW_CODE; i < ASCII_HIGH_CODE; ++i ) {
		if( FT_Load_Char( face, i, FT_LOAD_RENDER ) ) continue;

		width += g->bitmap.width;
		height = std::max( height, g->bitmap.rows );
	}

	return std::pair<uint,uint>(width,height);
}
// ============================================== //


// ============================================== //
// Draw this fontface to the texture atlas
// Sets yOffset to the y amount to advance forwards (the height of this portion of the atlas)
// NOTE: GL_TEXTURE_2D should be active, and atlasWidth/atlasHeight should be set
void UIText::UIWord::UIGlyph::setupFont(FT_Face face, uchar glyphFontIndex, uint& yOffset) {

	uint x = 0;
	int height = 0;
	FT_GlyphSlot g = face->glyph;
	for ( int i = ASCII_LOW_CODE, charcode = 0; i < ASCII_HIGH_CODE; ++i, ++charcode ) {
		// x += 1; // offset by 1 px to avoid artifacts
		if( FT_Load_Char( face, i, FT_LOAD_RENDER ) ) continue;
		characters[glyphFontIndex][charcode] = (CharacterInfo){
			g->advance.x >> 6, // advance

			g->bitmap.width, // texture width  (in px)
			g->bitmap.rows,  // texture height (in px)

			(float)g->bitmap_left,  // offset of glyph (relative to cursor)
			(float)g->bitmap_top,

			(float)x / (float)atlasWidth,                // offset of glyph (relative to texture)
			(float)yOffset / (float)atlasHeight,
			(float)g->bitmap.width / (float)atlasWidth,
			(float)g->bitmap.rows / (float)atlasHeight
		};

		glTexSubImage2D( GL_TEXTURE_2D, 0, x, yOffset, g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer );
		x += g->bitmap.width;
		height = std::max( height, g->bitmap.rows );
	}

	yOffset += height;
}
// ============================================== //


// ============================================== //
UIText::UIWord::UIGlyph::UIGlyph(char charcode, uchar fontface) : charcode(charcode) {
	this->glyph = &characters[fontface][charcode - ASCII_LOW_CODE];
}
// ============================================== //


// ============================================== //
UIText::UIWord::UIWord(const char* text, uchar fontface) : fontface(fontface) {
	length = strlen( text );
	width = 0;
	glyphs = new UIGlyph*[length];
	ushort i = 0;
	for ( const char *c = text; *c; ++c, ++i ) {
		glyphs[i] = new UIGlyph( *c, fontface );
		// glyphs[i]->glyph = &UIWord::UIGlyph::characters[*c];
		width += glyphs[i]->glyph->advance_x;
	}
}
UIText::UIWord::UIWord(const char* text, ushort length, uchar fontface) : length(length), fontface(fontface) {
	width = 0;
	glyphs = new UIGlyph*[length];
	ushort i = 0;
	for ( const char *c = text; *c, i < length; ++c, ++i ) {
		glyphs[i] = new UIGlyph( *c, fontface );
		// glyphs[i]->glyph = &UIWord::UIGlyph::characters[*c];
		width += glyphs[i]->glyph->advance_x;
	}
}
UIText::UIWord::~UIWord() {
	delete glyphs;
}
// ============================================== //



// ============================================== //
UIText::UITextline::UITextline(UIWord** words, ushort numwords) : numwords(numwords) {
	this->words = new UIWord*[numwords];
	for ( ushort i = 0; i < numwords; ++i ) {
		this->words[i] = words[i];
	}
}
UIText::UITextline::~UITextline() {
	delete words;
}
// ============================================== //


// ============================================== //
UIText::UIText(const char* text) {
	this->lineheight = 20;
	this->text = string(text);

	// setup glyph buffer
	// count up how many words we have first (to alloc the space)
	numwords = 0;
	ushort length = 0;
	bool inWord = false;
	uchar fontface = 4;
	for ( const char *c = text; *c; ++c, ++length ) {
		if ( *c < 32 ) continue; // ignore control characters
		if ( *c == 32 && inWord ) {
			numwords++; // this was a word
			inWord = false;
			continue;
		} else if ( !inWord ) inWord = true;
	}
	if ( inWord ) ++numwords;


	// setup the words
	// count up the lines
	words = new UIWord*[numwords];
	numlines = 1;
	linewidth = 300;
	length = 0;
	ushort wIndex = 0;
	ushort lWidth = 0;
	const char *c;
	for ( c = text; *c; ++c, ++length ) {
		if ( *c < 32 ) continue; // ignore control characters
		if ( *c == 32 && inWord ) {
			inWord = false;

			words[wIndex] = new UIWord(c - length, length, fontface);
			length = 0;
			if ( (lWidth + words[wIndex]->width) > linewidth ) {
				if ( lWidth == 0 ) { // the word is bigger than the max line width..accept it anyways
					++numlines;
					continue;
				}
				++numlines;
				lWidth = words[wIndex]->width;
			}
			lWidth += words[wIndex]->width;
			++wIndex;
			continue;
		}
		inWord = true;
	}
	if ( length-1 > 0 ) words[wIndex] = new UIWord(c - length, length, fontface);

	// setup the lines
	lines = new UITextline*[numlines];
	wIndex = 0;
	lWidth = 0;
	ushort lIndex = 0;
	length = 0;
	for ( ; wIndex < numwords; ++wIndex, ++length ) {
		if ( (lWidth += words[wIndex]->width) > linewidth ) {
			if ( lWidth == 0 ) { // word is bigger than max line width
				lines[lIndex] = new UITextline( &words[(wIndex-length)], length+1 ); // one word
				++lIndex;
				length = 0;
				continue;
			}
			lines[lIndex] = new UITextline( &words[(wIndex-length)], length+1 );
			++lIndex;
			length = 0;
			lWidth = words[wIndex]->width;
			continue;
		}
	}
	if ( length-1 > 0 ) {
		lines[lIndex] = new UITextline( &words[(wIndex-(length-1))], length-1 );
	}
}
UIText::~UIText() {
	for ( ushort i = 0; i < numlines; ++i ) {
		delete lines[i];
	}
	delete lines;

	for ( ushort i = 0; i < numwords; ++i ) {
		delete words[i];
	}
	delete words;
}
// ============================================== //



// ============================================== //
void UIText::construct() {

	this->createVertexBuffer();

	glUseProgram(gl_text);
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	glGenBuffers( 1, &vbo );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, glyphBuffer.size() * sizeof(UIGlyphBuffer), glyphBuffer.data(), GL_STATIC_DRAW );

	GLint glCoords = glGetAttribLocation( gl_text, "coord" );
	glEnableVertexAttribArray( glCoords );
	glVertexAttribPointer( glCoords, 4, GL_FLOAT, GL_FALSE, sizeof(UIGlyphBuffer), (void*)0 );
	glUniform1i( glGetUniformLocation( gl_text, "Tex4" ), GL_TEXTURE3 - GL_TEXTURE0 );


	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );
}
// ============================================== //


// ============================================== //
void UIText::createVertexBuffer() {
	UIWord::UIGlyph::CharacterInfo* charInfo;
	glyphBuffer.clear();
	
	float screenWidth  = this->manager->width,
		  screenHeight = this->manager->height;
	uint y = this->y,// UIWord::UIGlyph::atlasHeight,
		 x = this->x,
		 minLineHeight;
	for ( ushort lIndex = 0; lIndex < numlines; ++lIndex, y += minLineHeight ) {
		minLineHeight = lineheight;
		for ( ushort wIndex = 0, x = this->x; wIndex < lines[lIndex]->numwords; ++wIndex ) {
			uint faceHeight = font_sizes[(lines[lIndex]->words[wIndex]->fontface % FONT_NUM_SIZES)] ;
			minLineHeight = std::max( minLineHeight, faceHeight );
			for ( ushort gIndex = 0; gIndex < lines[lIndex]->words[wIndex]->length; ++gIndex ) {
				charInfo = lines[lIndex]->words[wIndex]->glyphs[gIndex]->glyph;
				float texLeft = charInfo->texLeft,
					  texTop = charInfo->texTop,
					  texWidth = charInfo->texWidth,
					  texHeight = charInfo->texHeight,
					  advance = charInfo->advance_x;
				float left   = 2 * (float)(x + texLeft) / screenWidth - 1,
					  right  = 2 * (float)(x + texLeft + texWidth) / screenWidth -1,
					  top    = -2 * (float)(y + faceHeight - texTop) / screenHeight + 1,
					  bottom = -2 * (float)(y + faceHeight - texTop + texHeight) / screenHeight + 1;

				float tx = charInfo->tx,
					  tw = charInfo->tw,
					  tt = charInfo->tt,
					  th = charInfo->th;

				glyphBuffer.push_back( (UIGlyphBuffer){ left, top, tx, tt } );
				glyphBuffer.push_back( (UIGlyphBuffer){ left, bottom, tx, tt + th } );
				glyphBuffer.push_back( (UIGlyphBuffer){ right, bottom, tx + tw, tt + th } );
				glyphBuffer.push_back( (UIGlyphBuffer){ right, top, tx + tw, tt } );

				x += advance;
			}
		}
	}
}
// ============================================== //


// ============================================== //
void UIText::render() {

	glUseProgram(gl_text);
	glBindVertexArray( vao );

	glUniform1i( glGetUniformLocation( gl_text, "Tex4" ), GL_TEXTURE3 - GL_TEXTURE0 );
	glDrawArrays( GL_QUADS, 0, glyphBuffer.size() );
	glBindVertexArray( 0 );
}

