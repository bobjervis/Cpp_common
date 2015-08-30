#include "../common/platform.h"
#include "hover_caption.h"

#include "device.h"

namespace display {

Color captionBackgroundColor(0xffffe0);

HoverCaption::HoverCaption(RootCanvas* parent) {
	_parent = parent;
}

void HoverCaption::show(point p) {
	super::show();
	_handle = CreateWindowEx(0, "EuropaFrame", "", 
							 WS_POPUP, p.x, p.y, 0, 0,
							 _parent->handle(), null, (HINSTANCE)GetModuleHandle(null), null);
	_windowStyle = WS_POPUP;
	setRootCanvas(_handle, this);
	dimension sz = preferredSize();
	RECT rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	rectangle r;

	WINDOWINFO w;

	w.cbSize = sizeof(w);
	GetWindowInfo(_parent->handle(), &w);

	r.topLeft.x = w.rcClient.left + p.x;
	r.topLeft.y = w.rcClient.top + p.y;
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
	bounds.topLeft.x = coord(rect.left);
	bounds.topLeft.y = coord(rect.top);
	bounds.opposite.x = coord(rect.right);
	bounds.opposite.y = coord(rect.bottom);
	child->resize(bounds.size());
	ShowWindow(_handle, SW_SHOWNOACTIVATE);
	SetActiveWindow(_parent->handle());
	_parent->setVisibleCaption(this);
}

void HoverCaption::dismiss() {
	hide();
	ShowWindow(_handle, SW_HIDE);
	scheduleDelete();
}

dimension HoverCaption::measure() {
	dimension sz = child->preferredSize();
	sz.width += 2 * CAPTION_EDGE_WIDTH;
	sz.height += 2 * CAPTION_EDGE_WIDTH;
	return sz;
}

void HoverCaption::layout(dimension size) {
	point p = bounds.topLeft;

	p.x++;
	p.y++;
	child->at(p);
	dimension sz = bounds.size();
	sz.width -= 2 * CAPTION_EDGE_WIDTH;
	sz.height -= 2 * CAPTION_EDGE_WIDTH;
	child->resize(sz);
}

void HoverCaption::paint(Device* dev) {
	dev->fillRect(bounds, &captionBackgroundColor);
	rectangle r;
	r.topLeft = bounds.topLeft;
	r.opposite.y = r.topLeft.y + CAPTION_EDGE_WIDTH;
	r.opposite.x = bounds.opposite.x;
	dev->fillRect(r, &black);
	r.opposite.x = r.topLeft.x + CAPTION_EDGE_WIDTH;
	r.opposite.y = bounds.opposite.y;
	dev->fillRect(r, &black);
	r.topLeft.y = bounds.opposite.y - CAPTION_EDGE_WIDTH;
	r.opposite.x = bounds.opposite.x;
	dev->fillRect(r, &black);
	r.topLeft.x = bounds.opposite.x - CAPTION_EDGE_WIDTH;
	r.topLeft.y = bounds.topLeft.y + CAPTION_EDGE_WIDTH;
	dev->fillRect(r, &black);
}

}  // namespace display