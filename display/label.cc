#include "../common/platform.h"
#include "label.h"

#include <typeinfo.h>
#include "../common/machine.h"
#include "context_menu.h"
#include "device.h"
#include "window.h"

namespace display {

Label::Label(const string& text, const Font* font) {
	_value = text;
	_font = font;
	init();
}

Label::Label(const string& text, int preferredWidth, const Font* font) {
	_value = text;
	_font = font;
	init();
	_preferredWidth = preferredWidth;
}

Label::Label(int preferredWidth, const Font* font) {
	_font = font;
	init();
	_preferredWidth = preferredWidth;
}

void Label::init() {
	_textColor = null;
	_leftMargin = 0;
	_preferredWidth = 0;
	_format = 0;
	_cursor = 0;
	_selection = -1;
	_boundFont = null;
	_blink = null;
	_blinkOn = false;
	_showCursor = true;
}

Label::~Label() {
	beforeDelete.fire();
	if (_blink != null)
		_blink->kill(this);
}

dimension Label::measure() {
	Device* b = device();
	if (_font == null)
		_font = serifFont();
	if (_boundFont == null)
		_boundFont = _font->currentFont(b->owner());
	b->set_font(_boundFont);
	dimension sz;
	if (_preferredWidth == 0){
		string v = _value;
		if (v.size() == 0)
			v = "M";
		sz = b->textExtent(v);
		sz.width += _leftMargin;
	} else {
		sz = b->textExtent("M", 1);
		sz.width = coord(sz.width * _preferredWidth + _leftMargin);
	}
	return sz;
}

void Label::unbind() {
	_boundFont = null;
	jumble();
}

void Label::paint(Device* b) {
	preferredSize();
	b->set_font(_boundFont);
	if (_value.size()){
		int m = b->backMode(TRANSPARENT);
		int cv;
		if (_textColor != null)
			cv = _textColor->value();
		else if (_font->text())
			cv = _font->text()->value();
		else
			cv = 0;
		b->setTextColor(cv);
		rectangle r = bounds;
		r.topLeft.x += _leftMargin;
		if (_selection >= 0 && hasFocus()) {
			int s0, s1;

			if (_selection < _cursor) {
				s0 = _selection;
				s1 = _cursor;
			} else {
				s0 = _cursor;
				s1 = _selection;
			}
			if (s0 > 0) {
				dimension sz = b->textExtent(_value.c_str(), s0);
				r.opposite.x = r.topLeft.x + sz.width;
				b->textBox(r, _value.c_str(), s0, _format);
			} else
				r.opposite.x = r.topLeft.x;
			if (s0 < s1) {
				dimension sz = b->textExtent(_value.c_str() + s0, s1 - s0);
				r.topLeft.x = r.opposite.x;
				r.opposite.x += sz.width;
				b->fillRect(r, &black);
				b->setTextColor(0xffffff);
				b->textBox(r, _value.c_str() + s0, s1 - s0, _format);
			}
			if (s1 < _value.size()) {
				dimension sz = b->textExtent(_value.c_str() + s1, _value.size() - s1);
				r.topLeft.x = r.opposite.x;
				r.opposite.x += sz.width;
				b->setTextColor(cv);
				b->textBox(r, _value.c_str() + s1, _value.size() - s1, _format);
			}
		} else
			b->textBox(r, _value, _format);
		b->backMode(m);
	}
	if (_showCursor && _blinkOn){
		int n = getCursorPixel(b);

		b->set_pen(blackPen);
		b->line(n, bounds.topLeft.y, n, bounds.opposite.y);
	}
}

void Label::insert(char c) {
	deleteSelection();
	string s(&c, 1);
	int len = _value.size();
	if (_cursor == 0){
		if (len == 0)
			_value = s;
		else
			_value = s + _value;
	} else if (_cursor == len)
		_value = _value + s;
	else {
		string prefix = _value.substr(0, _cursor);
		string suffix = _value.substr(_cursor);
		_value = prefix + s + suffix;
	}
	_cursor++;
	appearanceChanged.fire();
	valueChanged.fire();
	invalidate();
}

void Label::deleteChars(int count) {
	if (count >= _value.size() - _cursor)
		_value = _value.substr(0, _cursor);
	else if (_cursor)
		_value = _value.substr(0, _cursor) + _value.substr(_cursor + count);
	else
		_value = _value.substr(count);
	appearanceChanged.fire();
	valueChanged.fire();
	invalidate();
}

void Label::deleteSelection() {
	if (_selection == -1)
		return;
	int s0, s1;
	if (_selection < _cursor) {
		s0 = _selection;
		s1 = _cursor;
	} else {
		s0 = _cursor;
		s1 = _selection;
	}
	if (s1 > s0) {
		_cursor = s0;
		deleteChars(s1 - s0);
	}
	_selection = -1;
}

int Label::getCursorPixel(Device* b) {
	int n = _leftMargin + bounds.topLeft.x;
	if (_cursor > 0){
		b->set_font(_boundFont);
		dimension d = b->textExtent(_value, 0, _cursor);
		n += d.width;
	}
	return n;
}

void Label::repaintCursor() {
	rectangle r = bounds;
	Device* d = device();
	if (d == null)
		return;
	int n = getCursorPixel(d);
	r.topLeft.x = coord(n - 1);
	r.opposite.x = coord(n + 1);
	invalidate(r);
}

int Label::startOfWord(int offset) const {
	if (offset >= _value.size() || isspace(_value[offset])) {
		if (isalnum(_value[offset - 1])) {
			while (offset && isalnum(_value[offset - 1]))
				offset--;
		} else if (isspace(_value[offset - 1])) {
			while (offset && isspace(_value[offset - 1]))
				offset--;
		} else
			return offset - 1;
	} else if (isalnum(_value[offset])) {
		while (offset && isalnum(_value[offset - 1]))
			offset--;
	}
	return offset;
}

int Label::endOfWord(int offset) const {
	if (offset == _value.size())
		return _value.size();
	else {
		if (isalnum(_value[offset])) {
			do
				offset++;
			while (offset < _value.size() &&
				   isalnum(_value[offset]));
		} else if (isspace(_value[offset])) {
			if (offset == 0 || isspace(_value[offset - 1])) {
				do
					offset++;
				while (offset < _value.size() &&
					   isspace(_value[offset]));
			}
		} else
			offset++;
		return offset;
	}
}

int Label::previousWord(int offset) const {
	if (offset == 0)
		return 0;
	else {
		offset--;
		while (offset && isspace(_value[offset]))
			offset--;
		if (offset && isalnum(_value[offset])) {
			while (offset && isalnum(_value[offset - 1]))
				offset--;
		}
		return offset;
	}
}

int Label::nextWord(int offset) const {
	if (offset == _value.size())
		return offset;
	else {
		if (isalnum(_value[offset])) {
			do
				offset++;
			while (offset < _value.size() &&
				   isalnum(_value[offset]));
		} else if (!isspace(_value[offset]))
			offset++;
		while (offset < _value.size() && isspace(_value[offset]))
			offset++;
		return offset;
	}
}

int Label::selectionStart() {
	if (_selection < _cursor)
		return _selection;
	else
		return _cursor;
}

int Label::selectionEnd() {
	if (_selection > _cursor)
		return _selection;
	else
		return _cursor;
}

void Label::clearSelection() {
	if (_selection >= 0) {
		_selection = -1;
		invalidate();
	}
}

void Label::startSelection(int location) {
	if (location < -1)
		location = -1;
	else if (location > _value.size())
		location = _value.size();
	_selection = location;
	invalidate();
}

void Label::extendSelection() {
	if (_selection == -1)
		_selection = _cursor;
}

void Label::selectAll() {
	_selection = 0;
	_cursor = _value.size();
	invalidate();
}

void Label::getKeyboardFocus() {
	RootCanvas* rc = rootCanvas();
	if (rc != null){
		_blink = startTimer(rc->blinkInterval);
		_blink->tick.addHandler(this, &Label::onTick);
		repaintCursor();
	}
	_blinkOn = true;
	selectAll();
	gotKeyboardFocus.fire();
}

void Label::loseKeyboardFocus() {
	if (_blink != null) {
		_blink->kill(null);
		_blink = null;
	}
	_blinkOn = false;
	repaintCursor();
	clearSelection();
	valueCommitted.fire();
	lostKeyboardFocus.fire();
}

void Label::onTick() {
	_blinkOn = !_blinkOn;
	repaintCursor();
}

int Label::locateCursorColumn(int rx) {
	int first = 0;
	int last = _value.size();
	Device* b = device();
	if (b == null)
		return first;
	b->set_font(_boundFont);
	int endCol = 0;
	while (first < last){
		int mid = (first + last) / 2;
		dimension d = b->textExtent(_value, 0, mid);
		endCol = d.width;
		if (endCol >= rx){
			last = mid;
			continue;
		}
		first = mid + 1;
		if (endCol == rx)
			break;
	}
	int firstCol = endCol;
	if (rx > endCol){
		if (first < _value.size()){
			dimension d = b->textExtent(_value, first, 1);
			endCol += d.width;
		}
	} else {
		if (first > 0){
			dimension d = b->textExtent(_value, first, 1);
			firstCol = (endCol - d.width);
		}
	}
	if ((rx << 1) < endCol + firstCol)
		first--;
	return first;
}

void Label::set_leftMargin(int lm) {
	if (lm != _leftMargin) {
		_leftMargin = lm;
		jumble();
		appearanceChanged.fire();
	}
}

void Label::set_value(const string& v) {
	if (_value != v) {
		_value = v;
		clearSelection();
		jumble();
		appearanceChanged.fire();
		valueChanged.fire();
	}
}

void Label::set_font(const Font* f) {
	if (_font != f) {
		_font = f;
		_boundFont = null;
		jumble();
		appearanceChanged.fire();
	}
}

void Label::set_format(unsigned f) {
	if (_format != f) {
		_format = f;
		appearanceChanged.fire();
		invalidate();
	}
}

void Label::set_textColor(Color* c) {
	if (_textColor == null ||c == null || _textColor->value() != c->value()) {
		_textColor = c;
		invalidate();
		appearanceChanged.fire();
	}
}

void Label::set_cursor(int c) {
	if (c < 0)
		c = 0;
	else if (c >= _value.size())
		c = _value.size();
	if (_cursor == c && _showCursor)
		return;
	repaintCursor();
	_cursor = c;
	_showCursor = true;
	appearanceChanged.fire();
	if (_selection >= 0)
		invalidate();
	else
		repaintCursor();
}

DropDown::DropDown(int value, const vector<string>& captions, display::Font* font)
: _captions(captions), Label(value >= 0 && value < captions.size() ? captions[value] : "", font) {
	_value = value;
}

dimension DropDown::measure() {
	Device* b = device();
	if (_font == null)
		_font = serifFont();
	if (_boundFont == null)
		_boundFont = _font->currentFont(b->owner());
	b->set_font(_boundFont);
	dimension sz;
	if (_preferredWidth == 0){
		for (int i = 0; i < _captions.size(); i++) {
			dimension captionSize = b->textExtent(_captions[i]);
			if (captionSize.width > sz.width)
				sz.width = captionSize.width;
			if (captionSize.height > sz.height)
				sz.height = captionSize.height;
		}
		if (sz.width == 0)
			sz = b->textExtent("M");
		sz.width += _leftMargin;
	} else {
		sz = b->textExtent("M", 1);
		sz.width = coord(sz.width * _preferredWidth + _leftMargin);
	}
	return sz;
}

void DropDown::set_value(int v) {
	if (_value != v) {
		_value = v;
		Label::set_value(_captions[v]);
	}
}

Field::Field(RootCanvas* root, Label *lbl) {
	_root = root;
	next = null;
	_label = lbl;
	_beforeDeleteHandler = _label->beforeDelete.addHandler(this, &Field::onBeforeDelete);
	if (typeid(*lbl) == typeid(DropDown)) {
		_characterHandler = _label->character.addHandler(this, &Field::onDropDownCharacter);
		_functionKeyHandler = _label->functionKey.addHandler(this, &Field::onDropDownFunctionKey);
		_clickHandler = null;
		_buttonDownHandler = _label->buttonDown.addHandler(this, &Field::onDropDownButtonDown);
		_doubleClickHandler = null;
		_startDragHandler = null;
		_dragHandler = null;
		_dropHandler = null;
	} else {
		_characterHandler = _label->character.addHandler(_label, &Label::insert);
		_functionKeyHandler = _label->functionKey.addHandler(this, &Field::onFunctionKey);
		_clickHandler = _label->click.addHandler(this, &Field::onClick);
		_buttonDownHandler = _label->buttonDown.addHandler(this, &Field::onButtonDown);
		_doubleClickHandler = _label->doubleClick.addHandler(this, &Field::onDoubleClick);
		_startDragHandler = _label->startDrag.addHandler(this, &Field::onStartDrag);
		_dragHandler = _label->drag.addHandler(this, &Field::onDrag);
		_dropHandler = _label->drop.addHandler(this, &Field::onDrop);
	}
}

Field::~Field() {
	if (_label) {
		_label->character.removeHandler(_characterHandler);
		_label->functionKey.removeHandler(_functionKeyHandler);
		_label->click.removeHandler(_clickHandler);
		_label->buttonDown.removeHandler(_buttonDownHandler);
		_label->doubleClick.removeHandler(_doubleClickHandler);
		_label->startDrag.removeHandler(_startDragHandler);
		_label->drag.removeHandler(_dragHandler);
		_label->drop.removeHandler(_dropHandler);
		_label->beforeDelete.removeHandler(_beforeDeleteHandler);
		if (_root)
			_root->removeFields(_label);
	}
}

void Field::removed() {
	_root = null;
}

void Field::onBeforeDelete() {
	if (_root) {
		if (!_root->removeFields(_label))		// if this didn't delete me, mark me up so I don't try again
			_label = null;						// Our label is going away, what happened to our field?
	}
}

void Field::onClick(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		if (_label->hasFocus()) {
			_label->set_cursor(_label->locateCursorColumn(p.x - _label->bounds.topLeft.x));
			_label->clearSelection();
		} else {
			_label->set_showCursor(true);
			_label->rootCanvas()->setKeyboardFocus(_label);
		}
	}
}

void Field::onButtonDown(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		_label->clearSelection();
		_label->set_showCursor(false);
	}
}

void Field::onDoubleClick(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		int startOfWord = _label->startOfWord(_label->cursor());
		int endOfWord = _label->endOfWord(_label->cursor());
		// BUGBUG: issuing this warningMessage cascades into a funky state where the
		// TextCanvas is left in drag mode.  It shouldn't do that.
		//warningMessage("Double");
		_label->startSelection(startOfWord);
		_label->set_cursor(endOfWord);
	}
}

void Field::onStartDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		int desiredPosition = _label->locateCursorColumn(p.x - _label->bounds.topLeft.x);
		_label->set_cursor(desiredPosition);
		_label->startSelection(desiredPosition);
	}
}

void Field::onDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		int desiredPosition = _label->locateCursorColumn(p.x - _label->bounds.topLeft.x);
		_label->extendSelection();
		_label->set_cursor(desiredPosition);
	}
}

void Field::onDrop(MouseKeys mKeys, point p, Canvas* target) {
	onDrag(mKeys, p, target);
}

void Field::onFunctionKey(FunctionKey fk, ShiftState ss) {
	Field* f;
	switch (fk){
	case	FK_END:
		if (ss & SS_CONTROL)
			break;
		if (ss == SS_SHIFT)
			_label->extendSelection();
		else
			_label->clearSelection();
		_label->set_cursor(_label->value().size());
		break;

	case	FK_HOME:
		if (ss & SS_CONTROL)
			break;
		if (ss == SS_SHIFT)
			_label->extendSelection();
		else
			_label->clearSelection();
		_label->set_cursor(0);
		break;

	case	FK_LEFT:
		if (ss & SS_SHIFT)
			_label->extendSelection();
		else
			_label->clearSelection();
		if (ss & SS_CONTROL)
			_label->set_cursor(_label->previousWord(_label->cursor()));
		else
			_label->set_cursor(_label->cursor() - 1);
		break;

	case	FK_RIGHT:
		if (ss & SS_SHIFT)
			_label->extendSelection();
		else
			_label->clearSelection();
		if (ss & SS_CONTROL)
			_label->set_cursor(_label->nextWord(_label->cursor()));
		else
			_label->set_cursor(_label->cursor() + 1);
		break;

	case	FK_ESCAPE:
		_label->set_cursor(0);
		_label->set_value("");
		break;

	case	FK_BACK:
		if (_label->hasSelection()) {
			_label->deleteSelection();
			break;
		}
		// Backspace at start of buffer is a no-op.
		if (_label->cursor() == 0)
			break;
		_label->set_cursor(_label->cursor() - 1);
		_label->deleteChars(1);
		break;

	case	FK_DELETE:
		if (_label->hasSelection()) {
			_label->deleteSelection();
			break;
		}
		_label->deleteChars(1);
		break;

	case	FK_TAB:
		if (ss & SS_SHIFT)
			f = preceding();
		else
			f = succeeding();
		if (f != null)
			_label->rootCanvas()->setKeyboardFocus(f->_label);
		break;

	case	FK_RETURN:
		if (ss == 0) {
			carriageReturn.fire();
			if (_label->rootCanvas()->isDialog()) {
				Dialog* d = (Dialog*)_label->rootCanvas();
				d->launchOk(_label->bounds.topLeft, _label);
			}
		}
		break;

	case	FK_A:
		if (ss == SS_CONTROL)
			_label->selectAll();
		break;

	case	FK_C:
		if (ss == SS_CONTROL)
			copySelection();
		break;

	case	FK_V:
		if (ss == SS_CONTROL)
			pasteCutBuffer();
		break;

	case	FK_X:
		if (ss == SS_CONTROL)
			cutSelection();
		break;
	}
}

void Field::onDropDownButtonDown(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		_label->clearSelection();
		_label->set_showCursor(false);
		exposeDropDown();
	}
}

void Field::onDropDownCharacter(char c) {
}

void Field::onDropDownFunctionKey(FunctionKey fk, ShiftState ss) {
	Field* f;
	switch (fk){
/*
	case	FK_END:
		if (ss)
			break;
		_label->set_cursor(_label->value().size());
		break;

	case	FK_HOME:
		if (ss)
			break;
		_label->set_cursor(0);
		break;

	case	FK_LEFT:
	case	FK_UP:
		if (ss)
			break;
		break;

	case	FK_RIGHT:
	case	FK_DOWN:
		if (ss)
			break;
		break;
 */
	case	FK_TAB:
		if (ss & SS_SHIFT)
			f = preceding();
		else
			f = succeeding();
		if (f != null)
			_label->rootCanvas()->setKeyboardFocus(f->_label);
		break;

	case	FK_RETURN:
		if (ss)
			break;
		carriageReturn.fire();
		if (_label->rootCanvas()->isDialog()) {
			Dialog* d = (Dialog*)_label->rootCanvas();
			d->launchOk(_label->bounds.topLeft, _label);
		}
		break;
/*
	case	FK_A:
		if (ss == SS_CONTROL)
			_label->selectAll();
		break;

	case	FK_C:
		if (ss == SS_CONTROL)
			copySelection();
		break;

	case	FK_V:
		if (ss == SS_CONTROL)
			pasteCutBuffer();
		break;
 */
	}
}

Field* Field::preceding() {
	Field* here = this;
	Field* lastVisible = null;
	Field* f = _label->rootCanvas()->fields();
	if (here == f)
		here = null;
	for (;;) {
		if (f->_label->rootCanvas() != null)
			lastVisible = f;
		if (f->next == here){
			if (lastVisible)
				return lastVisible;
			else if (here)
				here = null;
			else
				return null;
		}
		f = f->next;
	}
}

Field* Field::succeeding() {
	for (Field* f = next; f != this; f = f->next){
		if (f == null)
			f = _label->rootCanvas()->fields();
		if (f->_label->rootCanvas() != null)
			return f;
	}
	return null;
}

void Field::cutSelection() {
	copySelection();
	_label->deleteSelection();
}

void Field::copySelection() {
	int loc1 = _label->selectionStart();
	int loc2 = _label->selectionEnd();
	if (loc1 == loc2)
		return;
	string text = _label->value().substr(loc1, loc2 - loc1);
	Clipboard c(_label->rootCanvas());
	c.write(text);
}

void Field::pasteCutBuffer() {
	Clipboard c(_label->rootCanvas());
	string text;

	if (c.read(&text)) {
		if (_label->hasSelection())
			_label->deleteSelection();
		for (int i = 0; i < text.size(); i++) {
			if (text[i] != '\n')
				_label->insert(text[i]);
		}
	}
}

void Field::exposeDropDown() {
	point anchor;

	anchor.x = _label->bounds.topLeft.x;
	anchor.y = _label->bounds.opposite.y;
	ContextMenu* m = new ContextMenu(_label->rootCanvas(), anchor, _label);
	DropDown* d = (DropDown*)_label;
	for (int i = 0; i < d->captions().size(); i++)
		m->choice(d->captions()[i])->click.addHandler(this, &Field::selectDropDownValue, i);
	m->show();
}

void Field::selectDropDownValue(point p, Canvas* target, int value) {
	DropDown* d = (DropDown*)_label;
	d->set_value(value);
}

}  // namespace display