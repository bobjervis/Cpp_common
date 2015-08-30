#pragma once
#include "canvas.h"

#include "../common/vector.h"

namespace display {

class ContextMenu;
class FileManager;
class Label;
class TabManager;
class TextEditor;

const int MORE_BUTTON_PADDING = 6;
const int TAB_LEFT_MARGIN = 3;
const int TAB_INTER_MARGIN = 2;
const int TAB_TOP_MARGIN = 1;
const int TAB_PADDING = 5;
const int TAB_BORDER_WIDTH = 2;

const int DEFAULT_TAB_HISTORY = 15;		// Maximum number of tabs that can appear.

class TabbedGroup : public Canvas {
	typedef Canvas super;
public:
	TabbedGroup();

	~TabbedGroup();

	void addFileFilter(FileManager* manager);
	/*
	 *	select
	 *
	 *	This method selects the indicated tab.  If the tab is not currently
	 *	in this group, the tab is moved to this group.
	 *
	 *	If the tab is the currently selected tab, then the call has no effect.
	 *
	 *	If the tab is not, the currently selected tab is unselected.
	 *
	 *	If the tab is visible, it will be selected in place and the set of
	 *	visible tabs will be unaffected.
	 *
	 *	If the tab is not visible, it will be placed left-most in the set
	 *	of visible tabs and as many tab as necessary are removed to make room.
	 */
	void select(TabManager* manager);
	/*
	 *	closeTab
	 *
	 *	This method closes the given tab and removes it from the group.
	 *	
	 *	If the tab is currently selected, it will be unselected and the
	 *	next oldest tab will be selected.
	 *
	 *	If the deleteTab method of the TabManager returns true, the
	 *	TabManager is deleted.
	 */
	void closeTab(TabManager* manager);


	void setTabHistory(int th);

	void replaceManager(TabManager* oldManager, TabManager* newManager);

	void refreshSelectedTab();

	virtual dimension measure();

	virtual void layout(dimension size);

	virtual void paint(Device* b);

	const vector<TabManager*>& tabs() const { return _tabs; }

	const TabManager* selected() const { return _selected < 0 ? null : _tabs[_selected]; }

	int tabsAreaHeight() const { return _tabsAreaHeight; }

	int tabHistory() const { return _tabHistory; }
private:
	/*
	 *	removeTab
	 *
	 *	This method removes the given tab from the group.
	 *
	 *	If the tab is currently selected, it will be unselected and the
	 *	next oldest tab will be selected.
	 *
	 *	The TabManager will not be deleted.
	 */
	bool removeTab(TabManager* manager);

	void moreClicked(Bevel* b);

	void openFile(point p, Canvas* c);

	void switchBody(Canvas* c);

	int indexOf(TabManager* manager) const;

	Canvas*					_moreButton;
	ButtonHandler*			_moreHandler;
	vector<TabManager*>		_tabs;				// elelemtn 0 is always null, left-most tab is index 1
	int						_selected;
	vector<FileManager*>	_filters;
	BoundFont*				_tabLabelFont;
	int						_tabsAreaHeight;
	Canvas*					_currentBody;
	int						_tabHistory;		// maximum number of tabs allowed in group
};

class TabbedGroupHandler {
public:
	TabbedGroupHandler(TabbedGroup* group);

private:
	void onClick(MouseKeys mKeys, point p, Canvas* target);

	void onOpenContextMenu(point p, Canvas* c);

	void onHover(point p, Canvas* target);

	void onStartDrag(MouseKeys keys, point p, Canvas* target);

	void onDrag(MouseKeys keys, point p, Canvas* target);

	void onDrop(MouseKeys keys, point p, Canvas* target);

	void saveTab(point p, Canvas* c);

	void closeTab(point p, Canvas* c);

	void selectTab(point p, Canvas* c, TabManager* manager);

	TabManager* hitTest(point p, Canvas* c);

	TabManager*				_menuTab;
	TabbedGroup*			_group;
};

class TabManager {
	friend TabbedGroup;
public:
	TabManager() {
		_owner = null;
		_labelSize.setNoSize();
	}

	virtual ~TabManager();

	virtual bool deleteTab();

	virtual const char* tabLabel() = 0;

	virtual Canvas* tabBody() = 0;

	virtual void bind(TextEditor* editor);

	virtual void extendContextMenu(ContextMenu* menu);

	virtual Canvas* onHover();

	virtual bool needsSave();

	virtual bool save();

	virtual void reload();

	virtual bool hasErrors();

	virtual void clearAnnotations();

	// Base implementation:

	/*
	 *	tabModified
	 *
	 *	This method is called whenever the tab label or body
	 *	has changed appearance.
	 */
	void tabModified();

	TabbedGroup* owner() const { return _owner; }

	bool visible() const { return _visible; }

	int labelWidth() const;

	Event1<TabbedGroup*>	closed;				// fired when this tab gets removed (removing group passed in) 
	Event					selected;
	Event					unselected;

private:
	TabbedGroup*	_owner;
	dimension		_labelSize;
	bool			_visible;
};

class FileManager {
public:
	virtual ~FileManager();

	virtual bool matches(const string& filename) const = 0;

	virtual bool openFile(const string& filename) = 0;

	virtual void extendContextMenu(ContextMenu* menu);
};

} // namespace display
