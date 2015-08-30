#include "../common/platform.h"
#include "canvas.h"

#include <windows.h>
#include "../common/machine.h"
#include "device.h"
#include "window.h"

namespace display {

const char* SYMBOL_UP_TRIANGLE = "5";
const char* SYMBOL_DOWN_TRIANGLE = "6";
const char* SYMBOL_LEFT_TRIANGLE = "3";
const char* SYMBOL_RIGHT_TRIANGLE = "4";

const dimension NO_SIZE(-1, -1);

bool Timer::start(int elapsed) {
	return SetTimer(_root->handle(), unsigned(this), elapsed, NULL) != FALSE;
}

void Timer::fire() {
	if (_root != null)
		tick.fire();
	else {
		char buffer[256];

		sprintf(buffer, "%p: fire()\n", this);
		debugPrint(buffer);
	}
}

void Timer::kill(const Canvas* keyFocus) {
	if (_root != null) {
		RootCanvas* root = _root;
		_root = null;
		KillTimer(root->handle(), unsigned(this));
		if (root->keyFocus() == keyFocus)
			root->setKeyboardFocus(null);
		root->run(this, &Timer::release);
	}
}

void Timer::release() {
	delete this;
}

Canvas::Canvas() {
	parent = null;
	child = null;
	sibling = null;
	disableDoubleClick = false;
	mouseCursor = null;
	_preferredSize = NO_SIZE;
	_arranged = false;
	_complained = false;
	_background = null;
}

Canvas::~Canvas() {
	while (child) {
		Canvas* c = child;
		c->prune();
		delete c;
	}
}

void Canvas::paint(Device* b) {
}

void Canvas::getKeyboardFocus() {
}

void Canvas::loseKeyboardFocus() {
}

Timer* Canvas::startTimer(unsigned elapsed) {
	if (parent != null)
		return parent->startTimer(elapsed);
	else
		return null;
}

MouseCursor* Canvas::activeMouseCursor() {
	Canvas* c = this;
	do {
		if (c->mouseCursor != null)
			return c->mouseCursor;
		c = c->parent;
	} while (c != null);
	return null;
}

bool Canvas::isBindable() {
	if (parent != null)
		return parent->isBindable();
	else
		return false;
}

Device* Canvas::device() {
	if (parent == null)
		return null;
	else
		return parent->device();
}

void Canvas::invalidate(const rectangle& r) {
	if (parent != null)
		parent->invalidate(r);
}

RootCanvas* Canvas::rootCanvas() {
	if (parent)
		return parent->rootCanvas();
	else
		return null;
}

void Canvas::prune() {
	if (parent == null)
		return;
	RootCanvas* root = parent->rootCanvas();
	if (root)
		root->purgeCaches(this);
	parent->jumble();
	if (parent->child == this)
		parent->child = sibling;
	else {
		for (Canvas* ch = parent->child; ch->sibling != null; ch = ch->sibling)
			if (ch->sibling == this){
				ch->sibling = sibling;
				break;
			}
	}
	sibling = null;
	parent = null;
	unmeasure();
}

void Canvas::append(Canvas* c) {
	c->parent = this;
	c->sibling = null;
	if (child == null)
		child = c;
	else {
		Canvas* cc;
		for (cc = child; cc->sibling != null; cc = cc->sibling)
			;
		cc->sibling = c;
	}
	jumble();
	if (isBindable())
		beforeShowTree(c);
}

bool Canvas::bufferedDisplay(Device* device) {
	return false;
}

dimension Canvas::estimateMeasure(Canvas* c) {
	if (isBindable()) {
		Canvas* psv = c->parent;
		c->parent = this;
		dimension size = c->preferredSize();
		c->parent = psv;
		return size;
	}
	return NO_SIZE;
}

void Canvas::layout(dimension size) {
	if (child) {
		child->at(bounds.topLeft);
		child->resize(size);
	}
}

void Canvas::childJumbled(Canvas* child) {
	jumble();
}

void Canvas::unbind() {
}

void Canvas::resize(dimension sz) {
	dimension oldSize = bounds.size();
	if (sz.width != oldSize.width || sz.height != oldSize.height) {
		bounds.set_size(sz);
		jumble();
	}
}

void Canvas::unbindAll() {
	unbind();
	for (Canvas* c = child; c != null; c = c->sibling)
		c->unbindAll();
}

void Canvas::arrange() {
	if (!_arranged) {
		layout(bounds.size());
		_arranged = true;
		_complained = false;
	}
}

void Canvas::jumble() {
	jumbleContents();
	if (!_complained) {
		_complained = true;
		if (parent)
			parent->childJumbled(this);
	}
}

void Canvas::jumbleContents() {
	if (_arranged) {
		_arranged = false;
		invalidate();
		unmeasure();
	}
}

bool Canvas::measured() {
	return _preferredSize.hasSize();
}

dimension Canvas::preferredSize() {
	if (!_preferredSize.hasSize() && isBindable())
		_preferredSize = measure();
	return _preferredSize;
}

void Canvas::at(point p) {
	dimension sz = bounds.size();
	bounds.topLeft = p;
	bounds.set_size(sz);
	jumbleContents();
}

void Canvas::setBackground(Background* b) {
	if (b != _background) {
		_background = b;
		invalidate();
	}
}

Canvas* Canvas::hitTest(point p) {
	Canvas* c;
	if (sibling) {
		c = sibling->hitTest(p);
		if (c)
			return c;
	}
	if (!bounds.contains(p))
		return null;
	if (child) {
		c = child->hitTest(p);
		if (c)
			return c;
	}
	return this;
}

void Canvas::doButtonDown(MouseKeys mKeys, point p) {
	if (buttonDown.has_listeners())
		buttonDown.fire(mKeys, p, this);
	else if (parent != null)
		parent->doButtonDown(mKeys, p);
 }

void Canvas::doClick(MouseKeys mKeys, point p) {
	if (click.has_listeners())
		click.fire(mKeys, p, this);
	else if (parent != null)
		parent->doClick(mKeys, p);
}

MouseButtonState Canvas::doDoubleClick(MouseKeys mKeys, point p) {
	if (disableDoubleClick){
		doButtonDown(mKeys, p);
		return MBS_DOWN;
	}
	if (doubleClick.has_listeners())
		doubleClick.fire(mKeys, p, this);
	else if (parent != null)
		return parent->doDoubleClick(mKeys, p);
	return MBS_DOWN_DOUBLE;
}

void Canvas::doMouseWheel(MouseKeys mKeys, short zDelta, point p) {
	if (mouseWheel.has_listeners())
		mouseWheel.fire(mKeys, p, zDelta, this);
	else if (parent != null)
		parent->doMouseWheel(mKeys, zDelta, p);
}

void Canvas::doStartDrag(MouseKeys mKeys, point p) {
	if (startDrag.has_listeners())
		startDrag.fire(mKeys, p, this);
	else if (parent != null)
		parent->doStartDrag(mKeys, p);
}

void Canvas::doDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (drag.has_listeners())
		drag.fire(mKeys, p, target);
	else if (parent != null)
		parent->doDrag(mKeys, p, target);
}

void Canvas::doDrop(MouseKeys mKeys, point p, Canvas* target) {
	if (drop.has_listeners())
		drop.fire(mKeys, p, target);
	else if (parent != null)
		parent->doDrop(mKeys, p, target);
}

void Canvas::doMouseMove(point p) {
	if (mouseMove.has_listeners())
		mouseMove.fire(p, this);
	else if (parent != null)
		parent->doMouseMove(p);
}

void Canvas::doOpenContextMenu(point p) {
	if (openContextMenu.has_listeners())
		openContextMenu.fire(p, this);
	else if (parent != null)
		parent->doOpenContextMenu(p);
}

void Canvas::doMouseHover(point p) {
	if (mouseHover.has_listeners())
		mouseHover.fire(p, this);
	else if (parent != null)
		parent->doMouseHover(p);
}

bool Canvas::under(Canvas* c) {
	Canvas* u = this;
	while (u) {
		if (u == c)
			return true;
		u = u->parent;
	}
	return false;
}

void Canvas::invalidate() {
	invalidate(bounds);
}

void Canvas::unmeasure() {
	if (_preferredSize.hasSize()) {
		_preferredSize = NO_SIZE;
		for (Canvas* sc = child; sc != null; sc = sc->sibling)
			sc->unmeasure();
	}
}

Spacer::Spacer(int left, int top, int right, int bottom, Canvas* insert) {
	init(left, top, right, bottom);
	if (insert)
		append(insert);
}

Spacer::Spacer(int margin, Canvas* insert) {
	init(margin, margin, margin, margin);
	if (insert)
		append(insert);
}

void Spacer::init(int left, int top, int right, int bottom) {
	this->left = left;
	this->right = right;
	this->top = top;
	this->bottom = bottom;
}

dimension Spacer::measure() {
	if (child != null){
		dimension sz = child->preferredSize();
		sz.width += coord(left + right);
		sz.height += coord(top + bottom);
		return sz;
	} else {
		return dimension(left + right, top + bottom);
	}
}

void Spacer::layout(dimension sz) {
	sz.width -= left + right;
	sz.height -= top + bottom;
	point p = bounds.topLeft;
	p.x += left;
	p.y += top;
	for (Canvas* c = child; c != null; c = c->sibling) {
		c->at(p);
		c->resize(sz);
	}
}

Image::Image(const string &filename) {
	_filename = filename;
	_bitmap = null;
}

Image::~Image() {
	delete _bitmap;
}

dimension Image::measure() {
	if (_bitmap == null)
		_bitmap = loadBitmap(device(), _filename);
	return dimension(_bitmap->width(), _bitmap->height());
}

void Image::paint(Device *d) {
	d->stretchBlt(bounds.topLeft.x, bounds.topLeft.y, bounds.width(), bounds.height(), 
				  _bitmap, 0, 0, _bitmap->width(), _bitmap->height());
}

Bevel::Bevel(int width, Canvas* c) {
	init(width);
	if (c)
		append(c);
}

Bevel::Bevel(int width, bool depressed, Canvas* c) {
	init(width);
	set_depressed(depressed);
	if (c)
		append(c);
}

void Bevel::set_depressed(bool d) {
	if (_depressed != d){
		if (d){
			_backEdgeColor = frontEdgeColor;
			_frontEdgeColor = backEdgeColor;
		} else {
			_frontEdgeColor = frontEdgeColor;
			_backEdgeColor = backEdgeColor;
		}
		invalidate();
		_depressed = d;
	}
}

dimension Bevel::measure() {
	if (child != null){
		dimension sz = child->preferredSize();
		sz.width += coord(2 * edgeWidth);
		sz.height += coord(2 * edgeWidth);
		return sz;
	} else {
		return dimension(2 * edgeWidth, 2 * edgeWidth);
	}
}

point Bevel::innerOrigin() {
	point p;

	p.x = edgeWidth;
	p.y = edgeWidth;
	return p;
}

void Bevel::paint(Device* b) {
	rectangle r;

//	debugPrint(string("bp:") + bounds.height() + ":" + bounds.width() + "at:" + bounds.topLeft.x + ":" + bounds.topLeft.y + "\n");
	dimension d = bounds.size();
	r = bounds;
	r.opposite.x -= edgeWidth;
	r.opposite.y = coord(r.topLeft.y + edgeWidth);
	b->fillRect(r, _frontEdgeColor);
	r.topLeft.y += edgeWidth;
	r.opposite.x = coord(r.topLeft.x + edgeWidth);
	r.opposite.y = coord(bounds.opposite.y - edgeWidth);
	b->fillRect(r, _frontEdgeColor);
	r.topLeft.x = bounds.topLeft.x;
	r.topLeft.y = coord(bounds.opposite.y - edgeWidth);
	r.opposite.x = coord(bounds.opposite.x);
	r.opposite.y = coord(bounds.opposite.y);
	b->fillRect(r, _backEdgeColor);
	r.topLeft.x = coord(bounds.opposite.x - edgeWidth);
	r.topLeft.y = bounds.topLeft.y;
	r.opposite.x = coord(bounds.opposite.x);
	r.opposite.y = coord(bounds.opposite.y - edgeWidth);
	b->fillRect(r, _backEdgeColor);
}

void Bevel::layout(dimension sz) {
	sz.width -= 2 * edgeWidth;
	sz.height -= 2 * edgeWidth;
	point p = bounds.topLeft;
	p.x += edgeWidth;
	p.y += edgeWidth;
	for (Canvas* c = child; c != null; c = c->sibling) {
		c->at(p);
		c->resize(sz);
	}
}

void Bevel::init(int width) {
	edgeWidth = coord(width);
	frontEdgeColor = &buttonHighlightColor;
	backEdgeColor = &buttonShadowColor;
	_frontEdgeColor = frontEdgeColor;
	_backEdgeColor = backEdgeColor;
	_depressed = false;
}

Border::Border(int width, Canvas* c) {
	init(width, width, width, width);
	if (c)
		append(c);
}

Border::Border(int left, int top, int right, int bottom, Canvas* c) {
	init(left, top, right, bottom);
	if (c)
		append(c);
}

dimension Border::measure() {
	if (child != null){
		dimension sz = child->preferredSize();
		sz.width += coord(_left + _right);
		sz.height += coord(_top + _bottom);
		return sz;
	} else {
		return dimension(_left + _right, _top + _bottom);
	}
}

point Border::innerOrigin() {
	point p;

	p.x = _left;
	p.y = _top;
	return p;
}

void Border::paint(Device* b) {
	rectangle r;

//	debugPrint(string("bp:") + bounds.height() + ":" + bounds.width() + "at:" + bounds.topLeft.x + ":" + bounds.topLeft.y + "\n");
	dimension d = bounds.size();
	r = bounds;
	r.opposite.x -= _right;
	r.opposite.y = coord(r.topLeft.y + _top);
	b->fillRect(r, color);
	r.topLeft.y += _top;
	r.opposite.x = coord(r.topLeft.x + _left);
	r.opposite.y = coord(bounds.opposite.y - _bottom);
	b->fillRect(r, color);
	r.topLeft.x = bounds.topLeft.x;
	r.topLeft.y = coord(bounds.opposite.y - _bottom);
	r.opposite.x = coord(bounds.opposite.x);
	r.opposite.y = coord(bounds.opposite.y);
	b->fillRect(r, color);
	r.topLeft.x = coord(bounds.opposite.x - _right);
	r.topLeft.y = bounds.topLeft.y;
	r.opposite.x = coord(bounds.opposite.x);
	r.opposite.y = coord(bounds.opposite.y - _bottom);
	b->fillRect(r, color);
}

void Border::layout(dimension sz) {
	sz.width -= _left + _right;
	sz.height -= _top + _bottom;
	point p = bounds.topLeft;
	p.x += _left;
	p.y += _top;
	for (Canvas* c = child; c != null; c = c->sibling) {
		c->at(p);
		c->resize(sz);
	}
}

void Border::init(int left, int top, int right, int bottom) {
	_left = left;
	_top = top;
	_right = right;
	_bottom = bottom;
	color = &buttonShadowColor;
}

Filler::Filler(Canvas* c) {
	if (c)
		append(c);
}

dimension Filler::measure() {
	if (child != null)
		return child->preferredSize();
	else
		return dimension(0, 0);
}

point Filler::innerOrigin() {
	point p;

	return p;
}

void Filler::layout(dimension sz) {
	for (Canvas* c = child; c != null; c = c->sibling) {
		dimension inner = child->preferredSize();
		if (inner.width > sz.width)
			inner.width = sz.width;
		if (inner.height > sz.height)
			inner.height = sz.height;
		c->at(bounds.topLeft);
		c->resize(inner);
	}
}

ButtonHandler::ButtonHandler(Bevel* b, int ri) {
	_button = b;
	init();
	set_repeatInterval(ri);
}

ButtonHandler::~ButtonHandler() {
	stopTimer();
}

void ButtonHandler::init() {
	_button->buttonDown.addHandler(this, &ButtonHandler::pushButton);
	_button->click.addHandler(this, &ButtonHandler::onClick);
	_button->drag.addHandler(this, &ButtonHandler::onDrag);
	_button->drop.addHandler(this, &ButtonHandler::onDrop);
	_button->disableDoubleClick = true;
	_repeat = null;
	_repeated = false;
}

void ButtonHandler::set_repeatInterval(int ri) {
	_repeatInterval = ri;
}

void ButtonHandler::pushButton(MouseKeys mKeys, point p, Canvas* target) {
	_button->set_depressed(true);
	if (_repeatInterval != 0)
		startTimer();
}

void ButtonHandler::startTimer() {
	if (_repeat == null) {
		_repeat = _button->startTimer(_repeatInterval * 2);
		_repeat->tick.addHandler(this, &ButtonHandler::onTick);
	}
}

void ButtonHandler::stopTimer() {
	if (_repeat != null) {
		_repeat->kill(null);
		_repeat = null;
	}
}

void ButtonHandler::onTick() {
	if (_button->depressed())
		click.fire(_button);
	_repeated = true;
}

void ButtonHandler::onClick(MouseKeys mKeys, point p, Canvas* target) {
	_button->set_depressed(false);
	if (!_repeated)
		click.fire(_button);
	stopTimer();
}

void ButtonHandler::onDrag(MouseKeys mKeys, point p, Canvas* target) {
	_button->set_depressed(_button->bounds.contains(p));
}

void ButtonHandler::onDrop(MouseKeys mKeys, point p, Canvas* target) {
	_button->set_depressed(false);
	if (_button->bounds.contains(p) && !_repeated)
		click.fire(_button);
	stopTimer();
}

point EdgeCanvas::innerOrigin() {
	point p;
		
	p.x = 0;
	p.y = 0;
	return p;
}

RootCanvas* getRootCanvas(HWND handle) {
	return (RootCanvas*)GetProp(handle, "pRootCanvas");
}

void setRootCanvas(HWND handle, RootCanvas* rc) {
	SetProp(handle, "pRootCanvas", (HANDLE)rc);
}

void doSize(HWND handle, dimension d) {
	RootCanvas* rc = getRootCanvas(handle);
	if (rc != null)
		rc->resize(d);
}
#if 0
doMessage: (x: int)
{
	windows.debugPrint("msg=" + x + "\n")
}
#endif
/*
 *	mouseEvent
 *
 *	Called from the window proc, this code routes most incoming mouse events to
 *	the correct RootCanvas, which then does further processing.
 */
void mouseEvent(HWND handle, unsigned uMsg, MouseKeys fwKeys, short zDelta, point p) {
	getRootCanvas(handle)->mouseEvent(uMsg, fwKeys, zDelta, p);
}

void activateWindow(HWND handle, short fActive, bool fMinimized) {
	getRootCanvas(handle)->activateWindow(fActive, fMinimized);
}

void keyDownEvent(HWND handle, int virtualKey, unsigned keyData) {
	getRootCanvas(handle)->keyDownEvent(virtualKey, keyData);
}

void keyUpEvent(HWND handle, int virtualKey, unsigned keyData) {
	getRootCanvas(handle)->keyUpEvent(virtualKey, keyData);
}

void setFocusEvent(HWND handle, HWND loseHandle) {
	getRootCanvas(handle)->setFocusEvent(loseHandle);
}

void killFocusEvent(HWND handle, HWND getHandle) {
	getRootCanvas(handle)->killFocusEvent(getHandle);
}

void closeWindow(HWND handle) {
	getRootCanvas(handle)->close.fire();
}

void deleteRootCanvas(HWND handle) {
	delete getRootCanvas(handle);
}

void runInRootCanvas(HWND handle, LPARAM lParam) {
	getRootCanvas(handle)->execute((void*)lParam);
}

void timerEvent(HWND handle, unsigned id) {
	getRootCanvas(handle)->timerEvent(id);
}

#if 0
idleEvent: ()
{
	fire idle()
}

idle: public event ()

transBltCount: int = 0

#endif

static Pen* togglePen = createPen(PS_SOLID, 1, 0);

Toggle::Toggle(data::Boolean* iValue) {
	state = iValue;
}

dimension Toggle::measure() {
	return dimension(TOGGLE_SIZE, TOGGLE_SIZE);
}

void Toggle::paint(Device* b) {
	dimension d = bounds.size();
	dimension d2 = d;
	if (d.width > TOGGLE_SIZE)
		d2.width = TOGGLE_SIZE;
	if (d.height > TOGGLE_SIZE)
		d2.height = TOGGLE_SIZE;
	int orX = bounds.topLeft.x + (d.width - d2.width) / 2;
	int orY = bounds.topLeft.y + (d.height - d2.height) / 2;
	int oppX = orX + d2.width - 1;
	int oppY = orY + d2.height - 1;
	b->set_pen(togglePen);
	b->line(orX, orY, oppX, orY);
	b->line(oppX, orY, oppX, oppY);
	b->line(oppX, oppY, orX, oppY);
	b->line(orX, oppY, orX, orY);
	if (state->value()){
		b->line(orX, oppY, oppX, orY);
		b->line(orX, orY, oppX, oppY);
	}
}

ToggleHandler::ToggleHandler(Toggle* t) {
	_toggle = t;
	t->state->changed.addHandler(this, &ToggleHandler::onChange);
	t->click.addHandler(this, &ToggleHandler::onClick);
	t->drop.addHandler(this, &ToggleHandler::onClick);
	t->disableDoubleClick = true;
}

void ToggleHandler::onClick(MouseKeys mKeys, point p, Canvas* target) {
	if (target == _toggle)
		_toggle->state->set_value(!_toggle->state->value());
}

void ToggleHandler::onChange() {
	_toggle->invalidate();
}

static Color radioColor(0, 0, 0);
static Color radioEmptyColor(0xff, 0xff, 0xff);

RadioButton::RadioButton(data::Integer* iValue, int s) {
	state = iValue;
	setting = s;
}

dimension RadioButton::measure() {
	return dimension(RADIO_SIZE, RADIO_SIZE);
}

void RadioButton::paint(Device* b) {
	dimension d = bounds.size();
	dimension d2 = d;
	if (d.width > RADIO_SIZE)
		d2.width = RADIO_SIZE;
	if (d.height > RADIO_SIZE)
		d2.height = RADIO_SIZE;
	rectangle r;
	r.topLeft.x = coord(bounds.topLeft.x + (d.width - d2.width) / 2);
	r.topLeft.y = coord(bounds.topLeft.y + (d.height - d2.height) / 2);
	r.set_size(d2);
	b->set_pen(togglePen);
	b->set_background(&radioEmptyColor);
	b->ellipse(r);
	if (state->value() == setting){
		r.topLeft.x += 2;
		r.topLeft.y += 2;
		r.opposite.x -= 2;
		r.opposite.y -= 2;
		b->set_background(&radioColor);
		b->ellipse(r);
	}
}

RadioHandler::RadioHandler(data::Integer* i) {
	_currentRadio = null;
	_radios = null;
	i->changed.addHandler(this, &RadioHandler::onChanged, i);
}

void RadioHandler::member(RadioButton* r) {
	if (r->setting == r->state->value())
		_currentRadio = r;
	r->click.addHandler(this, &RadioHandler::onClick);
	r->drop.addHandler(this, &RadioHandler::onClick);
	r->disableDoubleClick = true;
	r->next = _radios;
	_radios = r;
}

void RadioHandler::onClick(MouseKeys mKeys, point p, Canvas* target) {
	for (RadioButton* r = _radios; r != null; r = r->next)
		if (r == target) {
			if (r != _currentRadio){
				r->state->set_value(r->setting);
				r->invalidate();
				if (_currentRadio != null)
					_currentRadio->invalidate();
				_currentRadio = r;
			}
			return;
		}
}

void RadioHandler::onChanged(data::Integer* i) {
	int x = i->value();
	if (_currentRadio != null){
		if (_currentRadio->setting == x)
			return;
		_currentRadio->invalidate();
		_currentRadio = null;
	}
	for (RadioButton* r = _radios; r != null; r = r->next)
		if (r->setting == x){
			r->invalidate();
			_currentRadio = r;
			return;
		}
}

}  // namespace display
