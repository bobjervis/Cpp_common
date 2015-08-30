#pragma once
#include "../common/event.h"
#include "../common/map.h"
#include "../common/string.h"
#include "canvas.h"
#include "undo.h"

namespace display {

class BoundFont;
class Device;
class Field;
class GlobalFunctionKey;
class HoverCaption;
class Label;
class MouseCursor;

class RootCanvas : public Canvas {
	friend Canvas;
	friend Font;
	typedef Canvas super;
public:
	RootCanvas();

	~RootCanvas();

	void scheduleDelete();

	virtual bool isDialog();

	bool				isActive;

	virtual dimension measure();

	/*
	 *	mouseEvent
	 *
	 *	This code transforms a low-level Windows mouse event into a higher-level
	 *	Parasol2 mouse event.  Mouse events will only be received for events
	 *  occurring inside the window's client area, except for drag and drop events.
	 *
	 *	Mouse events are propagated according to the layout of the window.  Except
	 *	for drag and drop events, all mouse events are routed first to the smallest
	 *	canvas containing the mouse hot spot.  If that canvas does not have any
	 *	listener for the event, the event is then routed to that canvas' parent.
	 *	The event 'bubbles' out to each parent in turn until either the event is
	 *	handled by a listener, or the root canvas is reached, in which case the event
	 *	is discarded.  Drag and drop events are all routed to the listener that 
	 *	responded to the startDrag event.  The corresponding dragOver and dropOn
	 *	events are routed as normal events are.
	 *
	 *	The recognized Parasol2 mouse events are:
	 *
	 *		mouseMove			Means that no mouse buttons are prssed down, but
	 *							the mouse is moving over the window.  If the window
	 *							is not activated no mouseMove events are generated.
	 *
	 *
	 *		buttonDown			Means that a button has been depressed.
	 *
	 *		click				A mouse down event is followed by a mouse up event
	 *							with no intervening movement.
	 *
	 *		doubleClick			A click event is followed very quickly by a second
	 *							mouse down event.  If the mouse then doesn't move,
	 *							releasing the mouse button is discarded.  If after
	 *							a doubleClick event, the user starts moving the
	 *							mouse, a drag operation will start.
	 *
	 *		startDrag			After a mouse button is pushed down, if a subsequent
	 *							series of movements are received, they are treated
	 *							as a drag operation.  These are in turn recognized
	 *							as selection or drag-and-drop depending on the context.
	 *							Note that the mouse events are captured during a drag
	 *							operation, so that the drag and drop events may occur
	 *							outside the client area.
	 *
	 *		drag				This is an intermediate event during a drag.  Each
	 *							detected movement is reported as a drag event, rather
	 *							than a movement event.  Drag events may occur outside
	 *							the client area.
	 *
	 *		dragOver			Before a drag event is generated, if the mouse is
	 *							currently over another canvas, then a dragOver event
	 *							is generated and routed to the other canvas.
	 *
	 *		drop				This is generated when the mouse button is finally
	 *							released from a drag operation.  A drop operation can
	 *							occur outside the client area.
	 *
	 *		dropOn				Before a drop event is generated, if the mouse is
	 *							currently over another canvas, then a dropOn event is
	 *							generated and routed to the other canvas.
	 *
	 *		openContextMenu		The right mouse button is treated differently from the
	 *							middle and left buttons.  The right mouse button will
	 *							generate this event instead of a click event.  No double
	 *							click or drag and drop events will be generated for the
	 *							right mouse button.
	 *
	 *		mouseWheel			Moving the mouse wheel will generate mouseWheel events.
	 *							The direction of the movement as well as it's location
	 *							is specified.
	 */
	void mouseEvent(unsigned uMsg, MouseKeys fwKeys, short zDelta, point p);

	void timerEvent(unsigned id);

	void keyDownEvent(int virtualKey, unsigned keyData);

	void keyUpEvent(int virtualKey, unsigned keyData);

	void setFocusEvent(HWND loseHandle);

	void killFocusEvent(HWND getHandle);

	void paintEvent(HDC hDC, const rectangle& paintRect);

	void activateWindow(short fActive, bool fMinimized);

	void invalidate() { super::invalidate(); }

	virtual void invalidate(const rectangle& r);

	virtual Timer* startTimer(unsigned elapsed);

public:
	int				blinkInterval;			// in milliseconds

	point toScreenCoords(point p);

	point toLocalCoords(point p);

	rectangle screenBounds();

	void setSize(dimension d);

	void setKeyboardFocus(Canvas* c);

	void onFunctionKey(FunctionKey key, ShiftState shiftState, 
					   Event2<FunctionKey, ShiftState>* action);

	Field* field(Label* lbl);
	/*
	 *	fieldAfter
	 *
	 *	Creates a field and places it in the tab order after the 'prior'
	 *	parameter.  If 'prior' is not a defined label/field of this root,
	 *	(such as if it is a null) the field is created at the end of the
	 *	tab order.
	 */
	Field* fieldAfter(Label* lbl, Label* prior);
	/*
	 *	removeFields
	 *
	 *	All fields defined for any Label under the canvas c are
	 *	removed from the root tab order.
	 */
	bool removeFields(Canvas* c);
	/*
	 *	nthUnder
	 *
	 *	This returns the nth field (in tab order) that appears
	 *	attached to a label under the canvas c.  If there are fewer than
	 *	m fields under that canvas, a null is returned.
	 */
	Field* nthUnder(int index, Canvas* c);

	int indexUnder(Label* label, Canvas* c);

	HoverCaption* hoverCaption();

	void setVisibleCaption(HoverCaption* caption);

	void dismissVisibleCaption();

	void run(void (*func)()) {
		enqueue(new FunctionHandler(func));
	}

	template<class T>
	void run(T* object, void (T::*func)()) {
		enqueue(new ObjectHandler<T>(object, func));
	}

	template<class T, class M>
	void run(T* object, void (T::*func)(M), M m) {
		enqueue(new Object1Handler<T, M>(object, func, m));
	}

	Event							close;
	Event							beforeInputEvent;
	Event							afterInputEvent;
	Event2<display::FunctionKey,
		   display::ShiftState>		undo;
	Event2<display::FunctionKey,
		   display::ShiftState>		redo;

	void addUndo(Undo* u);

	Undo* nextUndo();

	virtual void show();

	virtual void hide();

	virtual bool isBindable();

	virtual Device* device();

	virtual RootCanvas* rootCanvas();

	void setModalChild(RootCanvas* window);

	void execute(void* handler);

	Field* fields() const { return _fields; }

	point mouseLocation() const { return _mouseLocation; }

	HWND handle() const { return _handle; }

	Canvas* keyFocus() const { return _keyFocus; }

protected:
	HWND				_handle;
	unsigned			_windowStyle;

	Device* paintDevice() { return _paintDevice; }

private:
	class Handler {
	public:
		virtual void run() = 0;
	};

	class FunctionHandler : public Handler {
	public:
		FunctionHandler(void (*func)()) {
			_func = func;
		}

		virtual void run() {
			_func();
		}

	private:
		void (*_func)();
	};

	template<class T>
	class ObjectHandler : public Handler {
	public:
		ObjectHandler(T* object, void (T::*func)()) {
			_object = object;
			_func = func;
		}

		virtual void run() {
			(_object->*_func)();
		}

	private:
		T* _object;
		void (T::*_func)();
	};

	template<class T, class M>
	class Object1Handler : public Handler {
	public:
		Object1Handler(T* object, void (T::*func)(M), M m) {
			_object = object;
			_func = func;
			_m = m;
		}

		virtual void run() {
			(_object->*_func)(_m);
		}

	private:
		T* _object;
		void (T::*_func)(M);
		M _m;
	};

	BoundFont* bind(const Font* f);

	void purgeCaches(Canvas* pruning);

	void init();

	void undoCommand(FunctionKey key, ShiftState ss);

	void redoCommand(FunctionKey key, ShiftState ss);

	void onHoverTick();

	void enqueue(Handler* h);

	Field*							_fields;
	Timer*							_hoverTimer;
	Canvas*							_hoverTarget;
	point							_hoverPoint;
	UndoStack						_undoStack;
	Device*							_device;
	RootCanvas*						_modalChild;
	MouseButtonState				_lButtonState;
	MouseButtonState				_mButtonState;
	MouseButtonState				_rButtonState;
	MouseCursor*					_curMouseCursor;
	point							_mouseLocation;
	point							_downPoint;
	Canvas*							_dragOwner;
	Canvas*							_keyFocus;
	bool							_shortDrag;
	bool							_hasKeyFocus;
	GlobalFunctionKey*				_globalKeys;
	bool							_isModal;
	Device*							_paintDevice;
	HoverCaption*					_visibleCaption;
	map<const Font, BoundFont*>		_fontBindings;
};

}  // namespace display
