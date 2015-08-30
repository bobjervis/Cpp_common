#pragma once
#include "canvas.h"
#include "measurement.h"

namespace display {

class HorizontalScrollBar;
class Label;
class VerticalScrollBar;

const coord SCROLLBAR_WIDTH = 16;

class ScrollableCanvas : public Canvas {
	typedef Canvas super;
public:
	ScrollableCanvas();

	~ScrollableCanvas();

	virtual void childJumbled(Canvas* child);

	virtual bool bufferedDisplay(Device* device);

	virtual dimension measure();

	virtual void layout(dimension size);

	void expose(const rectangle& r);

	void align(const rectangle& r, float vertical, float horizontal);

	void hScroll(int scrollX);

	void vScroll(int scrollY);

	void hScrollNotice(int x);

	void vScrollNotice(int y);

	bool usesVerticalBar();

	bool usesHorizontalBar();

	VerticalScrollBar* vertical() const { return _vertical; }
	HorizontalScrollBar* horizontal() const { return _horizontal; }

private:
	VerticalScrollBar*		_vertical;
	HorizontalScrollBar*	_horizontal;
};

class ScrollBar : public Canvas {
	friend ScrollBarHandler;
public:
	ScrollBar();

	~ScrollBar();

	int value() const { return _value; }

	void set_value(int v);

	virtual void layout(dimension size);

	virtual void updateSlider();

	int				minimum;
	int				maximum;
	int				sliderSize;

	Event1<int>		changed;

protected:
	int				_value;
	Bevel*			negativeButton;
	Bevel*			positiveButton;
	Label*			negativeLabel;
	Label*			positiveLabel;
	Canvas*			slider;
	rectangle		smallerTray;
	rectangle		largerTray;
private:
	static Font*	_scrollFont;
};

class VerticalScrollBar : public ScrollBar {
	typedef ScrollBar super;
public:
	VerticalScrollBar();

	virtual dimension measure();

	virtual void layout(dimension size);

	virtual void updateSlider();
};

class HorizontalScrollBar : public ScrollBar {
	typedef ScrollBar super;
public:
	HorizontalScrollBar();

	virtual dimension measure();

	virtual void layout(dimension size);

	virtual void updateSlider();
};

class ScrollBarHandler {
public:

	ScrollBarHandler(VerticalScrollBar* vBar, HorizontalScrollBar* hBar, Canvas* client);
	
	~ScrollBarHandler();

	void newClient(Canvas* client);

	void recalcRepeatInterval();

	void vSmallUpMove(Bevel* b = null);

	void vSmallDownMove(Bevel* b = null);

	void vBigUpMove();

	void vBigDownMove();

	void vBigMove(MouseKeys mKeys, point p, Canvas* target);

	void vMouseWheel(MouseKeys mKeys, point p, int zDelta, Canvas* target);

	void hMouseWheel(MouseKeys mKeys, point p, int zDelta, Canvas* target);

	void vStartDrag(MouseKeys mKeys, point p, Canvas* target);

	void vDrag(MouseKeys mKeys, point p, Canvas* target);

	void vDrop(MouseKeys mKeys, point p, Canvas* target);

	void hStartDrag(MouseKeys mKeys, point p, Canvas* target);

	void hDrag(MouseKeys mKeys, point p, Canvas* target);

	void hDrop(MouseKeys mKeys, point p, Canvas* target);

	void vScroll(int ny);

	void hSmallLeftMove(Bevel* b);

	void hSmallRightMove(Bevel* b);

	void hBigMove(MouseKeys mKeys, point p, Canvas* target);

	void hScroll(int ny);

	int vSmall() const { return vSmallIncrement ? vSmallIncrement : client->bounds.height() / 25; }

	int hSmall() const { return hSmallIncrement ? hSmallIncrement : client->bounds.width() / 25; }

	int						hSmallIncrement;
	int						vSmallIncrement;
private:
	VerticalScrollBar*		vBar;
	HorizontalScrollBar*	hBar;
	Canvas*					client;
	ButtonHandler*			vnegHandler;	
	ButtonHandler*			vposHandler;
	ButtonHandler*			hnegHandler;	
	ButtonHandler*			hposHandler;
	bool					vDragging;
	bool					hDragging;
	coord					anchor;
	coord					interval;
	int						baseValue;
};

class ScrollableCanvasHandler : public ScrollBarHandler {
public:
	ScrollableCanvasHandler(ScrollableCanvas* canvas);

	ScrollableCanvas*			canvas() const { return _canvas; }
private:
	ScrollableCanvas*			_canvas;
};

int keyboardRepeatInterval();

const coord TRACKBAR_WIDTH = 16;
const coord SLIDER_MARGIN = 2;

class TrackBar : public Canvas {
	friend TrackBarHandler;
public:
	int					minimum;
	int					maximum;
	int					clickIncrement;
	Canvas*				slider;
	rectangle			smallerTray;
	rectangle			largerTray;

	TrackBar();

	virtual dimension doMeasure();

	int value() const { return _value; }

	void set_value(int v);

	virtual void updateSlider();

	virtual int anchor(point p);
protected:
	float				intervalPerPixel;
	int					_value;
};
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
class HorizontalTrackBar : public TrackBar {
	typedef TrackBar super;
public:
	virtual dimension measure();

	virtual void layout(dimension size);

	virtual void updateSlider();

	virtual int anchor(point p);
};

class TrackBarHandler {
public:
	TrackBarHandler(TrackBar* bar);

	void bigMove(MouseKeys mKeys, point p, Canvas* target);

	void mouseWheel(MouseKeys mKeys, point p, int zDelta, Canvas* target);

	void bDown(MouseKeys mKeys, point p, Canvas* target);

	void bStartDrag(MouseKeys mKeys, point p, Canvas* target);

	void bDrag(MouseKeys mKeys, point p, Canvas* target);

	void bDrop(MouseKeys mKeys, point p, Canvas* target);

	void newValue(int nv);

	Event1<int>		change;
private:
	TrackBar*		bar;
	bool			dragging;
	bool			hDragging;
	coord			anchor;
	coord			interval;
	int				baseValue;
	point			anchorPoint;
};

}  // namespace display
