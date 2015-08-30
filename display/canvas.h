#pragma once
#include <windows.h>
#include "../common/data.h"
#include "../common/event.h"
#include "../common/string.h"
#include "measurement.h"

namespace display {

class Background;
class Bitmap;
class BoundFont;
class Canvas;
class Color;
class Device;
class Font;
class FontMetrics;
class Grid;
class MouseCursor;
class OutlineItem;
class OutlineViewport;
class RadioHandler;
class RootCanvas;
class ScrollBarHandler;
class TrackBarHandler;

typedef WORD MouseKeys;

extern const char* SYMBOL_UP_TRIANGLE;
extern const char* SYMBOL_DOWN_TRIANGLE;
extern const char* SYMBOL_LEFT_TRIANGLE;
extern const char* SYMBOL_RIGHT_TRIANGLE;

enum MouseButtonState {
	MBS_UP,
	MBS_DOWN,
	MBS_DRAGGING,
	MBS_DOWN_DOUBLE,
};

const int SHORT_DRAG = 4;			// Any drag that is very small will not be very helpful and may just be the
									// result of a twitch.

enum FunctionKey {
	FK_UNUSED,
	FK_LBUTTON,
	FK_RBUTTON,
	FK_CANCEL,
	FK_MBUTTON,

	FK_BACK = 0x08,
	FK_TAB,

	FK_CLEAR = 0x0c,
	FK_RETURN,

	FK_SHIFT = 0x10,
	FK_CONTROL,
	FK_MENU,
	FK_PAUSE,
	FK_CAPITAL,

	FK_KANA,
	FK_HANGUL = 0x15,
	FK_JUNJA = 0x17,
	FK_FINAL,
	FK_HANJA,
	FK_KANJI = 0x19,

	FK_ESCAPE = 0x1b,

	FK_CONVERT,
	FK_NONCONVERT,
	FK_ACCEPT,
	FK_MODECHANGE,

	FK_SPACE,
	FK_PRIOR,
	FK_NEXT,
	FK_END,
	FK_HOME,
	FK_LEFT,
	FK_UP,
	FK_RIGHT,
	FK_DOWN,
	FK_SELECT,
	FK_PRINT,
	FK_EXECUTE,
	FK_SNAPSHOT,
	FK_INSERT,
	FK_DELETE,
	FK_HELP,

/* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
/* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */

	FK_A = 0x41,
	FK_B,
	FK_C,
	FK_D,
	FK_E,
	FK_F,
	FK_G,
	FK_H,
	FK_I,
	FK_J,
	FK_K,
	FK_L,
	FK_M,
	FK_N,
	FK_O,
	FK_P,
	FK_Q,
	FK_R,
	FK_S,
	FK_T,
	FK_U,
	FK_V,
	FK_W,
	FK_X,
	FK_Y,
	FK_Z,

	FK_LWIN = 0x5b,
	FK_RWIN,
	FK_APPS,

	FK_NUMPAD0 = 0x60,
	FK_NUMPAD1,
	FK_NUMPAD2,
	FK_NUMPAD3,
	FK_NUMPAD4,
	FK_NUMPAD5,
	FK_NUMPAD6,
	FK_NUMPAD7,
	FK_NUMPAD8,
	FK_NUMPAD9,
	FK_MULTIPLY,
	FK_ADD,
	FK_SEPARATOR,
	FK_SUBTRACT,
	FK_DECIMAL,
	FK_DIVIDE,

	FK_F1,
	FK_F2,
	FK_F3,
	FK_F4,
	FK_F5,
	FK_F6,
	FK_F7,
	FK_F8,
	FK_F9,
	FK_F10,
	FK_F11,
	FK_F12,
	FK_F13,
	FK_F14,
	FK_F15,
	FK_F16,
	FK_F17,
	FK_F18,
	FK_F19,
	FK_F20,
	FK_F21,
	FK_F22,
	FK_F23,
	FK_F24,

	FK_NUMLOCK = 0x90,
	FK_SCROLL,

/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
	FK_LSHIFT = 0xa0,
	FK_RSHIFT,
	FK_LCONTROL,
	FK_RCONTROL,
	FK_LMENU,
	FK_RMENU,

	FK_PROCESSKEY = 0xe5,

	FK_ATTN = 0xf6,
	FK_CRSEL,
	FK_EXSEL,
	FK_EREOF,
	FK_PLAY,
	FK_ZOOM,
	FK_NONAME,
	FK_PA1,
	FK_OEM_CLEAR
};

typedef unsigned char ShiftState;

enum ShiftStateValues {
	SS_NONE = 0x00,
	SS_ALT = 0x01,
	SS_CONTROL = 0x02,
	SS_SHIFT =0x04
};

class Timer {
public:
	Timer(RootCanvas* root) {
		_root = root;
	}

	bool start(int elapsed);

	void kill(const Canvas* keyFocus);

	void fire();

	Event tick;

	RootCanvas* root() const { return _root; }

private:
	void release();

	RootCanvas* _root;
};

class Canvas {
public:
	Canvas();

	virtual ~Canvas();

	// ------------------------------ No-op default methods
	// These methods are virtual on Canvas with default empty
	// implementations.  Any Canvas must decide whether it must
	// override these with their own functionality.

	virtual dimension measure() = 0;

	virtual void paint(Device* b);

	virtual void getKeyboardFocus();

	virtual void loseKeyboardFocus();

	// ------------------------------- Overrideable default methods
	// These methods can be overridden. If your Canvas can have 
	// any children, layout is usually overridden to handle
	// layout of the child Canvases.  The append method default 
	// implementation can probably be used without overriding except
	// in unusual circumstances.  TODO: remove those circumstances.
	// unbind should be overridden when a canvas binds fonts or other
	// related resources and/or device info.

	virtual void layout(dimension size);

	virtual void childJumbled(Canvas* child);

	virtual void append(Canvas* c);

	virtual bool bufferedDisplay(Device* device);

	virtual void unbind();

	// ------------------------------- RootCanvas proxy methods ---------
	// These methods are virtual on Canvas because they delegate to their
	// parent Canvas.  Any RootCanvas will supply the missing functionality
	// and in each case, if the Canvas is not rooted at a RootCanvas, some
	// default behavior is implemented (noted in each case).  In general,
	// you should not override these methods.

	/*
	 *	isBindable
	 *
	 *	This method returns true if the current canvas is connected to a
	 *	RootCanvas through it's parent chain.  When attached to a RootCanvas
	 *	a Canvas can obtain a Device object and can bind Fonts, which will
	 *	allow you to calculate the size of text.
	 */
	virtual bool isBindable();

	virtual Device* device();

	virtual RootCanvas* rootCanvas();

	virtual void invalidate(const rectangle& r);

	virtual Timer* startTimer(unsigned elapsed);

	// ------------------------- end of RootCanvas proxy methods ---------

	void at(point p);

	void resize(dimension d);

	void unbindAll();			// Called to cause all children to unbind

	void arrange();

	void jumble();

	void jumbleContents();

	bool measured();
	/*
	 *	under
	 *
	 *	Returns true if this canvas is under the given canvas c.
	 */
	bool under(Canvas* c);

	dimension preferredSize();

	void invalidate();

	MouseCursor* activeMouseCursor();

	coord height() const { return bounds.height(); }

	void prune();

	dimension estimateMeasure(Canvas* c);

	void setBackground(Background* b);

	Canvas* hitTest(point p);

	// ------------------------------- mouse event routing methods ---------

	void doButtonDown(MouseKeys mKeys, point p);

	void doClick(MouseKeys mKeys, point p);

	MouseButtonState doDoubleClick(MouseKeys mKeys, point p);

	void doMouseWheel(MouseKeys mKeys, short zDelta, point p);

	void doStartDrag(MouseKeys mKeys, point p);

	void doDrag(MouseKeys mKeys, point p, Canvas* target);

	void doDrop(MouseKeys mKeys, point p, Canvas* target);

	void doMouseMove(point p);

	void doOpenContextMenu(point p);

	void doMouseHover(point p);

	bool arranged() const { return _arranged; }

	bool jumbled() const { return !_arranged; }

	Background* background() const { return _background; }

	Canvas*					parent;
	Canvas*					child;
	Canvas*					sibling;
	rectangle				bounds;
	bool					disableDoubleClick;
	MouseCursor*			mouseCursor;

		// Input events

	Event										beforeShow;
	Event										beforeHide;
	Event3<MouseKeys, point, Canvas*>			buttonDown;
	Event3<MouseKeys, point, Canvas*>			click;
	Event3<MouseKeys, point, Canvas*>			doubleClick;
	Event3<MouseKeys, point, Canvas*>			startDrag;
	Event3<MouseKeys, point, Canvas*>			drag;
	Event3<MouseKeys, point, Canvas*>			drop;
	Event4<MouseKeys, point, int, Canvas*>		mouseWheel;
	Event										settingChange;
	Event2<point, Canvas*>						mouseMove;
	Event2<point, Canvas*>						mouseHover;
	Event2<point, Canvas*>						openContextMenu;
	Event1<char>								character;
	Event2<FunctionKey, ShiftState>				functionKey;

private:
	void unmeasure();

	dimension				_preferredSize;
	bool					_arranged;
	bool					_complained;
	Background*				_background;
};

class Spacer : public Canvas {
	typedef Canvas super;
public:
	Spacer(int left, int top, int right, int bottom, Canvas* insert = null);

	Spacer(int margin, Canvas* insert = null);

	virtual dimension measure();

	virtual void layout(dimension size);
private:
	void init(int left, int top, int right, int bottom);

	int left, top, right, bottom;
};

class Image : public Canvas {
public:
	Image(const string& filename);

	~Image();

	virtual dimension measure();

	virtual void paint(Device* b);

private:
	string				_filename;
	Bitmap*				_bitmap;
};
/*
 * EdgeCanvas
 *
 * An EdgeCanvas is used in constructing nested features.  There is
 * assumed to be a single child canvas and that it's origin is the same
 * as the innerOrigin of the enclosing edge.
 */
class EdgeCanvas : public Canvas {
public:
	virtual point innerOrigin();
};

class Bevel : public EdgeCanvas {
	typedef EdgeCanvas super;
public:
	Bevel(int width, Canvas* c = null);

	Bevel(int width, bool depressed, Canvas* c = null);

	virtual dimension measure();

	virtual point innerOrigin();

	virtual void paint(Device* b);

	virtual void layout(dimension size);

	bool depressed() const { return _depressed; }

	void set_depressed(bool d);

	Color*				frontEdgeColor;
	Color*				backEdgeColor;

private:
	void init(int width);

	coord				edgeWidth;
	Color*				_frontEdgeColor;
	Color*				_backEdgeColor;
	bool				_depressed;
};

class Border : public EdgeCanvas {
	typedef EdgeCanvas super;
public:
	Border(int width, Canvas* c = null);

	Border(int left, int top, int right, int bottom, Canvas* c = null);

	virtual dimension measure();

	virtual point innerOrigin();

	virtual void paint(Device* b);

	virtual void layout(dimension size);

	Color*				color;

private:
	void init(int left, int top, int right, int bottom);

	coord				_left;
	coord				_top;
	coord				_right;
	coord				_bottom;
};

class Filler : public EdgeCanvas {
	typedef EdgeCanvas super;
public:
	Filler(Canvas* c);

	virtual dimension measure();

	virtual point innerOrigin();

	virtual void layout(dimension size);

private:
};

class ButtonHandler {
public:
	ButtonHandler(Bevel* b, int ri);

	~ButtonHandler();

	int repeatInterval() const { return _repeatInterval; }
	void set_repeatInterval(int ri);

	Bevel* button() const { return _button; }

	Event1<Bevel*>		click;
private:
	void init();

	void pushButton(MouseKeys mKeys, point p, Canvas* target);

	void startTimer();

	void stopTimer();

	void onTick();

	void onClick(MouseKeys mKeys, point p, Canvas* target);

	void onDrag(MouseKeys mKeys, point p, Canvas* target);

	void onDrop(MouseKeys mKeys, point p, Canvas* target);

	Timer*				_repeat;
	Bevel*				_button;
	int					_repeatInterval;		// button repeat rate in msec
	bool				_latched;
	bool				_repeated;
};


enum MouseEvent {
	ME_DOWN,
	ME_DRAG,
	ME_DROP,
	ME_CLICK,
	ME_DOUBLE_CLICK,
	ME_WHEEL,
};

RootCanvas* getRootCanvas(HWND handle);

void setRootCanvas(HWND handle, RootCanvas* rc);

void doPaint(HWND handle, HDC hdc, const rectangle& paintRect);
void doSize(HWND handle, dimension d);
/*
 *	mouseEvent
 *
 *	Called from the window proc, this code routes most incoming mouse events to
 *	the correct RootCanvas, which then does further processing.
 */
void mouseEvent(HWND handle, unsigned uMsg, MouseKeys fwKeys, short zDelta, point p);
void activateWindow(HWND handle, short fActive, bool fMinimized);
void keyDownEvent(HWND handle, int virtualKey, unsigned keyData);
void keyUpEvent(HWND handle, int virtualKey, unsigned keyData);
void setFocusEvent(HWND handle, HWND loseHandle);
void killFocusEvent(HWND handle, HWND getHandle);
void closeWindow(HWND handle);
void deleteRootCanvas(HWND handle);
void runInRootCanvas(HWND handle, LPARAM lParam);
void timerEvent(HWND handle, unsigned id);

const int TOGGLE_SIZE = 8;
const int RADIO_SIZE = 8;

class Toggle : public Canvas {
public:
	data::Boolean*		state;

	Toggle(data::Boolean* iValue);

	virtual dimension measure();

	virtual void paint(Device* b);
};

class ToggleHandler {
public:
	ToggleHandler(Toggle* t);
private:
	void onClick(MouseKeys mKeys, point p, Canvas* target);

	void onChange();

	Toggle*		_toggle;
};

class RadioButton : public Canvas {
	friend RadioHandler;
public:
	data::Integer*	state;
	int				setting;

	RadioButton(data::Integer* iValue, int s);

	virtual dimension measure();

	virtual void paint(Device* b);
private:
	RadioButton*	next;
};

class RadioHandler {
public:
	RadioHandler(data::Integer* i);

	void member(RadioButton* r);

private:
	void onClick(MouseKeys mKeys, point p, Canvas* target);

	void onChanged(data::Integer* i);

	RadioButton*		_currentRadio;
	RadioButton*		_radios;
};

Bitmap* loadBitmap(Device* d, const string& filename);

}  // namespace display
