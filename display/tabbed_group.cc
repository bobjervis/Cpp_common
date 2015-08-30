#include "../common/platform.h"
#include "tabbed_group.h"

#include <typeinfo.h>
#include "../common/machine.h"
#include "context_menu.h"
#include "device.h"
#include "hover_caption.h"

namespace display {

static Font tabLabelFont("Times New Roman", 10, false, false, false, null, null);
static Color emptyBackground(0x606060);
static Color unsavedTabBackground(255, 240, 200);
static Color tabBackground(0xffffff);
static Color tabBorder(0);

TabbedGroup::TabbedGroup() {
	_moreButton = null;
	_tabLabelFont = null;
	_selected = 0;
	_currentBody = null;
	_tabHistory = DEFAULT_TAB_HISTORY;
	_tabs.push_back(null);
	_moreHandler = null;
}

TabbedGroup::~TabbedGroup() {
	while (_tabs.size() > 1)
		closeTab(_tabs[1]);
	delete _moreHandler;
}

void TabbedGroup::addFileFilter(FileManager* manager) {
	if (_filters.size() == 0) {
		Bevel* b = new Bevel(2, new Image("more_button.bmp"));
		_moreButton = b;
		_moreHandler = new ButtonHandler(b, 0);
		_moreHandler->click.addHandler(this, &TabbedGroup::moreClicked);
		append(b);
	}
	_filters.push_back(manager);
}

void TabbedGroup::select(TabManager* manager) {
	if (_tabs[_selected] == manager)
		return;
	if (_selected > 0) {
		_tabs[_selected]->unselected.fire();
		_selected = 0;
		switchBody(null);
	}
	if (_currentBody) {
		_currentBody->prune();
		_currentBody = null;
	}
	if (manager) {
		int index;
		if (manager->owner() != this) {
			if (manager->owner())
				manager->owner()->removeTab(manager);
			index = _tabs.size();
			_tabs.push_back(manager);
			manager->_owner = this;
			manager->_visible = false;
		} else
			index = indexOf(manager);
		if (!manager->_visible) {
			_tabs.remove(index);
			_tabs.insert(1, manager);
			index = 1;
		}
		while (_tabs.size() > _tabHistory)
			closeTab(_tabs[_tabs.size() - 1]);

		_selected = index;
		switchBody(manager->tabBody());
		manager->selected.fire();
		invalidate();
	}
}

void TabbedGroup::setTabHistory(int th) {
	_tabHistory = th;
	while (_tabs.size() > _tabHistory)
		closeTab(_tabs[_tabs.size() - 1]);
}

void TabbedGroup::closeTab(TabManager* manager) {
	if (removeTab(manager)) {
		manager->closed.fire(this);
		if (manager->deleteTab())
			delete manager;
	}
}

bool TabbedGroup::removeTab(TabManager* manager) {
	if (manager->_owner != this)
		return false;

	int oldSelected = _selected;
	int i = indexOf(manager);
	if (oldSelected == i)
		select(null);
	_tabs.remove(i);
	// Switch to the next tab, if there is one,
	// otherwise switch to the last tab, if any.
	if (oldSelected == i) {
		if (i < _tabs.size())
			select(_tabs[i]);
		else if (i > 1)
			select(_tabs[i - 1]);
	}
	manager->_owner = null;
	invalidate();
	return true;
}

void TabbedGroup::replaceManager(TabManager* oldManager, TabManager* newManager) {
	if (oldManager->owner() != this)
		return;
	bool selected = false;
	int i = indexOf(oldManager);
	if (i == _selected) {
		select(null);
		selected = true;
	}
	if (newManager->_owner != null)
		newManager->owner()->removeTab(newManager);
	newManager->_owner = this;
	_tabs[i] = newManager;
	oldManager->closed.fire(this);
	if (selected)
		select(newManager);
}

void TabbedGroup::refreshSelectedTab() {
	switchBody(_tabs[_selected]->tabBody());
//	_selected->selected();
	rectangle r = bounds;
	r.opposite.y = r.topLeft.y + _tabsAreaHeight;
	invalidate(r);
}

dimension TabbedGroup::measure() {
	if (_tabLabelFont == null)
		_tabLabelFont = tabLabelFont.currentFont(rootCanvas());
	Device* d = device();
	d->set_font(_tabLabelFont);
	dimension sz = d->textExtent("M", 1);
	sz.width = TAB_LEFT_MARGIN + TAB_BORDER_WIDTH * 2;
	sz.height += TAB_PADDING * 2 + TAB_BORDER_WIDTH + TAB_TOP_MARGIN;
	if (_moreButton) {
		dimension moreSize = _moreButton->preferredSize();
		sz.width += moreSize.width + MORE_BUTTON_PADDING * 2;
		int h = moreSize.height + MORE_BUTTON_PADDING * 2 + TAB_BORDER_WIDTH;
		if (h > sz.height)
			sz.height = h;
	}
	_tabsAreaHeight = sz.height;
	if (_currentBody) {
		dimension bodySize = _currentBody->preferredSize();
		sz.height += bodySize.height;
		if (bodySize.width > sz.width)
			sz.width = bodySize.width;
	}
	return sz;
}

void TabbedGroup::layout(dimension size) {
	if (_currentBody) {
		point bodyAt = bounds.topLeft;
		bodyAt.y += _tabsAreaHeight;
		_currentBody->at(bodyAt);
		size.height -= _tabsAreaHeight;
		_currentBody->resize(size);
	}
	if (_moreButton) {
		point moreAt = bounds.topLeft;
		dimension moreSize = _moreButton->preferredSize();
		moreAt.x += size.width - moreSize.width - MORE_BUTTON_PADDING;
		moreAt.y += MORE_BUTTON_PADDING;
		_moreButton->at(moreAt);
		_moreButton->resize(moreSize);
	}
}

void TabbedGroup::paint(Device *d) {
	rectangle r = bounds;
	if (_tabs.size())
		r.opposite.y = r.topLeft.y + _tabsAreaHeight;
	d->fillRect(r, &emptyBackground);
	int h = _tabsAreaHeight - TAB_BORDER_WIDTH;
	if (_tabs.size()) {
		r.topLeft = bounds.topLeft;
		r.opposite.x = bounds.opposite.x;
		r.topLeft.y += h;
		r.opposite.y = r.topLeft.y + TAB_BORDER_WIDTH;
		d->fillRect(r, &tabBorder);
		d->set_font(_tabLabelFont);
		int nextTabX = bounds.topLeft.x + TAB_LEFT_MARGIN;
		for (int i = 1; i < _tabs.size(); i++) {
			TabManager* manager = _tabs[i];
			string tabLabel = manager->tabLabel();
			if (!manager->_labelSize.hasSize())
				manager->_labelSize = d->textExtent(tabLabel);
			int tabRightX = nextTabX + TAB_BORDER_WIDTH + TAB_PADDING * 2 + manager->_labelSize.width;
			if (tabRightX > bounds.opposite.x && i > 1) {
				for (; i < _tabs.size(); i++)
					_tabs[i]->_visible = false;
				break;
			}
			int topY = bounds.topLeft.y + TAB_TOP_MARGIN;
			r.topLeft.x = nextTabX;
			r.opposite.x = nextTabX + TAB_BORDER_WIDTH;
			r.topLeft.y = topY;
			r.opposite.y = topY + h;
			d->fillRect(r, &tabBorder);
			r.topLeft.x = tabRightX;
			r.opposite.x = tabRightX + TAB_BORDER_WIDTH;
			r.topLeft.y = topY;
			r.opposite.y = topY + h;
			d->fillRect(r, &tabBorder);
			r.topLeft.x = nextTabX;
			r.opposite.x = tabRightX;
			r.topLeft.y = topY;
			r.opposite.y = topY + TAB_BORDER_WIDTH;
			d->fillRect(r, &tabBorder);
			r.topLeft.x = nextTabX + TAB_BORDER_WIDTH;
			r.opposite.x = tabRightX;
			r.topLeft.y = topY + TAB_BORDER_WIDTH;
			r.opposite.y = topY + h;
			manager->_visible = true;
			if (i == _selected)
				r.opposite.y += TAB_BORDER_WIDTH;
			if (manager->needsSave())
				d->fillRect(r, &unsavedTabBackground);
			else
				d->fillRect(r, &tabBackground);
			int textX = nextTabX + TAB_BORDER_WIDTH + TAB_PADDING;
			int textY = topY + TAB_BORDER_WIDTH + TAB_PADDING;
			d->backMode(TRANSPARENT);
			d->setTextColor(0);
			d->text(textX, textY, tabLabel);
			if (manager->hasErrors())
				d->squiggle(textX, textY + manager->_labelSize.height + 1, manager->_labelSize.width, &red);
			nextTabX = tabRightX + TAB_INTER_MARGIN + TAB_BORDER_WIDTH;
		}
	}
}

void TabbedGroup::moreClicked(Bevel* b) {
	point p = _moreButton->bounds.topLeft;
	p.y += _moreButton->height();
	ContextMenu* c = new ContextMenu(rootCanvas(), p, this);
	c->choice("Open file...")->click.addHandler(this, &TabbedGroup::openFile);
	for (int i = 0; i < _filters.size(); i++)
		_filters[i]->extendContextMenu(c);
	c->show();
}

void TabbedGroup::openFile(point p, Canvas* c) {
	OPENFILENAME o;
	char buffer[1024];

	memset(&o, 0, sizeof o);
	buffer[0] = 0;
	o.lStructSize = sizeof o;
	o.hwndOwner = c->rootCanvas()->handle();
	o.lpstrFile = buffer;
	o.nMaxFile = sizeof buffer;
	if (GetOpenFileName(&o)) {
		for (int i = 0; i < _filters.size(); i++) {
			if (_filters[i]->matches(buffer) &&
				_filters[i]->openFile(buffer))
				break;
		}
	}
}

void TabbedGroup::switchBody(Canvas* c) {
	if (c == _currentBody)
		return;
	if (_currentBody) {
		RootCanvas* c = rootCanvas();
		if (c) {
			Canvas* focus = c->keyFocus();
			if (focus && focus->under(_currentBody))
				c->setKeyboardFocus(null);
		}
		_currentBody->prune();
		_currentBody = null;
	}
	if (c) {
		_currentBody = c;
		append(c);
		point p = bounds.topLeft;
		p.y += _tabsAreaHeight;
		c->at(p);
		dimension sz = bounds.size();
		sz.height -= _tabsAreaHeight;
		c->resize(sz);
	}
}

int TabbedGroup::indexOf(TabManager* manager) const {
	if (manager->_owner != this)
		return 0;
	for (int i = 1; i < _tabs.size(); i++)
		if (_tabs[i] == manager)
			return i;
	manager->_owner = null;
	return 0;							// should probably raise an exception		
}

TabbedGroupHandler::TabbedGroupHandler(TabbedGroup *group) {
	_group = group;
	_group->click.addHandler(this, &TabbedGroupHandler::onClick);
	_group->openContextMenu.addHandler(this, &TabbedGroupHandler::onOpenContextMenu);
	_group->mouseHover.addHandler(this, &TabbedGroupHandler::onHover);
	_group->startDrag.addHandler(this, &TabbedGroupHandler::onStartDrag);
	_group->drag.addHandler(this, &TabbedGroupHandler::onDrag);
	_group->drop.addHandler(this, &TabbedGroupHandler::onDrop);
	_menuTab = null;
}

void TabbedGroupHandler::onClick(MouseKeys mKeys, point p, Canvas* target) {
	TabManager* manager = hitTest(p, target);
	if (manager)
		_group->select(manager);
}

void TabbedGroupHandler::onOpenContextMenu(point p, Canvas* target) {
	TabManager* manager = hitTest(p, target);
	if (manager) {
		_menuTab = manager;
		ContextMenu* cm = new ContextMenu(_group->rootCanvas(), p, _group);
		if (_menuTab->needsSave())
			cm->choice(string("Save ") + _menuTab->tabLabel())->click.addHandler(this, &TabbedGroupHandler::saveTab);
		cm->choice("Close")->click.addHandler(this, &TabbedGroupHandler::closeTab);
		_menuTab->extendContextMenu(cm);
		cm->show();
	} else {
		ContextMenu* cm = new ContextMenu(_group->rootCanvas(), p, _group);
		const vector<TabManager*>& tabs = _group->tabs();
		for (int i = 1; i < tabs.size(); i++) {
			if (tabs[i]->visible())
				continue;
			cm->choice(tabs[i]->tabLabel())->click.addHandler(this, &TabbedGroupHandler::selectTab, tabs[i]);
		}
		cm->show();
	}
}

void TabbedGroupHandler::onHover(point p, Canvas* target) {
	TabManager* manager = hitTest(p, target);
	if (manager) {
		Canvas* c = manager->onHover();
		if (c) {
			HoverCaption* hc = _group->rootCanvas()->hoverCaption();
			hc->append(c);
			hc->show(p);
		}
	}
}

void TabbedGroupHandler::onStartDrag(MouseKeys keys, point p, Canvas* target) {
	_menuTab = hitTest(p, target);
	if (_menuTab == null)
		return;
	_group->mouseCursor = standardCursor(NO);
}

void TabbedGroupHandler::onDrag(MouseKeys keys, point p, Canvas* target) {
	if (_menuTab == null)
		return;
	if (target != null && 
		typeid(*target) == typeid(TabbedGroup)) {
		if (target != _group) {
			_group->mouseCursor = standardCursor(CROSS);
			return;
		}
	}
	_group->mouseCursor = standardCursor(NO);
}

void TabbedGroupHandler::onDrop(MouseKeys keys, point p, Canvas* target) {
	if (_menuTab == null)
		return;
	_group->mouseCursor = null;
	if (target == null)
		return;
	if (typeid(*target) == typeid(TabbedGroup)) {
		if (target == _group)
			return;
		TabbedGroup* t = (TabbedGroup*)target;
		t->select(_menuTab);
		_menuTab = null;
	}
}


void TabbedGroupHandler::closeTab(point p, Canvas *c) {
	_group->closeTab(_menuTab);
	_menuTab = null;
}

void TabbedGroupHandler::saveTab(point p, Canvas *c) {
	if (_menuTab->save())
		_group->invalidate();
	else
		warningMessage(string("Could not save ") + _menuTab->tabLabel());
}

void TabbedGroupHandler::selectTab(point p, Canvas* c, TabManager* manager) {
	_group->select(manager);
}

TabManager* TabbedGroupHandler::hitTest(point p, Canvas* c) {
	int h = _group->tabsAreaHeight() - TAB_BORDER_WIDTH;
	if (p.y >= _group->bounds.topLeft.y + h)
		return null;
	if (p.y < _group->bounds.topLeft.y + TAB_TOP_MARGIN)
		return null;
	if (_group->tabs().size()) {
		int nextTabX = _group->bounds.topLeft.x + TAB_LEFT_MARGIN;
		for (int i = 1; i < _group->tabs().size(); i++) {
			if (p.x < nextTabX)
				return null;
			TabManager* manager = _group->tabs()[i];
			if (!manager->visible())
				return null;
			int tabRightX = nextTabX + TAB_BORDER_WIDTH + TAB_PADDING * 2 + manager->labelWidth();
			nextTabX = tabRightX + TAB_INTER_MARGIN + TAB_BORDER_WIDTH;
			if (p.x < tabRightX)
				return manager;
		}
	}
	return null;
}

TabManager::~TabManager() {
}

bool TabManager::deleteTab() {
	return true;
}

void TabManager::bind(TextEditor* editor) {
}

void TabManager::extendContextMenu(ContextMenu* menu) {
}

Canvas* TabManager::onHover() {
	return null;
}

bool TabManager::needsSave() {
	return false;
}

bool TabManager::save() {
	return false;
}

void TabManager::reload() {
}

bool TabManager::hasErrors() {
	return false;
}

void TabManager::clearAnnotations() {
}

void TabManager::tabModified() {
	if (_owner)
		_owner->refreshSelectedTab();
}

int TabManager::labelWidth() const {
	return _labelSize.width;
}

FileManager::~FileManager() {
}

void FileManager::extendContextMenu(ContextMenu* menu) {
}

} // namespace display