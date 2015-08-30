#pragma once
#include "measurement.h"

namespace display {

class Canvas;
class Color;
class Device;
class Font;
class SolidBackground;

extern SolidBackground editableBackground;
extern SolidBackground scrollbarBackground;
extern SolidBackground whiteBackground;
extern SolidBackground blackBackground;
extern SolidBackground buttonFaceBackground;

class Background {
public:
	virtual ~Background();

	void paint(Canvas* area, Device* device);

	virtual void paint(Canvas* area, const rectangle& r, Device* device) = 0;

	virtual void pack(string* output) const = 0;
};

class SolidBackground : public Background {
	friend Font;
public:
	SolidBackground(Color* c);

	virtual void paint(Canvas* area, const rectangle& r, Device* device);

	virtual void pack(string* output) const;

	Color* color() const { return _color; }

private:
	void set_color(Color* c) { _color = c; }

	Color*			_color;
};


}  // namespace display
