#pragma once
#include "measurement.h"
/*
	The Europa Display system is a layout and rendering engine
	that provides platform independence and rich user experience
	in a powerful programming environment.

	The core concept of the display system is the separation of
	output and input functionality.  For mouse input, the output
	objects are only used to route events to context-specific
	handlers.

	A Layout object is the unit of output.  It manages an abstract
	rectangle of space, located in some 3-dimensional coordinate
	system.  The rectangle is oriented parallel to the x-y plane
	(all points in the rectangle have the same z coordinate).

	Layout objects are logically grouped into trees.  Each Layout
	object may have a parent, which can change, but which cannot
	form a cycle.  In addition, all Layout objects that share a 
	common parent are ordered with respect to one another.

	Before anything can be drawn or mouse events processed, the
	Layout objects of a tree must be arranged.
	The user views the Layout rectangles from above (as if from a
	positive infinite z).  Layouts may have any alpha blend from 
	1 (fully opaque content) to 0 (fully transparent content),
	although having no background makes the alpha value always
	treated as 0.

	The widgets supplied in the package include the following:

		General Layout:

			Grid
			TabbedGroup
			ScrollableCanvas
			*Box
			*Flow

		On-Screen Controls:

			Toggle
			RadioButton
			Label
			Trackbar
				HorizontalTrackBar
			ScrollBar
				HorizontalScrollBar
				VerticalScrollBar
			Image

		Complex Controls:

			Outline
			TextCanvas
			NavigationBar (for TextCanvas)

		Windows:

			RootCanvas
				Window
				Dialog
				ContextMenu
				HoverCaption

 */
namespace display {

class Layout {
public:
	Layout();

	virtual ~Layout();

	virtual dimension measure();

	void setPosition(point p);

	void setSize(dimension s);

private:
};

}  // namespace display