#pragma once
#include "../common/event.h"
#include "../common/string.h"
#include "../common/vector.h"
#include "canvas.h"
#include "measurement.h"

namespace display {

class BoundFont;
class Color;
class Device;
class Font;
class RootCanvas;

class Label : public Canvas {
public:
	Label(const string& text, const Font* font = null);

	Label(const string& text, int preferredWidth, const Font* font = null);

	Label(int preferredWidth, const Font* font = null);

	~Label();

	virtual dimension measure();

	virtual void unbind();

	virtual void paint(Device* b);

	int getCursorPixel(Device* b);

	int locateCursorColumn(int rx);

	void insert(char c);

	void deleteChars(int count);

	void deleteSelection();

	void repaintCursor();

	int startOfWord(int location) const;

	int endOfWord(int location) const;

	int previousWord(int location) const;

	int nextWord(int location) const;

	bool hasSelection() const { return _selection >= 0; }

	int selectionStart();

	int selectionEnd();

	void clearSelection();

	void startSelection(int location);

	void extendSelection();

	void selectAll();

	virtual void getKeyboardFocus();

	virtual void loseKeyboardFocus();

	Event			appearanceChanged;
	Event			valueChanged;
	Event			valueCommitted;
	Event			gotKeyboardFocus;
	Event			lostKeyboardFocus;
	Event			beforeDelete;

	int leftMargin() const { return _leftMargin; }

	void set_leftMargin(int lm);

	const string& value() const { return _value; }

	void set_value(const string& s);

	const Font* font() const { return _font; }

	void set_font(const Font* f);

	unsigned format() const { return _format; }

	void set_format(unsigned format);

	Color* textColor() { return _textColor; }

	void set_textColor(Color* c);

	int cursor() const { return _cursor; }

	void set_cursor(int c);

	bool hasFocus() const { return _blink != null; }

	bool showCursor() const { return _showCursor; }

	void set_showCursor(bool v) { _showCursor = v; }

protected:
	string			_value;
	const Font*		_font;
	BoundFont*		_boundFont;
	int				_preferredWidth;
	int				_leftMargin;
	Color*			_textColor;
	unsigned		_format;
	bool			_showCursor;
	int				_cursor;
	int				_selection;

private:
	void init();

	void onTick();

	Timer*			_blink;
	bool			_blinkOn;
};

class DropDown : public Label {
public:
	DropDown(int value, const vector<string>& captions, Font* font = null);

	virtual dimension measure();

	void set_value(int value);

	int value() const { return _value; }

	const vector<string>& captions() const { return _captions; }

private:
	int						_value;
	const vector<string>&	_captions;
};

class Field {
public:
	Field(RootCanvas* root, Label* lbl);

	~Field();

	void removed();

	void onButtonDown(MouseKeys mKeys, point p, Canvas* target);

	void onClick(MouseKeys mKeys, point p, Canvas* target);

	void onDoubleClick(MouseKeys mKeys, point p, Canvas* target);

	void onStartDrag(MouseKeys mKeys, point p, Canvas* target);

	void onDrag(MouseKeys mKeys, point p, Canvas* target);

	void onDrop(MouseKeys mKeys, point p, Canvas* target);

	void onFunctionKey(FunctionKey fk, ShiftState ss);

	void onDropDownButtonDown(MouseKeys mKeys, point p, Canvas* target);

	void onDropDownClick(MouseKeys mKeys, point p, Canvas* target);

	void onDropDownStartDrag(MouseKeys mKeys, point p, Canvas* target);

	void onDropDownDrag(MouseKeys mKeys, point p, Canvas* target);

	void onDropDownDrop(MouseKeys mKeys, point p, Canvas* target);

	void onDropDownCharacter(char c);

	void onDropDownFunctionKey(FunctionKey fk, ShiftState ss);

	void onBeforeDelete();

	Field* preceding();

	Field* succeeding();

	Event carriageReturn;

	Field*			next;

	Label* label() const { return _label; }

private:
	void cutSelection();

	void copySelection();

	void pasteCutBuffer();

	void exposeDropDown();

	void selectDropDownValue(point p, Canvas* target, int value);

	RootCanvas*		_root;
	Label*			_label;
	void*			_characterHandler;
	void*			_functionKeyHandler;
	void*			_clickHandler;
	void*			_buttonDownHandler;
	void*			_doubleClickHandler;
	void*			_startDragHandler;
	void*			_dragHandler;
	void*			_dropHandler;
	void*			_beforeDeleteHandler;
};

}  // namespace display
