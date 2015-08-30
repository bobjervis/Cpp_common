#include "../common/platform.h"
#include "background.h"

#include "canvas.h"
#include "device.h"

namespace display {

SolidBackground editableBackground(&editableColor);
SolidBackground scrollbarBackground(&scrollbarColor);
SolidBackground whiteBackground(&white);
SolidBackground buttonFaceBackground(&buttonFaceColor);
SolidBackground blackBackground(&black);

Background::~Background() {
}

void Background::paint(Canvas *area, Device *device) {
	paint(area, area->bounds, device);
}

SolidBackground::SolidBackground(Color* c) {
	_color = c;
}

void SolidBackground::paint(Canvas *area, const rectangle& r, Device *device) {
	device->fillRect(r, _color);
}

void SolidBackground::pack(string* output) const {
}

}  // namespace display
