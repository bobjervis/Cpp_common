#include "../common/platform.h"
#include "scrollbar.h"

#include <typeinfo.h>
#include "background.h"
#include "device.h"
#include "label.h"

namespace display {

ScrollableCanvas::ScrollableCanvas() {
	_vertical = new VerticalScrollBar();
	_horizontal = new HorizontalScrollBar();
}

ScrollableCanvas::~ScrollableCanvas() {
	_vertical->prune();
	_horizontal->prune();
	delete _vertical;
	delete _horizontal;
}

void ScrollableCanvas::childJumbled(Canvas* child) {
	jumbleContents();
}

bool ScrollableCanvas::bufferedDisplay(Device* device) {
	return true;
}

dimension ScrollableCanvas::measure() {
	if (child) {
		dimension area = child->preferredSize();
		_vertical->maximum = area.height;
		_horizontal->maximum = area.width;
		return area;
	} else
		return dimension(0, 0);
}

void ScrollableCanvas::layout(dimension size) {
	Canvas* client;
	for (client = child; client != null; client = client->sibling) {
		if (typeid(*client) == typeid(VerticalScrollBar))
			continue;
		if (typeid(*client) == typeid(HorizontalScrollBar))
			continue;
		break;
	}
	if (client == null)
		return;
	dimension area = client->preferredSize();
	bool hasVbar = _vertical->parent != null;
	bool hasHbar = _horizontal->parent != null;
	bool needsVbar;
	bool needsHbar;
	if (area.width <= size.width && area.height <= size.height) {
		needsVbar = false;
		needsHbar = false;
	} else if (area.width <= size.width - SCROLLBAR_WIDTH) {
		needsVbar = true;
		needsHbar = false;
	} else if (area.height <= size.height - SCROLLBAR_WIDTH) {
		needsVbar = false;
		needsHbar = true;
	} else {
		needsVbar = true;
		needsHbar = true;
	}
	int scrollY;
	int scrollX;

		// Allow space for any horizontal scroll bar

	if (needsHbar)
		size.height -= SCROLLBAR_WIDTH;

		// Now insert or update any vertical bar

	if (needsVbar){
		if (!hasVbar) {
			append(_vertical);
			scrollY = 0;
		} else
			scrollY = _vertical->value();
		if (scrollY > area.height - size.height)
			scrollY = area.height - size.height;
		size.width -= SCROLLBAR_WIDTH;
		_vertical->sliderSize = size.height;
		_vertical->maximum = area.height;
		_vertical->set_value(scrollY);
		point p = bounds.topLeft;
		p.x += size.width;
		_vertical->at(p);
		dimension sz2 = size;
		sz2.width = SCROLLBAR_WIDTH;
		_vertical->resize(sz2);
	} else {
		scrollY = 0;
		_vertical->set_value(scrollY);
		if (hasVbar)
			_vertical->prune();
	}
	if (needsHbar){
		if (!hasHbar) {
			append(_horizontal);
			scrollX = 0;
		} else
			scrollX = _horizontal->value();
		if (scrollX > area.width - size.width)
			scrollX = area.width - size.width;
		_horizontal->sliderSize = size.width;
		_horizontal->maximum = area.width;
		_horizontal->set_value(scrollX);
		point p = bounds.topLeft;
		p.y += size.height;
		_horizontal->at(p);
		dimension sz2 = size;
		sz2.height = SCROLLBAR_WIDTH;
		_horizontal->resize(sz2);
	} else {
		scrollX = 0;
		_horizontal->set_value(scrollX);
		if (hasHbar)
			_horizontal->prune();
	}
	point corner(bounds.topLeft.x - scrollX, bounds.topLeft.y - scrollY);
	client->at(corner);
	int h;
	if (needsHbar)
		h = bounds.height() - SCROLLBAR_WIDTH;
	else
		h = bounds.height();
	if (area.height < h)
		area.height = h;
	int w;
	if (needsVbar)
		w = bounds.width() - SCROLLBAR_WIDTH;
	else
		w = bounds.width();
	if (area.width < w)
		area.width = w;
	client->resize(area);
}

void ScrollableCanvas::expose(const rectangle& r) {
	if (jumbled())
		measure();
	dimension focusSize = r.size();
	int cornerX = r.topLeft.x - child->bounds.topLeft.x;
	int cornerY = r.topLeft.y - child->bounds.topLeft.y;
	// First arrange the horizontal scroll
	if (_horizontal->parent) {
		if (focusSize.width >= _horizontal->sliderSize ||
			cornerX < _horizontal->value())
			hScroll(cornerX);
		else {
			int min = cornerX + focusSize.width - _horizontal->sliderSize;
			if (_horizontal->value() < min)
				hScroll(min);
		}
	}
	if (_vertical->parent) {
		if (focusSize.height >= _vertical->sliderSize ||
			cornerY < _vertical->value())
			vScroll(cornerY);
		else {
			int min = cornerY + focusSize.height - _vertical->sliderSize;
			if (_vertical->value() < min)
				vScroll(min);
		}
	}
}

void ScrollableCanvas::align(const rectangle& r, float vertical, float horizontal) {
	if (jumbled())
		measure();
	dimension totalSize = child->preferredSize();
	dimension viewportSize = child->bounds.size();
	dimension focusSize = r.size();
	point focusReference(horizontal * focusSize.width, vertical * focusSize.height);
	point viewportReference(horizontal * viewportSize.width, vertical * viewportSize.height);
	hScroll(r.topLeft.x + focusReference.x - viewportReference.x);
	vScroll(r.topLeft.y + focusReference.y - viewportReference.y);
}

void ScrollableCanvas::hScroll(int scrollX) {
	if (child) {
		if (scrollX < 0)
			scrollX = 0;
		if (scrollX > _horizontal->maximum - _horizontal->sliderSize)
			scrollX = _horizontal->maximum - _horizontal->sliderSize;
		if (scrollX == _horizontal->value())
			return;
		_horizontal->set_value(scrollX);
	}
}

void ScrollableCanvas::vScroll(int scrollY) {
	if (child) {
		if (scrollY < 0)
			scrollY = 0;
		if (scrollY > _vertical->maximum - _vertical->sliderSize)
			scrollY = _vertical->maximum - _vertical->sliderSize;
		if (scrollY == _vertical->value())
			return;
		_vertical->set_value(scrollY);
	}
}

void ScrollableCanvas::hScrollNotice(int scrollX) {
	if (child && _horizontal->parent != null) {
		point corner(bounds.topLeft.x - scrollX, bounds.topLeft.y);
		child->at(corner);
		child->jumbleContents();
	}
}

void ScrollableCanvas::vScrollNotice(int scrollY) {
	if (child && _vertical->parent != null) {
		point corner(bounds.topLeft.x, bounds.topLeft.y - scrollY);
		child->at(corner);
		child->jumbleContents();
	}
}

bool ScrollableCanvas::usesVerticalBar() {
	for (Canvas* ch = child; ch != null; ch = ch->sibling)
		if (ch == _vertical)
			return true;
	return false;
}

bool ScrollableCanvas::usesHorizontalBar() {
	for (Canvas* ch = child; ch != null; ch = ch->sibling)
		if (ch == _horizontal)
			return true;
	return false;
}

VerticalScrollBar::VerticalScrollBar() {
	updateSlider();
}

dimension VerticalScrollBar::measure() {
	dimension sz;
	sz.width = SCROLLBAR_WIDTH;
	sz.height = coord(SCROLLBAR_WIDTH*2);
	return sz;
}

void VerticalScrollBar::layout(dimension size) {
	super::layout(size);
	negativeButton->at(bounds.topLeft);
	point p = bounds.topLeft;
	p.y = coord(bounds.opposite.y - SCROLLBAR_WIDTH);
	positiveButton->at(p);
	updateSlider();
}

void VerticalScrollBar::updateSlider() {
	dimension sz = bounds.size();
	int h = sz.height - 2 * SCROLLBAR_WIDTH;
	point p = bounds.topLeft;
	if (maximum <= minimum){
		sz.height = 0;
	} else {
		p.y += SCROLLBAR_WIDTH + (h * _value) / (maximum - minimum);
		sz.height = coord((h * sliderSize) / (maximum - minimum));
		if (sz.height < 2)
			sz.height = 2;
	}
	slider->at(p);
	slider->resize(sz);
	smallerTray = bounds;
	largerTray = bounds;
	largerTray.opposite.y -= SCROLLBAR_WIDTH;
	smallerTray.topLeft.y += SCROLLBAR_WIDTH;
	smallerTray.opposite.y = slider->bounds.top();
	largerTray.topLeft.y = slider->bounds.bottom();
}


HorizontalScrollBar::HorizontalScrollBar() {
	negativeLabel->set_value(SYMBOL_LEFT_TRIANGLE);
	positiveLabel->set_value(SYMBOL_RIGHT_TRIANGLE);
	updateSlider();
}

dimension HorizontalScrollBar::measure() {
	return dimension(SCROLLBAR_WIDTH*2, SCROLLBAR_WIDTH);
}

void HorizontalScrollBar::layout(dimension size) {
	super::layout(size);
	negativeButton->at(bounds.topLeft);
	point p = bounds.topLeft;
	p.x = coord(bounds.opposite.x - SCROLLBAR_WIDTH);
	positiveButton->at(p);
	updateSlider();
}

void	HorizontalScrollBar::updateSlider() {
	dimension sz = bounds.size();
	int w = sz.width - 2 * SCROLLBAR_WIDTH;
	point p = bounds.topLeft;
	if (maximum <= minimum){
		sz.width = 0;
	} else {
		p.x += SCROLLBAR_WIDTH + (w * _value) / (maximum - minimum);
		sz.width = coord((w * sliderSize) / (maximum - minimum));
		if (sz.width < 2)
			sz.width = 2;
	}
	slider->at(p);
	slider->resize(sz);
	smallerTray = bounds;
	largerTray = bounds;
	largerTray.opposite.x -= SCROLLBAR_WIDTH;
	smallerTray.topLeft.x += SCROLLBAR_WIDTH;
	smallerTray.set_right(slider->bounds.left());
	largerTray.set_left(slider->bounds.right());
}

ScrollBar::ScrollBar() {
	negativeButton = new Bevel(2);
	positiveButton = new Bevel(2);
	slider = new Bevel(2);
	append(negativeButton);
	append(slider);
	append(positiveButton);
	negativeLabel = new Label(SYMBOL_UP_TRIANGLE, symbolFont());
	positiveLabel = new Label(SYMBOL_DOWN_TRIANGLE, symbolFont());
	negativeLabel->set_format(DT_CENTER|DT_VCENTER);
	positiveLabel->set_format(DT_CENTER|DT_VCENTER);
	negativeButton->append(negativeLabel);
	positiveButton->append(positiveLabel);
	setBackground(&scrollbarBackground);
	smallerTray = bounds;
	largerTray = bounds;
	minimum = 0;
	maximum = 0;
	sliderSize = 0;
	_value = 0;
}

ScrollBar::~ScrollBar() {
}

void ScrollBar::set_value(int v) {
	if (v != _value) {
		rectangle r;
		
		_value = v;
		updateSlider();
		r.topLeft = smallerTray.topLeft;
		r.opposite = largerTray.opposite;
		invalidate(r);
		changed.fire(v);
	}
}

void ScrollBar::layout(dimension size) {
	dimension d(SCROLLBAR_WIDTH, SCROLLBAR_WIDTH);
	negativeButton->resize(d);
	positiveButton->resize(d);
}

void ScrollBar::updateSlider() {
}

ScrollBarHandler::ScrollBarHandler(VerticalScrollBar* vBar, HorizontalScrollBar* hBar, Canvas* client) {
	this->vBar = vBar;
	this->hBar = hBar;
	this->client = client;
	vBar->disableDoubleClick = true;
	hBar->disableDoubleClick = true;
	vnegHandler = new ButtonHandler(vBar->negativeButton, 0);
	vposHandler = new ButtonHandler(vBar->positiveButton, 0);
	hnegHandler = new ButtonHandler(hBar->negativeButton, 0);
	hposHandler = new ButtonHandler(hBar->positiveButton, 0);
	vBar->settingChange.addHandler(this, &ScrollBarHandler::recalcRepeatInterval);
	recalcRepeatInterval();
	vnegHandler->click.addHandler(this, &ScrollBarHandler::vSmallUpMove);
	vposHandler->click.addHandler(this, &ScrollBarHandler::vSmallDownMove);
	hnegHandler->click.addHandler(this, &ScrollBarHandler::hSmallLeftMove);
	hposHandler->click.addHandler(this, &ScrollBarHandler::hSmallRightMove);
	vBar->click.addHandler(this, &ScrollBarHandler::vBigMove);
	hBar->click.addHandler(this, &ScrollBarHandler::hBigMove);
	if (client)
		client->mouseWheel.addHandler(this, &ScrollBarHandler::vMouseWheel);
	vBar->mouseWheel.addHandler(this, &ScrollBarHandler::vMouseWheel);
	hBar->mouseWheel.addHandler(this, &ScrollBarHandler::hMouseWheel);
	vBar->startDrag.addHandler(this, &ScrollBarHandler::vStartDrag);
	vBar->drag.addHandler(this, &ScrollBarHandler::vDrag);
	vBar->drop.addHandler(this, &ScrollBarHandler::vDrop);
	hBar->startDrag.addHandler(this, &ScrollBarHandler::hStartDrag);
	hBar->drag.addHandler(this, &ScrollBarHandler::hDrag);
	hBar->drop.addHandler(this, &ScrollBarHandler::hDrop);
	hSmallIncrement = 0;
	vSmallIncrement = 0;
}

ScrollBarHandler::~ScrollBarHandler() {
	delete vnegHandler;
	delete vposHandler;
	delete hnegHandler;
	delete hposHandler;
}

void ScrollBarHandler::newClient(Canvas* client) {
	this->client = client;
	if (client)
		client->mouseWheel.addHandler(this, &ScrollBarHandler::vMouseWheel);
}

void ScrollBarHandler::recalcRepeatInterval() {
	int repeatInterval = keyboardRepeatInterval();
	vnegHandler->set_repeatInterval(repeatInterval);
	vposHandler->set_repeatInterval(repeatInterval);
	hnegHandler->set_repeatInterval(repeatInterval);
	hposHandler->set_repeatInterval(repeatInterval);
}

void ScrollBarHandler::vSmallUpMove(Bevel* b) {
	vScroll(vBar->value() - vSmall());
}

void ScrollBarHandler::vSmallDownMove(Bevel* b) {
	vScroll(vBar->value() + vSmall());
}

void ScrollBarHandler::vBigUpMove() {
	vScroll(vBar->value() - vBar->sliderSize);
}

void ScrollBarHandler::vBigDownMove() {
	vScroll(vBar->value() + vBar->sliderSize);
}

void ScrollBarHandler::vBigMove(MouseKeys mKeys, point p, Canvas* target) {
	if (vBar->smallerTray.contains(p))
		vBigUpMove();
	else if (vBar->largerTray.contains(p))
		vBigDownMove();
}

void ScrollBarHandler::vMouseWheel(MouseKeys mKeys, point p, int zDelta, Canvas* target) {
	vScroll(vBar->value() - (vSmall() * zDelta) / 120);
}

void ScrollBarHandler::hMouseWheel(MouseKeys mKeys, point p, int zDelta, Canvas* target) {
	hScroll(hBar->value() - (hSmall() * zDelta) / 120);
}

void ScrollBarHandler::vStartDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (vBar->slider->bounds.contains(p)){
		vDragging = true;
		anchor = p.y;
		interval = vBar->slider->bounds.height();
		baseValue = vBar->value();
	}
}

void ScrollBarHandler::vDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (vDragging){
		vScroll(baseValue + ((p.y - anchor) * vBar->sliderSize) / interval);
	}
}

void ScrollBarHandler::vDrop(MouseKeys mKeys, point p, Canvas* target) {
	vDragging = false;
}

void ScrollBarHandler::hStartDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (hBar->slider->bounds.contains(p)){
		hDragging = true;
		anchor = p.x;
		interval = hBar->slider->bounds.width();
		baseValue = hBar->value();
	}
}

void ScrollBarHandler::hDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (hDragging)
		hScroll(baseValue + ((p.x - anchor) * hBar->sliderSize) / interval);
}

void ScrollBarHandler::hDrop(MouseKeys mKeys, point p, Canvas* target) {
	hDragging = false;
}

void ScrollBarHandler::vScroll(int nv) {
	if (nv < vBar->minimum)
		nv = vBar->minimum;
	else if (nv > vBar->maximum - vBar->sliderSize)
		nv = vBar->maximum - vBar->sliderSize;
	vBar->set_value(nv);
}

void ScrollBarHandler::hSmallLeftMove(Bevel* b) {
	hScroll(hBar->value() - hSmall());
}

void ScrollBarHandler::hSmallRightMove(Bevel* b) {
	hScroll(hBar->value() + hSmall());
}

void ScrollBarHandler::hBigMove(MouseKeys mKeys, point p, Canvas* target) {
	if (hBar->smallerTray.contains(p))
		hScroll(hBar->value() - hBar->sliderSize);
	else if (hBar->largerTray.contains(p))
		hScroll(hBar->value() + hBar->sliderSize);
}

void ScrollBarHandler::hScroll(int nv) {
	if (nv < hBar->minimum)
		nv = hBar->minimum;
	else if (nv > hBar->maximum - hBar->sliderSize)
		nv = hBar->maximum - hBar->sliderSize;
	hBar->set_value(nv);
}

ScrollableCanvasHandler::ScrollableCanvasHandler(ScrollableCanvas* canvas)
	: ScrollBarHandler(canvas->vertical(), canvas->horizontal(), canvas->child) {
	_canvas = canvas;
	canvas->vertical()->changed.addHandler(canvas, &ScrollableCanvas::vScrollNotice);
	canvas->horizontal()->changed.addHandler(canvas, &ScrollableCanvas::hScrollNotice);
};

int keyboardRepeatInterval() {
	unsigned value;

	if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &value, 0) != 0)
		return 407 - 12 * value;
	else
		return 0;
}

/*
VerticalTrackBar: public type inherits TrackBar {
	new: ()
	{
		super()
	}

	runtimeCalculateSize: ()
	{
		preferredSize.width = TRACKBAR_WIDTH
		preferredSize.height = coord(2 * SLIDER_MARGIN + 1)
	}

	resize: (sz: dimension)
	{
		bounds.size = sz
		updateSlider()
	}

	at: (p: point)
	{
		super.at(p)
		updateSlider()
	}

	updateSlider: ()
	{
		sz := bounds.size
		h := sz.height - (2 * SLIDER_MARGIN + 1)
		if (h > 0)
			intervalPerPixel = (maximum - minimum) / h
		else
			intervalPerPixel = 0
		p := bounds.topLeft
		if (maximum > minimum)
			p.y += (h * xvalue) / (maximum - minimum)
		slider.at(p)
		smallerTray = bounds
		largerTray = bounds
		smallerTray.bottom = slider.bounds.top
		largerTray.top = slider.bounds.bottom
	}

	anchor: (p: point) int
	{
		return p.y
	}
}
 */
dimension HorizontalTrackBar::measure() {
	return dimension(2 * SLIDER_MARGIN + 1, TRACKBAR_WIDTH);
}

void HorizontalTrackBar::layout(dimension size) {
	size.width = 2 * SLIDER_MARGIN;
	slider->resize(size);
	updateSlider();
}

void HorizontalTrackBar::updateSlider() {
	dimension sz = bounds.size();
	int w = sz.width - (2 * SLIDER_MARGIN + 1);
	if (w > 0)
		intervalPerPixel = float(maximum - minimum) / w;
	else
		intervalPerPixel = 0;
	point p = bounds.topLeft;
	if (maximum > minimum)
		p.x += (w * _value) / (maximum - minimum);
	slider->at(p);
	smallerTray = bounds;
	largerTray = bounds;
	smallerTray.opposite.x = slider->bounds.topLeft.x;
	largerTray.topLeft.x = slider->bounds.opposite.x;
}

int HorizontalTrackBar::anchor(point p) {
	return p.x;
}

TrackBar::TrackBar() {
	slider = new Bevel(SLIDER_MARGIN);
	append(slider);
	setBackground(&scrollbarBackground);
	smallerTray = bounds;
	largerTray = bounds;
	_value = 0;
	intervalPerPixel = 0;
	minimum = 0;
	maximum = 0;
	clickIncrement = 0;
}

dimension TrackBar::doMeasure() {
	return dimension(0, TRACKBAR_WIDTH);
}

void TrackBar::set_value(int v) {
	_value = v;
	updateSlider();
	invalidate();
}

void TrackBar::updateSlider() {
}

int TrackBar::anchor(point p) {
	return 0;
}

TrackBarHandler::TrackBarHandler(TrackBar* bar) {
	this->bar = bar;
	bar->disableDoubleClick = true;
	bar->click.addHandler(this, &TrackBarHandler::bigMove);
	bar->mouseWheel.addHandler(this, &TrackBarHandler::mouseWheel);
	bar->startDrag.addHandler(this, &TrackBarHandler::bStartDrag);
	bar->drag.addHandler(this, &TrackBarHandler::bDrag);
	bar->drop.addHandler(this, &TrackBarHandler::bDrop);
	bar->buttonDown.addHandler(this, &TrackBarHandler::bDown);
	dragging = false;
	hDragging = false;
	anchor = 0;
	interval = 0;
	baseValue = 0;
	anchorPoint.x = 0;
	anchorPoint.y = 0;
}

void TrackBarHandler::bigMove(MouseKeys mKeys, point p, Canvas* target) {
	if (bar->smallerTray.contains(p))
		newValue(bar->value() - bar->clickIncrement);
	else if (bar->largerTray.contains(p))
		newValue(bar->value() + bar->clickIncrement);
}

void TrackBarHandler::mouseWheel(MouseKeys mKeys, point p, int zDelta, Canvas* target) {
	newValue(bar->value() - (bar->clickIncrement * zDelta) / 120);
}

void TrackBarHandler::bDown(MouseKeys mKeys, point p, Canvas* target) {
	anchorPoint = p;
	anchor = coord(bar->anchor(p));
	baseValue = bar->value();
}

void TrackBarHandler::bStartDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (bar->slider->bounds.contains(anchorPoint))
		dragging = true;
}

void TrackBarHandler::bDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (dragging){
		newValue(baseValue + int((bar->anchor(p) - anchor) * bar->intervalPerPixel));
	}
}

void TrackBarHandler::bDrop(MouseKeys mKeys, point p, Canvas* target) {
	dragging = false;
}

void TrackBarHandler::newValue(int nv) {
	if (nv < bar->minimum)
		nv = bar->minimum;
	else if (nv > bar->maximum)
		nv = bar->maximum;
	bar->set_value(nv);
	change.fire(nv);
}

}  // namespace display