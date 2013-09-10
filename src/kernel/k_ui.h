#ifndef __K_UI_H__
#define __K_UI_H__


/*
 * UI Manager
 *
 * TODO
 *
 *  - read UI from declarative template
 *  - write UI changes to file
 *  - link with shared textures in resources
 *  - textures & borders on windows
 *  - render text
 *
 ***/


#include "util.inc.h"
#include "libutils/lib_resmgr.h"

#include "extern/GL/glew.h"

/*
=================================================

	UI Manager

	Stores and manages all UI elements and rendering

=================================================
*/

struct UIVertexBuffer {
	UIVertexBuffer(float x, float y, float t_x, float t_y) : x(x), y(y), t_x(t_x), t_y(t_y) { }
	float x, y;
	float t_x, t_y;
};

struct UIElement;
struct UIWindow;
class UIManager {
vector<UIElement*> elements;
	float width, height;
	GLint gl;
public:

	UIManager(GLint gl, float width, float height);

	UIWindow* addWindow(const UIWindow& baseWindow, UIElement** addedElement = 0);

	void render();
	void windowResize(float width, float height);
};

const static uchar UI_ELEMENT_WINDOW = 0;

const static uint UI_EVENT_CLICK = 0;
const static uint UI_EVENT_HOVER = 1;


/*
=================================================

	UI Element

	All UI elements inherit from this base class. 

=================================================
*/

struct UIElement {
	UIElement() { }
	UIElement(uint x, uint y) : x(x), y(y) { }
	UIElement(const UIElement& element);
	uint x, y; uchar z; // element top-right position
	bool visible;
	UIElement* parent = 0;

	uint evtmask;      // which events to look out for
	uchar elementType; // what type of element is this
	void* element;     // pointer to self (the true element of this)

	void construct(float screenWidth, float screenHeight);
	inline virtual void render() { }
	inline virtual void createVertexBuffer(float screenWidth, float screenHeight) { }

	vector<UIVertexBuffer> vertexBuffer;
	static GLint gl;
	GLuint vao, vbo;
};

struct UIWindow : UIElement {
	UIWindow() { }
	UIWindow(const UIElement& element, uint width, uint height) : UIElement(element), width(width), height(height) { }
	vector<UIElement*> children;
	uint width, height;

	void createVertexBuffer(float screenWidth, float screenHeight);
	void render();
};

#endif
