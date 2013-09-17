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
 *  - load/store textures for multiple faces/sizes/styles;
 *  	store texture bindings along with shared textures
 *
 ***/


#include "util.inc.h"
#include "libutils/lib_resmgr.h"

#include <extern/freetype/ft2build.h>
#include FT_FREETYPE_H

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

struct UIGlyphBuffer {
	UIGlyphBuffer(float x, float y, float t_x, float t_y) : x(x), y(y), t_x(t_x), t_y(t_y) { }
	float x, y;
	float t_x, t_y;
};

struct UIElement;
struct UIWindow;
struct UIText;
class UIManager {
	vector<UIElement*> elements;
	float width, height;
	GLint gl;
public:

	UIManager(GLint gl, GLint gl_text, float width, float height);

	UIWindow* addWindow(const UIWindow& baseWindow, UIElement** addedElement = 0);

	void render();
	void windowResize(float width, float height);
};

const static uchar UI_ELEMENT_WINDOW = 0;
const static uchar UI_ELEMENT_TEXT = 1;

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

	inline virtual void construct(float screenWidth, float screenHeight) { }
	inline virtual void render() { }
	inline virtual void createVertexBuffer(float screenWidth, float screenHeight) { }

	static GLint gl;
	static GLint gl_text;
	GLuint vao, vbo;
};

struct UIWindow : UIElement {
	UIWindow() { }
	UIWindow(const UIElement& element, uint width, uint height) : UIElement(element), width(width), height(height) { }
	vector<UIElement*> children;
	uint width, height;

	vector<UIVertexBuffer> vertexBuffer;
	void construct(float screenWidth, float screenHeight);
	void createVertexBuffer(float screenWidth, float screenHeight);
	void render();

	void addText(const char* text);
};



/* UI Text
 *
 * A collection of words made up into a group of text
 * (paragraph). The text can have a boundary and will
 * automatically reposition the words to fit within that
 * boundary (newlines, tabs, etc.)
 ***/
#define ASCII_HIGH_CODE 128
#define ASCII_LOW_CODE 32
struct UIText : UIElement {

	UIText(const char* text);
	~UIText();

	void construct(float screenWidth, float screenHeight);
	void createVertexBuffer(float screenWidth, float screenHeight);
	void render();

	/* UI Word
	 *
	 * A collection of glyphs terminated by spaces. Each
	 * word can contain different properties (font face,
	 * bold, italics, scale, etc.). A word can construct a
	 * relative set of coordinates for drawing the text, and
	 * apply an offset for the overall word (offset, word
	 * spacing, etc.)
	 ***/
	struct UIWord {

		UIWord(const char* text);
		UIWord(const char* text, ushort length);
		~UIWord();


		/* UI Glyph
		 *
		 * A single character
		 ***/
		struct UIGlyph {

			UIGlyph(char charcode);


			/* Character Info
			 *
			 * Details necessary to describe a single character from the
			 * ASCII table in its Glyph form (where is it within the
			 * texture atlas)
			 ***/
			struct CharacterInfo {
				int    advance_x, // TODO: how far to advance to next char ?
					   advance_y,

					   texWidth,  // width/height of glyph in texture
					   texHeight;

				float  texLeft,   // offset of glyph in texture (relative to cursor
					   texTop,

					   tx,        // offset of glyph in texture (tex coords)
					   tw,        // width of character (scaled to atlas coords)
					   th;        // height of character (scaled to atlas coords)
			};

			static CharacterInfo characters[ASCII_HIGH_CODE - ASCII_LOW_CODE];
			static void initFonts();
			static GLuint atlasTex;
			static int atlasWidth;
			static int atlasHeight;

			char charcode;
			CharacterInfo* glyph;
		};

		UIGlyph** glyphs;
		ushort length;
		ushort width;
	};


	/* UI Textline
	 *
	 * A single line within the collection of rows for the
	 * text paragraph. Note line spacing and indentation is
	 * considered here
	 ***/
	struct UITextline {

		UITextline(UIWord** words, ushort numwords);
		~UITextline();
		
		UIWord** words;
		ushort numwords;
	};

	UITextline** lines;
	ushort numlines;
	ushort linewidth;

	UIWord** words;
	ushort numwords;
	string text;

	vector<UIGlyphBuffer> glyphBuffer;

	uint lineheight;
};

#endif
