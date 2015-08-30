#pragma once
#include "../display/canvas.h"

namespace display {

class DropDown;
class Field;
class Font;
class Label;

class FontSelector : public Canvas {
public:
	FontSelector(Font* f);

	~FontSelector();

	virtual dimension measure();

	Field* firstField();

	Event changed;

private:
	void onFamilyChanged();

	void onSizeChanged();

	void onToggleChanged();

	Font*			_font;
	DropDown*		_familyList;
	Label*			_size;
	Toggle*			_bold;
	Toggle*			_italic;
	Toggle*			_underlined;
	Label*			_sample;
	Field*			_firstField;
	ToggleHandler*	_boldHandler;
	ToggleHandler*	_italicHandler;
	ToggleHandler*	_underlinedHandler;
};

}
