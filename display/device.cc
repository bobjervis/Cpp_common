#include "../common/platform.h"
#include "device.h"

#include <windows.h>
#include "../common/atom.h"
#include "../common/machine.h"
#include "background.h"
#include "window.h"

namespace display {

Device::Device(RootCanvas* owner) {
	_owner = owner;
	_hDC = null;
	hMainDC = null;
	bufferBitMap = null;
	_font = null;
	_background = null;
}

void Device::reset(HDC hDC, const rectangle& paintRect) {
	_hDC = hDC;
	hMainDC = hDC;
	translate.x = 0;
	translate.y = 0;
	_clip = paintRect;
	_font = null;
}

rectangle Device::clip() {
	return _clip;
}

void Device::set_clip(const rectangle& rectangle) {
	_clip = rectangle;
	HRGN rgn = CreateRectRgn(_clip.topLeft.x, _clip.topLeft.y,
						_clip.opposite.x, _clip.opposite.y);
	SelectClipRgn(_hDC, rgn);
	DeleteObject(rgn);
}

bool Device::buffer() {
	return _hDC != hMainDC;
}

void Device::set_buffer(bool assigned) {
	if (assigned){
		if (hMainDC != _hDC)
			return;
		_hDC = CreateCompatibleDC(hMainDC);
		if (_hDC == null) {
			DWORD err = GetLastError();
			char buffer[256];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, null, err, 0, buffer, sizeof buffer, null);
			fatalMessage("Could not create buffering DC: " + string(buffer));
		}
		bufferBitMap = CreateCompatibleBitmap(hMainDC, _clip.opposite.x, _clip.opposite.y);
		if (bufferBitMap)
			_hBitmap = SelectObject(_hDC, bufferBitMap);
		else {
			DeleteDC(_hDC);
			_hDC = hMainDC;
		}
	} else {
		if (hMainDC == _hDC)
			return;
		SelectObject(_hDC, _hBitmap);
		DeleteDC(_hDC);
		DeleteObject(bufferBitMap);
		bufferBitMap = null;
		_hBitmap = null;
		_hDC = hMainDC;
	}
}

void Device::set_font(BoundFont* f) {
	_font = f;
	if (f != null)
		SelectObject(_hDC, f->_windowsFont);
}

void Device::set_background(Color* color) {
	_background = color;
	SelectObject(_hDC, color->hBrush());
}

int Device::backMode(int mode) {
	return SetBkMode(_hDC, mode);
}

unsigned Device::setTextColor(unsigned u) {
	return SetTextColor(_hDC, swapRedBlue(u));
}

void Device::squiggle(coord x, coord y, int length, Color *color) {
	for (int i = 0; i < length; i++) {
		if (i & 1)
			pixel(x + i, y + 1, color);
		else
			pixel(x + i, y, color);
	}
}

void Device::text(coord x, coord y, const char* s, int length) {
	TextOut(_hDC, x, y, s, length);
}

void Device::textBox(const rectangle& r, const char* s, int length, unsigned format) {
	RECT wr;

	wr.top = r.topLeft.y;
	wr.left = r.topLeft.x;
	wr.bottom = r.opposite.y;
	wr.right = r.opposite.x;
	DrawText(_hDC, s, length, &wr, format|DT_NOPREFIX);
}

dimension Device::textExtent(const char* s, unsigned length) {
	dimension d;
	SIZE lps;

	GetTextExtentPoint32(_hDC, s, length, &lps);
	d.width = coord(lps.cx);
	d.height = coord(lps.cy);
	return d;
}

bool Device::getFontMetrics(FontMetrics* metrics) {
	if (GetTextMetrics(_hDC, (TEXTMETRIC*)metrics) != FALSE)
		return true;
	else
		return false;
}

void Device::fillRect(const rectangle& r, Color* c) {
	RECT rr;

	rr.top = r.topLeft.y;
	rr.left = r.topLeft.x;
	rr.right = r.opposite.x;
	rr.bottom = r.opposite.y;
	HBRUSH h;
	if (c != null)
		h = c->hBrush();
	else
		h = null;
	FillRect(_hDC, &rr, h);
}

void Device::frameRect(const rectangle& r, Color* c) {
	RECT rr;

	rr.top = r.topLeft.y;
	rr.left = r.topLeft.x;
	rr.right = r.opposite.x;
	rr.bottom = r.opposite.y;
	HBRUSH h;
	if (c != null)
		h = c->hBrush();
	else
		h = null;
	FrameRect(_hDC, &rr, h);
}

void Device::ellipse(const rectangle& r) {
	Ellipse(_hDC, r.topLeft.x, r.topLeft.y, r.opposite.x, r.opposite.y);
}

void Device::polygon(const point* p, int pointCount) {
	POINT pointArray[20];
	POINT* points;
	if (pointCount <= 20)
		points = pointArray;
	else
		points = new POINT[pointCount];
	for (int i = 0; i < pointCount; i++) {
		points[i].x = p[i].x;
		points[i].y = p[i].y;
	}
	Polygon(_hDC, points, pointCount);
	if (pointCount > 20)
		delete points;
}

void Device::bitBlt(int destX, int destY, int width, int height, Bitmap* b, int srcX, int srcY) {
	if (width == 0)
		return;
	HDC ndc = CreateCompatibleDC(hMainDC);
	HGDIOBJ o = SelectObject(ndc, b->bitmap);
	int increment = 10000 / width;
	if (increment == 0)
		increment = 1;
	while (height > 0) {
		int h;
		if (height > increment)
			h = increment;
		else
			h = height;
		if (!BitBlt(_hDC, destX, destY, width, h, ndc, srcX, srcY, SRCCOPY))
			debugPrint(string("BitBlt failed: ") + GetLastError() + "\n");
		srcY += h;
		destY += h;
		height -= h;
	}
	SelectObject(ndc, o);
	DeleteDC(ndc);
}

void Device::stretchBlt(int destX, int destY, int width, int height, Bitmap* b, int srcX, int srcY, int srcWidth, int srcHeight) {
	if (width == 0)
		return;
	HDC ndc = CreateCompatibleDC(hMainDC);
	HGDIOBJ o = SelectObject(ndc, b->bitmap);
	int increment = 50000 / width;
	while (height > 0) {
		int h;
		if (height > increment)
			h = increment;
		else
			h = height;
		StretchBlt(_hDC, destX, destY, width, h, ndc, srcX, srcY, srcWidth, srcHeight, SRCCOPY);
		srcY += h;
		destY += h;
		height -= h;
	}
	SelectObject(ndc, o);
	DeleteDC(ndc);
}

void Device::transparentBlt(int destX, int destY, int width, int height, Bitmap* b, 
							int srcX, int srcY, int srcWidth, int srcHeight, int crIndex) {
	HDC ndc = CreateCompatibleDC(_hDC);
	if (ndc == null)
		debugPrint("CreateCompatibleDC failed\n");
	HGDIOBJ o = SelectObject(ndc, b->bitmap);
	if (o == null)
		debugPrint("SelectObject failed\n");
//		res := windows.BitBlt(hDC, destX, destY, width, height, ndc, srcX, srcY, windows.SRCCOPY)
	BOOL res = TransparentBlt(_hDC, destX, destY, width, height, ndc, srcX, srcY, srcWidth, srcHeight, crIndex);
	SelectObject(ndc, o);
	if (res == FALSE)
		debugPrint(string(" TransparentBlt failed ") + GetLastError() + "\n");
	DeleteDC(ndc);
}

void Device::commit() {
	dimension d = _clip.size();
	BitBlt(hMainDC, _clip.topLeft.x, _clip.topLeft.y, d.width, d.height, _hDC, 
								_clip.topLeft.x, _clip.topLeft.y, SRCCOPY);
}

void Device::set_pen(Pen* pen) {
	SelectObject(_hDC, (HGDIOBJ)pen);
}

void Device::line(int ix, int iy, int nx, int ny) {
	MoveToEx(_hDC, ix, iy, null);
	LineTo(_hDC, nx, ny);
}

void Device::pixel(int ix, int iy, Color* c) {
	SetPixel(_hDC, ix, iy, swapRedBlue(c->value()));
}

point Device::screenSize() {
	point p; 
	p.x = GetDeviceCaps(_hDC, HORZRES);
	p.y = GetDeviceCaps(_hDC, VERTRES);
	return p;
}

Pen* createPen(int style, int width, int color) {
	return (Pen*)CreatePen(style, width, swapRedBlue(color));
}

void deletePen(Pen* p) {
	DeleteObject((HGDIOBJ)p);
}

Pen* blackPen = createPen(PS_SOLID, 1, 0);
Pen* bluePen = createPen(PS_SOLID, 1, 0x0000ff);
Pen* redPen = createPen(PS_SOLID, 1, 0xff0000);
Pen* yellowPen = createPen(PS_SOLID, 1, 0xffff00);

#define INCHES_PER_MM 0.03937
#define INCHES_PER_CM 0.3937
#define INCHES_PER_PT (1.0 / 72)

int Measure::pixels(RootCanvas* root) {
	Device* device = root->device();

	switch (_unit) {
	case	INCH:
		return (int)(_value * GetDeviceCaps(device->hDC(), LOGPIXELSX));

	case	MM:
		return (int)(_value * INCHES_PER_MM * GetDeviceCaps(device->hDC(), LOGPIXELSX));

	case	CM:
		return (int)(_value * INCHES_PER_CM * GetDeviceCaps(device->hDC(), LOGPIXELSX));

	case	PIXEL:
		return (int)_value;

	case	PT:
		return (int)(_value * INCHES_PER_PT * GetDeviceCaps(device->hDC(), LOGPIXELSX));

	default:
		return 0;
	}
}

void Measure::set_unit(Units unit) {
	_unit = unit;
	changed.fire();
}

void Measure::set_value(float value) {
	_value = value;
	changed.fire();
}

static const char* unitLabels[] = {
	"px",
	"in",
	"mm",
	"cm",
	"pt"
};

bool Measure::unpack(script::Object* descriptor) {
	script::Atom* a = descriptor->get("value");
	if (a == null)
		return false;
	_value = (float)a->toString().toDouble();
	a = descriptor->get("unit");
	if (a == null)
		return false;
	string s = a->toString();
	for (_unit = PIXEL; _unit < dimOf(unitLabels); _unit = (Units)(_unit + 1))
		if (s == unitLabels[_unit])
			break;
	if (_unit >= dimOf(unitLabels))
		return false;
	changed.fire();
	return true;
}

script::Object* Measure::pack() {
	script::Object* o = new script::Object();
	o->put("tag", new script::String("measure"));
	o->put("value", new script::String(string(_value)));
	o->put("unit", new script::String(unitLabels[_unit]));
	return o;
}


Font* serifFont() {
	static Font font("Times New Roman", 10, false, false, false, null, null);

	return &font;
}

Font* sansSerifFont() {
	static Font font("Arial", 10, false, false, false, null, null);

	return &font;
}

Font* symbolFont() {
	static Font font("+Marlett", 10, false, false, false, null, null);

	return &font;
}

Font* monoFont() {
	static Font font("Courier New", 10, false, false, false, null, null);

	return &font;
}

Font::Font(const string& family, int pointSize, bool bold, bool italic, bool underlined, Color* text, Background* background) : _localBackground(&_localBackgroundColor) {
	_family = family;
	size.set_value(pointSize);
	this->bold.set_value(bold);
	this->italic.set_value(italic);
	this->underlined.set_value(underlined);
	_text = text;
	_background = background;
	_touchCount = 0;
}

BoundFont* Font::currentFont(RootCanvas *root) const {
	BoundFont* bf = root->bind(this);
	bf->rebind(_touchCount);
	return bf;
}

bool Font::unpack(script::Object* descriptor) {
	script::Atom* a = descriptor->get("family");
	if (a == null)
		return false;
	set_family(a->toString());
	familyChanged.fire();
	a = descriptor->get("size");
	if (a == null)
		return false;
	int i = a->toString().toInt();
	if (i < 6 || i > 72)
		return false;
	size.set_value(i);
	a = descriptor->get("bold");
	if (a)
		bold.set_value(a->toString().toBool());
	a = descriptor->get("italic");
	if (a)
		italic.set_value(a->toString().toBool());
	a = descriptor->get("underlined");
	if (a)
		underlined.set_value(a->toString().toBool());
	a = descriptor->get("color");
	if (a) {
		_local.set_rgb(a->toString().toInt());
		_text = &_local;
		textChanged.fire();
	}
	a = descriptor->get("bg");
	if (a) {
		_localBackgroundColor.set_rgb(a->toString().toInt());
		_background = &_localBackground;
		backgroundChanged.fire();
	}
	return true;
}

script::Object* Font::pack() {
	script::Object* o = new script::Object();
	o->put("tag", new script::String("font"));
	o->put("family", new script::String(_family));
	o->put("size", new script::String(string(size.value())));
	if (bold.value())
		o->put("bold", new script::String("true"));
	if (italic.value())
		o->put("italic", new script::String("true"));
	if (underlined.value())
		o->put("underlined", new script::String("true"));
	if (_text)
		o->put("color", new script::String(string((int)_text->value())));
	if (_background) {
		string packedString;

		_background->pack(&packedString);
		o->put("bg", new script::String(packedString));
	}
	return o;
}

void Font::set_family(const string &f) {
	_family = f;
	touch();
	familyChanged.fire();
}

BoundFont::BoundFont(const Font* font, RootCanvas* root) {
	_font = font;
	_key = -1;
	_root = root;
	_windowsFont = null;
}

void BoundFont::rebind(int key) {
	if (_key != key) {
		_key = key;
		DeleteObject(_windowsFont);
		bind();
	}
}

BoundFont::~BoundFont() {
	DeleteObject(_windowsFont);
}

void BoundFont::bind() {
	Device* device = _root->device();
	int pixelsPerInchY = GetDeviceCaps(device->hDC(), LOGPIXELSY);
	int height = -int((long(_font->size.value()) * pixelsPerInchY) / 72);

	const char* family = _font->family().c_str();
	DWORD charSet = DEFAULT_CHARSET;
	if (*family == '+') {
		family++;
		charSet = SYMBOL_CHARSET;
	}
	// Windows-specific mapping of portable fonts
	if (_stricmp(family, "serif") == 0)
		family = "Times Roman";
	else if (_stricmp(family, "sansSerif") == 0)
		family = "Arial";
	else if (_stricmp(family, "monospace") == 0)
		family = "Courier New";
	else if (_stricmp(family, "symbol") == 0) {
		family = "Marlett";
		charSet = SYMBOL_CHARSET;
	}
	_windowsFont = CreateFont(height, 0, 0, 0, _font->bold.value() ? FW_BOLD : FW_NORMAL,
							  _font->italic.value(), _font->underlined.value(), 0,
							  charSet, 0, 0, 0, 0, family);
}

Bitmap::Bitmap(Device* b, byte* d, int width, int height, int* colorTable, int ctSize) {
	data = d;
	_header = (BITMAPINFO*)malloc(sizeof (BITMAPINFOHEADER) + ctSize * sizeof (int));
	_headerAllocated = true;
	_header->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	_header->bmiHeader.biWidth = width;
	_header->bmiHeader.biHeight = height;
	_header->bmiHeader.biPlanes = 1;
	_header->bmiHeader.biBitCount = 8;
	_header->bmiHeader.biCompression = BI_RGB;
	_header->bmiHeader.biClrUsed = ctSize;
	int* ct = (int*)_header->bmiColors;
	memcpy(ct, colorTable, ctSize * sizeof (int));
//		bitmap = windows.CreateCompatibleBitmap(b.hDC, width, height);
	int i;
	bitmap = CreateCompatibleBitmap(b->hDC(), _header->bmiHeader.biWidth, _header->bmiHeader.biHeight);
	if (bitmap == null) {
		i = GetLastError();
		_loadError = true;
	} else {
		HDC hTmpDC = CreateCompatibleDC(b->hDC());
		HGDIOBJ o = SelectObject(hTmpDC, bitmap);
		int n = SetDIBitsToDevice(hTmpDC, 0, 0, _header->bmiHeader.biWidth,
								  _header->bmiHeader.biHeight, 0, 0, 0, 
								  _header->bmiHeader.biHeight,
								  data, _header, DIB_RGB_COLORS);
		if (n == 0) {
			i = GetLastError();
			_loadError = true;
		}
		SelectObject(hTmpDC, o);
		DeleteDC(hTmpDC);
	}
}
/*
	new: (b: Device, _header: pointer windows.BITMAPINFO, data: pointer [?] byte)
	{
		this.data = data
		this._header = header
		refresh(b)
	}
*/
Bitmap::Bitmap(Device* d, const string &filename) {
	_header = null;
	data = null;
	bitmap = null;
	_loadError = false;
	_headerAllocated = false;
	this->filename = filename;
	bitmap = (HBITMAP)LoadImage(null, filename.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (bitmap == null) {
		_loadError = true;
		return;
	}
	_header = (BITMAPINFO*)malloc(sizeof (BITMAPINFOHEADER));
	_headerAllocated = true;
	memset(_header, 0, sizeof (BITMAPINFOHEADER));
	_header->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	int i = GetDIBits(d->hDC(), bitmap, 0, 0, null, _header, DIB_RGB_COLORS);
	if (i == 0) {
		_loadError = true;
		return;
	}
}

Bitmap::Bitmap(Device* b, BITMAPINFO* header, byte* data) {
	int i, j;
	_header = header;
	this->data = data;
	_loadError = false;
	_headerAllocated = false;
	bitmap = CreateCompatibleBitmap(b->hDC(), header->bmiHeader.biWidth, header->bmiHeader.biHeight);
	if (bitmap == null)
		i = GetLastError();
	HDC hTmpDC = CreateCompatibleDC(b->hDC());
	HGDIOBJ o = SelectObject(hTmpDC, bitmap);
	int n = SetDIBitsToDevice(hTmpDC, 0, 0, header->bmiHeader.biWidth, 
							  header->bmiHeader.biHeight, 0, 0, 0, 
							  header->bmiHeader.biHeight,
							  data, header, DIB_RGB_COLORS);
	if (n == 0)
		j = GetLastError();
	SelectObject(hTmpDC, o);
	DeleteDC(hTmpDC);
}
/*
	new: (b: Device, header: pointer windows.BITMAPINFO, data: pointer [?] byte)
	{
		this.data = data
		this.header = header
		refresh(b)
	}
*/

Bitmap::~Bitmap() {
	if (_headerAllocated)
		free(_header);
}

void Bitmap::done() {
	if (bitmap != null){
		DeleteObject(bitmap);
		bitmap = null;
	}
}

int Bitmap::width() const {
	return _header->bmiHeader.biWidth;
}

int Bitmap::height() const {
	return _header->bmiHeader.biHeight;
}

Color::Color() {
	_hBrush = HBRUSH(COLOR_WINDOW + 1);
}

Color::Color(int wc, int dummy) {
	_hBrush = HBRUSH(wc + 1);
}

Color::Color(unsigned rgb) {
	_rgbVal = rgb;
	_hBrush = CreateSolidBrush(swapRedBlue(rgb));
}

Color::Color(int r, int g, int b) {
	_rgbVal = rgb(r, g, b);
	_hBrush = CreateSolidBrush(swapRedBlue(_rgbVal));
}

#if 0

	value: unsigned
		null =
			{
				return rgbVal
			}

	equal: (c: pointer Color) boolean
	{
		return false
	}
}
#endif
int rgb(int r, int g, int b) {
	return (r << 16) + (g << 8) + (b);
}

Color* createColor(unsigned rgb) {
	return new Color(rgb);
}

Color white(0xffffff);
Color black(0x000000);
Color red(0xff0000);
Color blue(0xff);
Color yellow(0xffff00);
Color green(0xff00);
/*
windowColor:			public instance Color(windows.COLOR_WINDOW, 0)
captionTextColor:		public instance Color(windows.COLOR_CAPTIONTEXT, 0)
windowTextColor:		public instance Color(windows.COLOR_WINDOWTEXT, 0)
 */
Color backgroundColor(COLOR_BACKGROUND, 0);
Color buttonHighlightColor(COLOR_BTNHIGHLIGHT, 0);
Color buttonShadowColor(COLOR_BTNSHADOW, 0);
Color buttonFaceColor(COLOR_BTNFACE, 0);
Color scrollbarColor(COLOR_3DLIGHT, 0);
Color editableColor(0xffffff);
Color disabledButtonText(0xa0a0a0);

MouseCursor* standardCursor(int kind) {
	HCURSOR hc = LoadCursor(null, LPCSTR(kind));
	return (MouseCursor*)hc;
}

int swapRedBlue(int x) {
	int r = (x & 0xff) << 16;
	int b = (x & 0xff0000) >> 16;
	int g = x & 0xff00;
	return r | g | b;
}

int dim(int color, int fade) {
	int r, g, b;
	int nc;

	r = color;
	g = color >> 8;
	b = color >> 16;
	r &= 0xff;
	g &= 0xff;
	b &= 0xff;
	r = ((r * fade) >> 8);
	g = ((g * fade) >> 8);
	b = ((b * fade) >> 8);
	nc = 0;
	nc |= r;
	nc |= g << 8;
	nc |= b << 16;
	return nc;
}

}  // namespace display
