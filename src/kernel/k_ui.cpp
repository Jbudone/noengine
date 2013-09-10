#include "kernel/k_ui.h"

GLint UIElement::gl = 0;



// ============================================== //
UIManager::UIManager(GLint gl, float width, float height) 
	: gl(gl) {
	
	UIElement::gl = gl;


	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Setup a test UI environment
	UIWindow* window = this->addWindow( UIWindow( UIElement( 0, 0 ), 200, 200 ) );
	window->construct( width, height );


	UIWindow* window2 = this->addWindow( UIWindow( UIElement( 250, 300 ), 82, 43 ) );
	window2->construct( width, height );
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
void UIManager::windowResize(float width, float height) {
	
	// Reconstruct every element to match the new window size
	for ( auto element : elements ) {
		element->createVertexBuffer( width, height );
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

	window->elementType = UI_ELEMENT_WINDOW;
	window->element     = (void*)window;
	window->construct( width, height );


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
	this->element = element.element;
	vao = element.vao;
	vbo = element.vbo;
}
// ============================================== //


// ============================================== //
void UIElement::construct(float screenWidth, float screenHeight) {

	this->createVertexBuffer( screenWidth, screenHeight );

	glUseProgram(gl);
	Log(str(format( "Using shader (%1%) for UI element" ) % (gl) ));
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
	Texture* texture = Texture::loadTexture( "data/textures/brick1.jpg" );

	// copy file to opengl
	GLuint tex;
	glGenTextures( 1, &tex );
	glActiveTexture( GL_TEXTURE2 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, tex );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture->width, texture->height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, texture->imageData );
	glUniform1i( glGetUniformLocation( gl, "Tex2" ), 2 );

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
void UIWindow::createVertexBuffer(float screenWidth, float screenHeight) {
	vertexBuffer.clear();

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

	glDrawArrays( GL_QUADS, 0, vertexBuffer.size() );
	glBindVertexArray( 0 );
}
// ============================================== //

