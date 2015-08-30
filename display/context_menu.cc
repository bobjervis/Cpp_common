#include "../common/platform.h"
#include "context_menu.h"

#include "../common/machine.h"
#include "background.h"
#include "device.h"
#include "grid.h"
#include "label.h"

namespace display {

const int NO_SELECTION = -1;

class ContextMenuLabel : public Spacer {
	typedef Spacer super;
public:
	ContextMenuLabel(ContextMenu* m, ContextMenuItem* c) : super(0, 0, c->isPullRight() ? 10 : 0, 0, null) {
		_label = new display::Label(c->tag);
		append(_label);
		menu = m;
		item = c;
	}

	virtual void paint(Device* device) {
		if (item->isPullRight()) {
			if (menu->_symbolFont == null)
				menu->_symbolFont = symbolFont()->currentFont(device->owner());
			device->set_font(menu->_symbolFont);
			int m = device->backMode(TRANSPARENT);
			display::Color* color = _label->textColor();
			device->setTextColor(color ? color->value() : 0);
			device->textBox(bounds, SYMBOL_RIGHT_TRIANGLE, DT_RIGHT|DT_VCENTER);
			device->backMode(m);
 		}
	}

	display::Label* label() const { return _label; }

private:
	ContextMenu*		menu;
	ContextMenuItem*	item;
	display::Label*		_label;
};

ContextMenu::ContextMenu(RootCanvas* w, point anchor, Canvas* target) {
	init(anchor, w, null, target);
}

ContextMenu::ContextMenu(ContextMenu* parentMenu, vector<ContextMenuItem*>& items, Canvas* lbl) {
	init(parentMenu->_anchor, parentMenu->parent, parentMenu, parentMenu->target);

	_topLeft.x = coord(_anchor.x + lbl->bounds.opposite.x);
	_topLeft.y = coord(_anchor.y + lbl->bounds.topLeft.y);
	for (int i = 0; i < items.size(); i++)
		_items.push_back(items[i]);
	this->choiceCount = items.size();
	_itemsCloned = true;
}

void ContextMenu::init(point anchor, RootCanvas* parent, ContextMenu* parentMenu, Canvas* target) {
	_anchor = anchor;
	_topLeft = _anchor;
	this->parent = parent;
	this->parentMenu = parentMenu;
	this->target = target;

	_symbolFont = null;
	setBackground(&buttonFaceBackground);
	hilightedBackground = &blackBackground;
	hilightedTextColor = &white;
	parentMenu = null;
	choiceCount = 0;
	_itemsCloned = false;
}

ContextMenu::~ContextMenu() {
	if (!_itemsCloned)
		_items.deleteAll();
}

Choice* ContextMenu::choice(const string& tag) {
	choiceCount++;
	Choice* c = new Choice(tag);
	_items.push_back(c);
	return c;
}

PullRight* ContextMenu::pullRight(const string &tag) {
	choiceCount++;
	PullRight* p = new PullRight(parent, tag);
	_items.push_back(p);
	return p;
}

void ContextMenu::show() {
	if (_items.size() == 0)
		return;				// if nothing to show, don't
	labelGrid = new Grid();
		for (int i = 0; i < _items.size(); i++) {
			ContextMenuLabel* lbl = new ContextMenuLabel(this, _items[i]);
			_labels.push_back(lbl);
			labelGrid->row();
			labelGrid->cell(lbl);
		}
	labelGrid->complete();
	append(new Bevel(CONTEXT_EDGE, labelGrid));

	super::show();
	parent->setModalChild(this);
	_handle = CreateWindowEx(0, "EuropaFrame", "", 
							 WS_POPUP, _topLeft.x, _topLeft.y, 0, 0,
							 parent->handle(), null, (HINSTANCE)GetModuleHandle(null), null);
	_windowStyle = WS_POPUP;
	setRootCanvas(_handle, this);
	_selected = NO_SELECTION;
	dimension sz = preferredSize();
	RECT rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	rectangle r;

	WINDOWINFO w;

	w.cbSize = sizeof(w);
	GetWindowInfo(parent->handle(), &w);

	r.topLeft.x = w.rcClient.left + _topLeft.x;
	r.topLeft.y = w.rcClient.top + _topLeft.y;
	r.opposite.x = r.topLeft.x + sz.width;
	r.opposite.y = r.topLeft.y + sz.height;
	if (r.opposite.y > rect.bottom)
		r.topLeft.y -= r.opposite.y - rect.bottom;
	if (r.opposite.x > rect.right)
		r.topLeft.x -= r.opposite.x - rect.right;
	if (r.topLeft.y < rect.top)
		r.topLeft.y = rect.top;
	if (r.topLeft.x < rect.left)
		r.topLeft.x = rect.left;
	SetWindowPos(_handle, HWND_TOP, r.topLeft.x, r.topLeft.y, sz.width, sz.height, 0);
	GetClientRect(_handle, &rect);
	SetCapture(_handle);
	bounds.topLeft.x = coord(rect.left);
	bounds.topLeft.y = coord(rect.top);
	bounds.opposite.x = coord(rect.right);
	bounds.opposite.y = coord(rect.bottom);
	drag.addHandler(this, &ContextMenu::onDrag);
	mouseMove.addHandler(this, &ContextMenu::onMouseMove);
	click.addHandler(this, &ContextMenu::onClick);
	drop.addHandler(this, &ContextMenu::onDrop);
	child->resize(bounds.size());
	ShowWindow(_handle, SW_SHOWNORMAL);
	run(this, &ContextMenu::emulateStartDrag, parent->mouseLocation());
}

void ContextMenu::hide() {
	deselect();
	super::hide();
	ShowWindow(_handle, SW_HIDE);
}

void ContextMenu::hideAllMenus() {
	if (parentMenu)
		parentMenu->hideAllMenus();
	hide();
}

void ContextMenu::dismiss() {
	deselect();
	if (parentMenu)
		parentMenu->becomeModal();
	parentMenu = null;
	shutDown();
}

void ContextMenu::select(int index) {
	deselect();
	_selected = index;
	if (_selected != NO_SELECTION) {
		_labels[_selected]->setBackground(hilightedBackground);
		_labels[_selected]->label()->set_textColor(hilightedTextColor);
		_items[_selected]->select(this, _labels[_selected]);
	}
}

void ContextMenu::deselect() {
	if (_selected != NO_SELECTION) {
		_labels[_selected]->setBackground(null);
		_labels[_selected]->label()->set_textColor(&black);
		_items[_selected]->deselect();
		_selected = NO_SELECTION;
	}
}

void ContextMenu::onMouseMove(point p, Canvas* target) {
	point physical = toScreenCoords(p);
	for (ContextMenu* chain = this; chain != null; chain = chain->parentMenu) {
		rectangle r = chain->screenBounds();
		if (r.contains(physical)) {
			p = chain->toLocalCoords(physical);
			int i = 0;
			for (Canvas* c = chain->labelGrid->child; c != null; c = c->sibling, i++) {
				if (c->bounds.contains(p)) {
					if (chain->_selected != i) {
						chain->deselect();
						chain->select(i);
					}
					return;
				}
			}
			if (this == chain)
				deselect();
			return;
		}
	}
}

void ContextMenu::onDrag(MouseKeys mKeys, point p, Canvas* target) {
	onMouseMove(p, target);
}

void ContextMenu::onClick(MouseKeys mKeys, point p, Canvas* target) {
	int i = 0;
	for (Canvas* c = labelGrid->child; c != null; c = c->sibling, i++) {
		if (c->bounds.contains(p)) {
			if (_items[i]->isPullRight())
				return;
			hideAllMenus();
			_items[i]->fireClick(_anchor, this->target);
			break;
		}
	}
	shutDown();
}

void ContextMenu::onDrop(MouseKeys mKeys, point p, Canvas* target) {
	int i = 0;
	for (Canvas* c = labelGrid->child; c != null; c = c->sibling, i++) {
		if (c->bounds.contains(p)) {
			if (_items[i]->isPullRight())
				return;
			hideAllMenus();
			_items[i]->fireClick(_anchor, this->target);
			break;
		}
	}
	shutDown();
}

bool ContextMenu::isInGrid(Canvas* c) {
	for (Canvas* r = labelGrid->child; r != null; r = r->sibling)
		if (r == c)
			return true;
	return false;
}

void ContextMenu::emulateStartDrag(point p) {
	mouseEvent(WM_LBUTTONDOWN, 0, 0, p);
}

void ContextMenu::becomeModal() {
	parent->setModalChild(this);
	SetCapture(_handle);
}

void ContextMenu::shutDown() {
	hide();
	if (parentMenu != null)
		parentMenu->shutDown();
	parent->setModalChild(null);
	ReleaseCapture();
	scheduleDelete();
}

ContextMenuItem::~ContextMenuItem() {
}

void ContextMenuItem::fireClick(point p, Canvas* target) {
}

void ContextMenuItem::select(ContextMenu* cm, Canvas* lbl) {
}

void ContextMenuItem::deselect() {
}

bool ContextMenuItem::isPullRight() {
	return false;
}

Choice::Choice(const string &tag) {
	this->tag = tag;
}

void Choice::fireClick(point p, Canvas* target) {
	click.fire(p, target);
}

void Choice::fireButtonClick(display::Bevel *b) {
	click.fire(b->bounds.topLeft, b);
}

PullRight::PullRight(RootCanvas *parent, const string &tag) {
	_parent = parent;
	_subMenu = null;
	this->tag = tag;
}

PullRight::~PullRight() {
	if (_subMenu)
		_subMenu->dismiss();
}

void PullRight::select(ContextMenu* cm, Canvas* lbl) {
	if (_subMenu == null) {
		_subMenu = new ContextMenu(cm, _items, lbl);
		_subMenu->show();
	}
}

void PullRight::deselect() {
	_subMenu->dismiss();
	_subMenu = null;
}

bool PullRight::isPullRight() {
	return true;
}

Choice* PullRight::choice(const string &tag) {
	Choice* c = new Choice(tag);
	_items.push_back(c);
	return c;
}

PullRight* PullRight::pullRight(const string& tag) {
	PullRight* p = new PullRight(_parent, tag);
	_items.push_back(p);
	return p;
}

}  // namespace display
