#pragma once
#include "../common/string.h"

namespace display {

typedef int coord;					// A value in device pixels, in some (implicit) coordinate space

const coord COORD_MAX = 0x7fffffff;
const coord COORD_MIN = 0x80000000;

struct point {
	coord	x, y;

	point() {
		x = 0;
		y = 0;
	}

	point(coord x, coord y) {
		this->x = x;
		this->y = y;
	}
};
#if 0


innerToOuter: (inner: point, origin: point) point
{
	outer: point
	outer.x = coord(origin.x + inner.x)
	outer.y = coord(origin.y + inner.y)
	return outer
}

outerToInner: (outer: point, origin: point) point
{
	inner: point
	inner.x = coord(outer.x - origin.x)
	inner.y = coord(outer.y - origin.y)
	return inner
}
#endif
struct dimension {
	coord	width;
	coord	height;

	dimension() {
		width = 0;
		height = 0;
	}

	dimension(int width, int height) {
		this->width = width;
		this->height = height;
	}

	void setNoSize() {
		width = -1;
	}

	bool hasSize() {
		return width >= 0;
	}
};

extern const dimension NO_SIZE;

struct rectangle {
	point		topLeft;
	point		opposite;
/*
	new: (x0: int, y0: int, x1: int, y1: int)
	{
		topLeft.x = coord(x0)
		topLeft.y = coord(y0)
		opposite.x = coord(x1)
		opposite.y = coord(y1)
	}
 */
	void zero() {
		topLeft.x = 0;
		topLeft.y = 0;
		opposite.x = 0;
		opposite.y = 0;
	}

	dimension size() const {
		dimension d;

		d.width = coord(opposite.x - topLeft.x);
		d.height = coord(opposite.y - topLeft.y);
		return d;
	}

	void set_size(dimension d) {
		opposite.x = coord(topLeft.x + d.width);
		opposite.y = coord(topLeft.y + d.height);
	}
/*
	setOrigin: (newOrigin: point)
	{
		opposite.x = coord(newOrigin.x + width)
		opposite.y = coord(newOrigin.y + height)
		topLeft.x = newOrigin.x
		topLeft.y = newOrigin.y
	}

	translateTo: (offset: point)
	{
		topLeft.x += offset.x
		topLeft.y += offset.y
		opposite.x += offset.x
		opposite.y += offset.y
	}

	translateFrom: (offset: point)
	{
		topLeft.x -= offset.x
		topLeft.y -= offset.y
		opposite.x -= offset.x
		opposite.y -= offset.y
	}

	translateX: (offset: int)
	{
		topLeft.x += offset
		opposite.x += offset
	}

	translateY: (offset: int)
	{
		topLeft.y += offset
		opposite.y += offset
	}

	x:		coord { topLeft.x = assigned; } = { return topLeft.x; }
	y:		coord { topLeft.y = assigned; } = { return topLeft.y; }
	*/
	coord width() const { return coord(opposite.x - topLeft.x); }
	void set_width(coord w) { opposite.x = coord(topLeft.x + w); }
	coord height() const { return coord(opposite.y - topLeft.y); }
	void set_height(coord h) { opposite.y = coord(topLeft.y + h); }
	coord top() const { return topLeft.y; }
	coord left() const { return topLeft.x; }
	coord bottom() const { return opposite.y; }
	coord right() const { return opposite.x; }

	void set_left(coord x) {
		topLeft.x = x;
	}

	void set_right(coord x) {
		opposite.x = x;
	}

	bool contains(point p) {
		if (p.x < topLeft.x)
			return false;
		if (p.y < topLeft.y)
			return false;
		if (p.x >= opposite.x)
			return false;
		if (p.y >= opposite.y)
			return false;
		else
			return true;
	}

	bool contains(const rectangle& r) {
		if (r.topLeft.x < topLeft.x)
			return false;
		if (r.topLeft.y < topLeft.y)
			return false;
		if (r.opposite.x >= opposite.x)
			return false;
		if (r.opposite.y >= opposite.y)
			return false;
		else
			return true;
	}
	/*
		METHOD:	add

		This method combines this rectangle with the
		argument rectangle.  The result is the smallest
		rectangle that contains both input rectangles.
	 */
	void add(const rectangle& r) {
		if (topLeft.x > r.topLeft.x)
			topLeft.x = r.topLeft.x;
		if (topLeft.y > r.topLeft.y)
			topLeft.y = r.topLeft.y;
		if (opposite.x < r.opposite.x)
			opposite.x = r.opposite.x;
		if (opposite.y < r.opposite.y)
			opposite.y = r.opposite.y;
	}

	string toString() {
		string s;

		s.printf("[%d:%d,%d:%d]", topLeft.x, topLeft.y, opposite.x, opposite.y);
		return s;
	}
};
#if 0
overlaps: (r1: pointer rectangle, r2: pointer rectangle) boolean
{

		// reject if r1 to the left of r2

	if (r1.opposite.x <= r2.topLeft.x)
		return false

		// reject if r1 above r2

	if (r1.opposite.y <= r2.topLeft.y)
		return false

		// reject if r1 to the right of r2

	if (r2.opposite.x <= r1.topLeft.x)
		return false

		// reject if r1 below r2

	if (r2.opposite.y <= r1.topLeft.y)
		return false

		// not rejected yet? must overlap

	return true
}
#endif

}  // namespace display
