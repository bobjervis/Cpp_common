#pragma once
#include "canvas.h"
#include "measurement.h"

namespace display {

class ScrollableCanvas;
class ScrollableCanvasHandler;

const coord OUTLINE_GLYPH_HEIGHT = 9;
const coord OUTLINE_GLYPH_WIDTH  = 9;
const coord OUTLINE_SPACER  = 2;

class Outline : public Canvas {
	typedef Canvas super;
public:
	Outline();

	~Outline();

	OutlineItem* locateOutlineItem(int ry);

	void recalculate(OutlineItem* oi);

	void flipExpansion(OutlineItem* oi);

	virtual dimension measure();

	virtual void layout(dimension size);

	virtual void paint(Device* b);
	/*
	 *	paintItem
	 *
	 *	Paints the outline bars and plus-minus boxes
	 *	for the entire outline.  The individual items
	 *	are painted by their respective Canvas objects.
	 */
	void paintItem(OutlineItem* oi, Device* b);

	OutlineItem* itemTree() const { return _itemTree; }

	void set_itemTree(OutlineItem* oi);

	void hScrollNotice(int i);

	void vScrollNotice(int i);

	Event2<int, int>	viewportMoved;

	Event1<OutlineItem*> expose;

private:
	int measureOutlineItem(OutlineItem* oi, int left, int top);

	void placeOutlineItem(OutlineItem* oi);

	bool isVisible(OutlineItem* oi);

	int hScroll(int i);

	int vScroll(int i);

	OutlineItem*			_itemTree;
	int						_maximumHeight;
	int						_maximumWidth;
	coord					_scrollX;
	coord					_scrollY;
};

class OutlineItem {
public:
	OutlineItem(Outline* o, Canvas* c);

	~OutlineItem();
	/*
	 *	setCanvas
	 *
	 *	This function sets the canvas for this outline item to c and
	 *	returns the previous value of the canvas.  Note that the OutlineItem
	 *	will delete its own canvas, so you are responsible for the returned
	 *	canvas (which has been pruned, and so is not visible).
	 */
	Canvas* setCanvas(Canvas* c);
	/*
	 *	setExpanded
	 *
	 *	This function sets the given OutlineItem and all of its direct parents
	 *	to be expanded.  It returns true if any parent changed state.
	 */
	bool setExpanded();

	void setCollapsed();
	/*
	 * append
	 *
	 *	This function appends a new OutlineItem to the list of children
	 *	of the current item.
	 */
	void append(OutlineItem* c);
	/*
	 * insertAfter
	 *
	 * This function inserts a new OutlineItem to become a new sibling
	 * of the current item
	 */
	void insertAfter(OutlineItem* c);

	void insertFirst(OutlineItem* c);

	OutlineItem* previousSibling();

	void extract();
	/*
	 * expose
	 *
	 *	This function will expand this item and all parent items that
	 *	are currently collapsed.
	 */
	void expose();

	/*
	 *	coveredArea
	 *
	 *	For the current expansion state of the outline, where does
	 *	the current item's canvas appear?
	 *
	 *	Returns a rectangle, relative to the outline coordinates of
	 *	the OutlineItem's canvas area.
	 */
	rectangle coveredArea();

	int subtreeHeight();

	OutlineItem* findItem(int ry);

	Event			firstExpand;	// Fired the first time an item is ever expanded

	Event			expand;			// Fired before an item is expanded

	Event			collapse;		// Fired after an item is collapsed

	/*
		Note: when a series of expand and collapse operations occur,
		you may collapse several layers of the outline in one click.
		The collapse event is fired for each nested layer that must be
		removed.  Subsequently, re-expanding a layer leaves sub-layers
		collapsed.

		Note: The timing of expand and collapse events allow for lazy
		instantiation of OutlineItem's.  Essentially, an OutlineItem
		expand event can trigger code that creates or adjusts the set of
		OutlineItem's.  
	 */
	OutlineItem*	parent;
	OutlineItem*	child;
	OutlineItem*	sibling;
	bool			isExpanded;
	bool			hasEverExpanded;
	bool			canExpand;
	dimension		itemSize;
	int				relativeY;
	int				relativeX;

	Canvas* canvas() const { return _canvas; }
private:
	void pruneChildCanvases();

	Outline*		_outline;
	Canvas*			_canvas;
};

class OutlineHandler {
public:
	OutlineHandler(Outline* viewport);

	~OutlineHandler();

private:
	void onClick(MouseKeys mKeys, point p, Canvas* target);

	void onExpose(OutlineItem* oi);

	Outline*					_viewport;
	void*						_clickHandler;
	void*						_exposeHandler;
};

}  // namespace display