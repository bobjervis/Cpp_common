#pragma once
#include "../common/vector.h"
#include "root_canvas.h"

namespace display {

class Choice;
class ContextMenuItem;
class ContextMenuLabel;
class PullRight;

const int CONTEXT_EDGE = 2;
/*
 * A ContextMenu is a pop-up menu that should be created and shown whenever 
 * an openContextMenu event is fired by a canvas.  The Handler for a canvas 
 * should create the ContextMenu object, populate it and show it.  When a
 * choice is made, the returned Choice click event will fire.  If no choice is made,
 * the menu will be closed and idscarded and no further notice is issued.
 */
class ContextMenu : public RootCanvas {
	typedef RootCanvas super;

	friend ContextMenuLabel;
public:

	ContextMenu(RootCanvas* w, point anchor, Canvas* target);

	ContextMenu(ContextMenu* parent, vector<ContextMenuItem*>& items, Canvas* lbl);

	~ContextMenu();

	Choice* choice(const string& tag);

	ContextMenuItem* item(int i) const { return _items[i]; }

	PullRight* pullRight(const string& tag);

	virtual void show();

	virtual void hide();

	void hideAllMenus();

	void dismiss();

	void select(int i);

	void deselect();

	Background* hilightedBackground;
	Color* hilightedTextColor;
private:
	void init(point anchor, RootCanvas* parent, ContextMenu* parentMenu, Canvas* target);

	void onMouseMove(point p, Canvas* target);

	void onDrag(MouseKeys mKeys, point p, Canvas* target);

	void onClick(MouseKeys mKeys, point p, Canvas* target);

	void onDrop(MouseKeys mKeys, point p, Canvas* target);

	void emulateStartDrag(point p);

	void becomeModal();

	void shutDown();

	// Returns true if the given Canvas c is in the lbelGrid that was built for
	// this menu.
	bool isInGrid(Canvas* c);

	RootCanvas*					parent;
	ContextMenu*				parentMenu;
	point						_anchor;
	point						_topLeft;
	Canvas*						target;
	BoundFont*					_symbolFont;
	int							choiceCount;
	int							maxWidth;
	int							lineHeight;
	Grid*						labelGrid;
	vector<ContextMenuItem*>	_items;
	vector<ContextMenuLabel*>	_labels;
	int							_selected;
	bool						_itemsCloned;
};

class ContextMenuItem {
public:
	virtual ~ContextMenuItem();
	/*
	 * return of true indicates that parent ContextMenu should
	 * remain open, false indicates that the menu should close.
	 */
	virtual void fireClick(point p, Canvas* target);

	virtual void select(ContextMenu* cm, Canvas* lbl);

	virtual void deselect();

	virtual bool isPullRight();

	string					tag;
};

class Choice : public ContextMenuItem {
public:
	Choice(const string& tag);

	virtual void fireClick(point p, Canvas* target);

	void fireButtonClick(Bevel* b);

	Event2<point, Canvas*>	 click;
};

class PullRight : public ContextMenuItem {
public:
	PullRight(RootCanvas* parent, const string& tag);

	~PullRight();

	virtual void select(ContextMenu* cm, Canvas* lbl);

	virtual void deselect();

	virtual bool isPullRight();

	Choice* choice(const string& tag);

	PullRight* pullRight(const string& tag);

private:
	RootCanvas*					_parent;
	ContextMenu*				_subMenu;
	vector<ContextMenuItem*>	_items;
};

}  // namespace display