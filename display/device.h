#pragma once
#include <windows.h>
#include "../common/data.h"
#include "../common/string.h"
#include "background.h"
#include "measurement.h"

namespace script {

class Object;

};

namespace display {

class Background;
class Bitmap;
class BoundFont;
class Device;
class Font;
class FontMetrics;
class PrintJob;
class RootCanvas;
class SolidBackground;

/*
 *	Pen
 *
 *	In the Windows implementation, this is a dummy class.  A
 *	Windows Pen handle is cast to a pointer to a display::Pen
 *	class.
 */
class Pen {
	char dummy;
};

class Color {
	friend Font;
public:
	Color();

	Color(int wc, int i);

	Color(unsigned rgb);

	Color(int r, int g, int b);

	unsigned value() const { return _rgbVal; }

	HBRUSH hBrush() const { return _hBrush; }

private:
	void set_rgb(unsigned rgb) { _rgbVal = rgb; }

	HBRUSH		_hBrush;
	unsigned	_rgbVal;
};

class Device {
	friend RootCanvas;
	friend PrintJob;

	Device(RootCanvas* owner);

	void reset(HDC hDC, const rectangle& rectangle);

public:

	rectangle clip();

	void set_clip(const rectangle& rectangle);

	bool buffer();

	void set_buffer(bool);

	BoundFont* font() const { return _font; }

	void set_font(BoundFont* f);

	Color* background() { return _background; }

	void set_background(Color* color);

	int backMode(int mode);

	unsigned setTextColor(unsigned u);

	void squiggle(coord x, coord y, int length, Color* color);

	void centeredText(coord x, coord y, const string& s) {
		dimension d = textExtent(s);
		text(x - d.width / 2, y, s.c_str(), s.size());
	}

	void text(coord x, coord y, const string& s) {
		text(x, y, s.c_str(), s.size());
	}

	void text(coord x, coord y, const string& s, int offset, int length) {
		text(x, y, s.c_str() + offset, length);
	}

	void text(coord x, coord y, const char* s, int length);

	void textBox(const rectangle& r, const string& s, unsigned format) {
		textBox(r, s.c_str(), s.size(), format);
	}

	void textBox(const rectangle& r, const string& s, int offset, int length, unsigned format) {
		textBox(r, s.c_str() + offset, length, format);
	}

	void textBox(const rectangle& r, const char* s, int length, unsigned format);

	dimension textExtent(const string& s) {
		return textExtent(s.c_str(), s.size());
	}

	dimension textExtent(const string& s, int origin, unsigned length) {
		return textExtent(s.c_str() + origin, length);
	}

	dimension textExtent(const char* s, unsigned length);

	bool getFontMetrics(FontMetrics* metrics);

	void fillRect(const rectangle& r, Color* c);

	void frameRect(const rectangle& r, Color* c);

	void ellipse(const rectangle& r);

	void polygon(const point* p, int points);

	void bitBlt(int destX, int destY, int width, int height, Bitmap* b, int srcX, int srcY);

	void stretchBlt(int destX, int destY, int width, int height, Bitmap* b, int srcX, int srcY, int srcWidth, int srcHeight);

	void transparentBlt(int destX, int destY, int width, int height, Bitmap* b, 
						int srcX, int srcY, int srcWidth, int srcHeight, int crIndex);

	void commit();

	Pen* pen();

	void set_pen(Pen* pen);

	void line(int ix, int iy, int nx, int ny);

	void pixel(int ix, int iy, Color* c);

	point screenSize();

	HDC hDC() const { return _hDC; }

	RootCanvas* owner() const { return _owner; }

private:
	RootCanvas*		_owner;
	HDC				_hDC;
	HGDIOBJ			_hBitmap;
	HDC				hMainDC;
	HBITMAP			bufferBitMap;
	rectangle		_clip;
	point			translate;
	BoundFont*		_font;
	Color*			_background;
};

Pen* createPen(int style, int width, int color);

void deletePen(Pen* p);

extern Pen* blackPen;
extern Pen* redPen;
extern Pen* yellowPen;
extern Pen* bluePen;

enum Units {
	PIXEL,
	INCH,
	MM,
	CM,
	PT,
};

class Measure {
public:
	Measure(float value, Units unit) {
		_value = value;
		_unit = unit;
	}

	int pixels(RootCanvas* root);

	script::Object* pack();
	/*
	 *	unpack
	 *
	 *	Unpacks an Object returned from pack().  Returns true
	 *	if all fields are valid.
	 */
	bool unpack(script::Object* descriptor);

	void set_value(float value);

	void set_unit(Units unit);

	float value() const { return _value; }

	Units unit() const { return _unit; }

	Event changed;

private:
	float	_value;
	Units _unit;
};

enum FontStyle {
	F_NORMAL = 0x00,
	F_ITALIC = 0x01,
	F_UNDERLINE = 0x02,
	F_STRIKEOUT = 0x04
};

Font* serifFont();

Font* sansSerifFont();

Font* symbolFont();

Font* monoFont();

class Font {
public:
	Font(const string& family, int size, bool bold, bool italic, bool underlined, Color* color, Background* background);
	/*
	 *	Packages the public properties of the selector into a
	 *	scipt::Object with the following properties:
	 *
	 *	Optional?		Name			Notes
	 *
	 *		N		family			exact family string.  Note:
	 *								built-in values are (on Windows):
	 *									serif		- Times Roman
	 *									sansSerif	- Arial
	 *									symbol		- Marlett
	 *									mono		- Courier New
	 *		N		size			in points
	 *		Y		color			as an RGB integer
	 *		Y		background		as an RGB integer (solid color)
	 *								<< future extension: as script::Object 
	 *								background descriptor >>
	 *		Y		bold			bool, default false
	 *		Y		italic			bool, default false
	 *		Y		underlined		bool, default false
	 */
	script::Object* pack();
	/*
	 *	unpack
	 *
	 *	Unpacks an Object returned from pack().  Returns true
	 *	if all fields are valid.
	 */
	bool unpack(script::Object* descriptor);
	/*
	 *	Creates a Font object from the specified parameters, bound to the given
	 *	RootCanvas.
	 */
	BoundFont* currentFont(RootCanvas* root) const;

	void touch() {
		_touchCount++;
	}

	data::Integer			size;		// in points
	data::Boolean			bold;
	data::Boolean			italic;
	data::Boolean			underlined;

	Event					familyChanged;
	Event					textChanged;
	Event					backgroundChanged;

	void set_family(const string& f);

	const string& family() const { return _family; }

	void set_text(Color* t);

	const Color* text() const { return _text; }

	void set_background(Background* bg);

	const Background* background() const { return _background; }

private:
	int					_touchCount;
	string				_family;
	const Color*		_text;			// null = transparent
	const Background*	_background;	// null = transparent
	Color				_local;
	SolidBackground		_localBackground;
	Color				_localBackgroundColor;
};

class BoundFont {
	friend Device;
	friend RootCanvas;
	friend Font;

public:
	~BoundFont();

	const Font* font() const { return _font; }

private:
	BoundFont(const Font* font, RootCanvas* root);

	void rebind(int key);

	void bind();

	int				_key;
	const Font*		_font;
	RootCanvas*		_root;
	HFONT			_windowsFont;
};

class FontMetrics {
public:
	int		height;
	int		ascent;
	int		descent;
	int		internalLeading;
	int		externalLeading;
	int		aveCharWidth;
	int		maxCharWidth;
	int		weight;
	int		overhang;
	int		digitizedAspectX;
	int		digitizedAspectY;
	byte	firstChar;
	byte	lastChar;
	byte	defaultChar;
	byte	breakChar;
	byte	italic;
	byte	underlined;
	byte	struckOut;
	byte	pitchAndFamily;
	byte	charSet;
};

class Bitmap {
public:
	string		filename;
	byte*		data;
	HBITMAP		bitmap;

	Bitmap(Device* b, byte* d, int width, int height, int* colorTable, int ctSize);

	Bitmap(Device* b, BITMAPINFO* header, byte* data);

	~Bitmap();

	Bitmap(Device* b, const string& filename);

	void done();

	bool hasError() const { return _loadError; }

	int width() const;

	int height() const;

private:
	bool		_loadError;
	bool		_headerAllocated;
	BITMAPINFO*	_header;
};

int rgb(int r, int g, int b);

Color* createColor(unsigned rgb);

extern Color backgroundColor;
extern Color buttonHighlightColor;
extern Color buttonShadowColor;
extern Color buttonFaceColor;
extern Color scrollbarColor;
extern Color editableColor;
extern Color disabledButtonText;

extern Color white;
extern Color black;
extern Color red;
extern Color blue;
extern Color green;
extern Color yellow;

	// Mouse cursor shapes

/*
 *	MouseCursor
 *	In the Windows implementation, the MouseCursor class
 *	is used as a handle.  A Windows Mouse Cursor handle is
 *	cast to a pointer to a MouseCursor class.
 */
class MouseCursor {
	char	dummy;
};

const int ARROW = (32512);
const int IBEAM = (32513);
const int WAIT = (32514);
const int CROSS = (32515);
const int UPARROW = (32516);
const int SIZENWSE = (32642);
const int SIZENESW = (32643);
const int SIZEWE = (32644);
const int SIZENS = (32645);
const int SIZEALL = (32646);
const int NO = (32648);
const int HAND = (32649);
const int APPSTARTING = (32650);
const int HELP = (32651);

MouseCursor* standardCursor(int kind);

int swapRedBlue(int x);

int dim(int color, int fade);

}  // namespace display
