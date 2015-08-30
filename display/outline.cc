#include "../common/platform.h"
#include "outline.h"

#include "device.h"
#include "scrollbar.h"

namespace display {

static Pen* outlineBoxPen = createPen(PS_SOLID, 1, 0);

Outline::Outline() {
	_itemTree = null;
	_scrollX = 0;
	_scrollY = 0;
}

Outline::~Outline() {
	delete _itemTree;
}

OutlineItem* Outline::locateOutlineItem(int ry) {
	if (_itemTree == null)
		return null;
	else
		return _itemTree->findItem(ry);
}

void Outline::recalculate(OutlineItem* oi) {
	jumble();
}

void Outline::flipExpansion(OutlineItem *oi) {
	if (oi->isExpanded)
		oi->setCollapsed();
	else
		oi->setExpanded();
	recalculate(oi);
}

dimension Outline::measure() {
	if (_itemTree != null) {
		dimension sz;

		_maximumWidth = 0;
		_maximumHeight = 0;
		sz.height = measureOutlineItem(_itemTree, 0, 0);
		sz.width = _maximumWidth;
		return sz;
	} else
		return dimension(0, 0);
}

int Outline::measureOutlineItem(OutlineItem* oi, int left, int top) {
	oi->relativeY = top;
	oi->itemSize = estimateMeasure(oi->canvas());
	if (oi->itemSize.height < OUTLINE_GLYPH_HEIGHT)
		oi->itemSize.height = OUTLINE_GLYPH_HEIGHT;
	if (oi->itemSize.height > _maximumHeight)
		_maximumHeight = oi->itemSize.height;
	top += oi->itemSize.height;
	left += OUTLINE_GLYPH_WIDTH + OUTLINE_SPACER;
	oi->relativeX = left;
	if (left + oi->itemSize.width > _maximumWidth)
		_maximumWidth = left + oi->itemSize.width;
	if (oi->isExpanded){
		for (OutlineItem* noi = oi->child; noi != null; noi = noi->sibling){
			top = measureOutlineItem(noi, left, top);
		}
	}
	return top;
}

void Outline::layout(dimension size) {
	if (_itemTree)
		placeOutlineItem(_itemTree);
	invalidate();
}

void Outline::placeOutlineItem(OutlineItem* oi) {
	if (_scrollY < oi->relativeY + oi->itemSize.height &&
		_scrollY + bounds.height() > oi->relativeY){
		point p;
		p.x = coord(bounds.topLeft.x + oi->relativeX - _scrollX);
		p.y = coord(bounds.topLeft.y + oi->relativeY - _scrollY);
		Canvas* oiCanvas = oi->canvas();
		if (oiCanvas->parent == null)
			append(oiCanvas);
		oiCanvas->at(p);
		dimension d = oi->itemSize;
		d.width = coord(_maximumWidth - oi->relativeX);
		oiCanvas->resize(d);
	} else if (oi->canvas()->parent == this)
		oi->canvas()->prune();
	if (oi->isExpanded){
		for (OutlineItem* noi = oi->child; noi != null; noi = noi->sibling){
			placeOutlineItem(noi);
		}
	}
}

bool Outline::isVisible(OutlineItem* oi) {
	return _scrollY < oi->relativeY + oi->itemSize.height &&
		   _scrollY + bounds.height() > oi->relativeY;
}

void Outline::paint(Device* b) {
	if (_itemTree != null)
		paintItem(_itemTree, b);
}

void Outline::paintItem(OutlineItem* oi, Device* b) {
	Canvas* oiCanvas = oi->canvas();
	if (oiCanvas->parent){
		b->set_pen(outlineBoxPen);
		point p = oiCanvas->bounds.topLeft;
		p.x -= OUTLINE_GLYPH_WIDTH + OUTLINE_SPACER;
		p.y += (oi->itemSize.height - OUTLINE_GLYPH_HEIGHT) >> 1;
		int lx = p.x + (OUTLINE_GLYPH_WIDTH / 2 - OUTLINE_GLYPH_WIDTH - OUTLINE_SPACER);
		int hy = p.y + OUTLINE_GLYPH_HEIGHT / 2;
		b->line(p.x, hy, lx, hy);
		int ey = oiCanvas->bounds.opposite.y;
		if (oi->sibling == null)
			ey = coord(hy + 1);
		b->line(lx, oiCanvas->bounds.topLeft.y, lx, ey);
		int plx = lx;
		for (OutlineItem* poi = oi->parent; poi != null; poi = poi->parent) {
			plx -= OUTLINE_GLYPH_WIDTH + OUTLINE_SPACER;
			if (poi->sibling != null)
				b->line(plx, oiCanvas->bounds.topLeft.y, plx,
						oiCanvas->bounds.opposite.y);
		}
		if (oi->child != null || oi->canExpand) {
			point opp = p;
			opp.x += OUTLINE_GLYPH_WIDTH - 1;
			opp.y += OUTLINE_GLYPH_HEIGHT - 1;
			b->line(p.x, p.y, opp.x, p.y);
			b->line(opp.x, p.y, opp.x, opp.y);
			b->line(opp.x, opp.y, p.x, opp.y);
			b->line(p.x, opp.y, p.x, p.y);
			b->line(p.x + 2, hy, opp.x - 1, hy);
			int hx = p.x + OUTLINE_GLYPH_WIDTH / 2;
			if (!oi->isExpanded) {
				b->line(hx, p.y + 2, hx, opp.y - 1);
			} else {
				lx += OUTLINE_GLYPH_WIDTH + OUTLINE_SPACER;
				b->line(hx, opp.y + 1, hx, oiCanvas->bounds.opposite.y);
			}
		}
	}
	if (oi->isExpanded) {
		for (OutlineItem* noi = oi->child; noi != null; noi = noi->sibling)
			paintItem(noi, b);
	}
}

void Outline::vScrollNotice(int i) {
	vScroll(i);
}

void Outline::hScrollNotice(int i) {
	hScroll(i);
}

int Outline::vScroll(int i) {
	if (_scrollY != i) {
		_scrollY = i;
		jumble();
		return 1;
	}
	return 0;
}

int Outline::hScroll(int i) {
	if (_scrollX != i) {
		_scrollX = i;
		jumble();
		return 1;
	}
	return 0;
}

void Outline::set_itemTree(OutlineItem* oi) {
	_itemTree = oi;
	jumble();
}

OutlineItem::OutlineItem(display::Outline *o, display::Canvas *c) {
	_outline = o;
	_canvas = c;
	parent = null;
	child = null;
	sibling = null;
	isExpanded = false;
	hasEverExpanded = false;
	canExpand = false;
	relativeY = 0;
	relativeX = 0;
}

OutlineItem::~OutlineItem() {
	extract();
	while (child != null) {
		OutlineItem* oi = child;
		child = child->sibling;
		delete oi;
	}
	_canvas->prune();
	delete _canvas;
}

Canvas* OutlineItem::setCanvas(Canvas* c) {
	Canvas* oldC = _canvas;
	_canvas->prune();
	_canvas = c;
	if (isExpanded)
		_outline->recalculate(this);
	return oldC;
}

bool OutlineItem::setExpanded() {
	bool didSomething = false;
	if (parent != null)
		didSomething = parent->setExpanded();
	if (!hasEverExpanded){
		hasEverExpanded = true;
		firstExpand.fire();
	}
	if (!isExpanded){
		isExpanded = true;
		expand.fire();
		didSomething = true;
	}
	return didSomething;
}

void OutlineItem::setCollapsed() {
	if (isExpanded){
		isExpanded = false;
		collapse.fire();
		pruneChildCanvases();
	}
}

void OutlineItem::append(OutlineItem* c) {
	c->parent = this;
	c->sibling = null;
	if (child == null)
		child = c;
	else {
		OutlineItem* s;
		for (s = child; s->sibling != null; s = s->sibling)
			;
		s->sibling = c;
	}
	if (isExpanded || child == c)
		_outline->recalculate(this);
}

void OutlineItem::insertAfter(OutlineItem* c) {
	c->parent = parent;
	c->sibling = sibling;
	sibling = c;
	if (parent != null &&
		parent->isExpanded)
		_outline->recalculate(parent);
}

void OutlineItem::insertFirst(OutlineItem* c) {
	c->parent = this;
	c->sibling = child;
	child = c;
	if (isExpanded || c->sibling == null)
		_outline->recalculate(this);
}

OutlineItem* OutlineItem::previousSibling() {
	if (parent == null)
		return null;
	OutlineItem* c = parent->child;
	if (c == this)
		return null;
	while (c->sibling != this)
		c = c->sibling;
	return c;
}

void OutlineItem::extract() {

		// Only do anything if this OutlineItem has a parent

	if (parent != null) {
		OutlineItem* pc = null;
		for (OutlineItem* c = parent->child; c != null; c = c->sibling) {
			if (c == this) {
				if (pc != null)
					pc->sibling = sibling;
				else
					parent->child = sibling;
				if (parent->isExpanded || parent->child == null)
					_outline->recalculate(parent);
				parent = null;
				sibling = null;
				return;
			}
			pc = c;
		}

			// if we get here, the OutlineItem tree is corrupted

	}
}

void OutlineItem::expose() {
	if (parent == null)
		return;
	// If any parent changed state, recalculate the layout
	if (parent->setExpanded())
		_outline->jumble();
	_outline->expose.fire(this);
}

rectangle OutlineItem::coveredArea() {
	rectangle r;

	_outline->preferredSize();
	r.topLeft.x = relativeX;
	r.topLeft.y = relativeY;
	r.opposite.x = relativeX + itemSize.width;
	r.opposite.y = relativeY + itemSize.height;
	return r;
}

int OutlineItem::subtreeHeight() {
	int h = itemSize.height;
	if (isExpanded){
		for (OutlineItem* noi = child; noi != null; noi = noi->sibling){
			h += noi->subtreeHeight();
		}
	}
	return h;
}

	/*
		Note: when a series of expand and collapse operations occur,
		you may collapse several layers of the outline in one click.
		The collapse event is fired for each nested layer that must be
		removed.  Subsequently, re-expanding a layer leaves sub-layers
		collapsed.

		Note: The timing of expand and collapse events allow for lazy
		instantiation of OutlineItem's.  Essentially, an OutlineItem
		expand event can trigger code that creates or adjusts the set of
		OutlineItem's.  The collapse event occurs before pruning children,
		so inspecting the Canvas tree is fine inside the collapse event
		handler.
	 */
OutlineItem* OutlineItem::findItem(int ry) {
	if (relativeY > ry)
		return null;
	OutlineItem* oi = this;
	if (isExpanded) {
		for (OutlineItem* noi = child; noi != null; noi = noi->sibling){
			OutlineItem* roi = noi->findItem(ry);
			if (roi == null)
				break;
			oi = roi;
		}
	}
	return oi;
}

void OutlineItem::pruneChildCanvases() {
	for (OutlineItem* oi = child; oi != null; oi = oi->sibling) {
		oi->canvas()->prune();
		oi->pruneChildCanvases();
	}
}

OutlineHandler::OutlineHandler(Outline *viewport) {
	_viewport = viewport;
	_clickHandler = _viewport->click.addHandler(this, &OutlineHandler::onClick);
	_exposeHandler = _viewport->expose.addHandler(this, &OutlineHandler::onExpose);
}

OutlineHandler::~OutlineHandler() {
	_viewport->click.removeHandler(_clickHandler);
	_viewport->expose.removeHandler(_exposeHandler);
}

void OutlineHandler::onClick(display::MouseKeys mKeys, display::point p, display::Canvas *target) {
	int rx = p.x - _viewport->bounds.topLeft.x;
	int ry = p.y - _viewport->bounds.topLeft.y;
	OutlineItem* oi = _viewport->locateOutlineItem(ry);
	if (oi->child != null || oi->canExpand){
		int left = oi->relativeX - (OUTLINE_GLYPH_WIDTH + OUTLINE_SPACER);
		if (rx < left)
			return;
		if (rx >= left + OUTLINE_GLYPH_WIDTH)
			return;
		int top = oi->relativeY + ((oi->itemSize.height - OUTLINE_GLYPH_HEIGHT) >> 1);
		if (ry < top)
			return;
		if (ry >= top + OUTLINE_GLYPH_HEIGHT)
			return;
		_viewport->flipExpansion(oi);
	}
}

void OutlineHandler::onExpose(OutlineItem* oi) {
	rectangle r = oi->coveredArea();
//	_canvas->align(r, 0.5, 0);
}

}  // namespace display