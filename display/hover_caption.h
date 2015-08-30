#pragma once
#include "root_canvas.h"

namespace display {

const int CAPTION_EDGE_WIDTH = 1;

class HoverCaption : public RootCanvas {
	typedef RootCanvas super;
public:
	HoverCaption(RootCanvas* parent);

	void show(point p);

	void dismiss();

	virtual dimension measure();

	virtual void layout(dimension size);

	virtual void paint(Device* b);

private:
	RootCanvas*			_parent;
};

}  // namespace display