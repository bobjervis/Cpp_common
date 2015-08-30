#include "../common/platform.h"
#include "window.h"

#include <windows.h>
#include "../common/file_system.h"
#include "../common/machine.h"
#include "../common/process.h"
#include "background.h"
#include "context_menu.h"
#include "device.h"
#include "grid.h"
#include "hover_caption.h"
#include "label.h"

namespace display {

const int WM_DELETE_ROOT_CANVAS = WM_USER;
const int WM_ROOT_CANVAS_RUN = WM_USER + 1;

static ATOM		frameWindowClass;
static string	binFolder;

static BOOL CALLBACK abortPrint(HDC hDC, int iError);
static void removeCharInstances(const char* data, char filter, string* output);
static void expandNewlines(const string& text, string* output);
static void paintTree(Canvas* c, Device* b);
static void settingChangeTree(Canvas* c);

PrintJob::PrintJob(HDC hDC, HGLOBAL hDevMode) {
	_handle = null;
	DEVMODE* dev = (DEVMODE*)GlobalLock(hDevMode);
	_color = dev->dmColor == DMCOLOR_COLOR;
	_printDC = hDC;
	_device = null;
	int horzRes = GetDeviceCaps(hDC, PHYSICALWIDTH);
	int vertRes = GetDeviceCaps(hDC, PHYSICALHEIGHT);
	int horzOffset = GetDeviceCaps(hDC, PHYSICALOFFSETX);
	int vertOffset = GetDeviceCaps(hDC, PHYSICALOFFSETY);
	int logPixelsX = GetDeviceCaps(hDC, LOGPIXELSX);
	int logPixelsY = GetDeviceCaps(hDC, LOGPIXELSY);
	rectangle r;
	r.topLeft.x = -horzOffset;
	r.topLeft.y = -vertOffset;
	r.opposite.x = horzRes - horzOffset;
	r.opposite.y = vertRes - vertOffset;
	bounds = r;
}

bool PrintJob::isBindable() {
	return true;
}

Device* PrintJob::device() {
	if (_device == null)
		_device = new Device(this);
	_device->reset(_printDC, bounds);
	return _device;
}

int PrintJob::startPrinting() {
	if (SetAbortProc(_printDC, &abortPrint) <= 0)
		return 0;

	DOCINFO di;

	memset(&di, 0, sizeof di);

	di.cbSize = sizeof di;

	if (title.size())
		di.lpszDocName = title.c_str();
	if (output.size())
		di.lpszOutput = output.c_str();
	if (datatype.size())
		di.lpszDatatype = datatype.c_str();
	di.fwType = 0;
	return StartDoc(_printDC, &di);
}

int PrintJob::startPage() {
	return StartPage(_printDC);
}

void PrintJob::paint() {
	paintEvent(_printDC, bounds);
}

int PrintJob::endPage() {
	return EndPage(_printDC);
}

int PrintJob::close() {
	int i = EndDoc(_printDC);
	DeleteDC(_printDC);
	return i;
}

static BOOL CALLBACK abortPrint(HDC hDC, int iError) {
    MSG msg;

    while(PeekMessage(&msg, null, 0, 0, PM_REMOVE) != FALSE) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return TRUE;
}

Window::Window() {
	_handle = CreateWindowEx(0, "EuropaFrame", "", 
							 WS_OVERLAPPEDWINDOW,
							 0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
							 null, null, (HINSTANCE)GetModuleHandle(null), null);
	_windowStyle = WS_OVERLAPPEDWINDOW;
	setRootCanvas(_handle, this);
	RECT r;
	GetClientRect(_handle, &r);
	bounds.topLeft.x = coord(r.left);
	bounds.topLeft.y = coord(r.top);
	bounds.opposite.x = coord(r.right);
	bounds.opposite.y = coord(r.bottom);
}

string Window::title() {
	string s;

	int i = GetWindowTextLength(_handle);
	char * buffer = s.buffer_(i);
	GetWindowText(_handle, buffer, i);
	return s;
}

void Window::set_title(const string& title) {
	SetWindowText(_handle, title.c_str());
}

void Window::show(ShowWindowStyle shParm) {
	int swc;
	switch (shParm){
	case	SWS_HIDE:
		swc = SW_HIDE;
		break;

	case	SWS_NORMAL:
		swc = SW_SHOWNORMAL;
		break;

	case	SWS_MAXIMIZED:
		swc = SW_MAXIMIZE;
		break;

	case	SWS_MINIMIZED:
		swc = SW_MINIMIZE;
		break;

	default:
		return;
	}
	super::show();
	if (ShowWindow(_handle, swc) != FALSE)
		BringWindowToTop(_handle);
}

void Window::show() {
	show(SWS_NORMAL);
}

void Window::hide() {
	super::hide();
	ShowWindow(_handle, SW_HIDE);
}

PrintJob* Window::openPrintDialog() {
	PRINTDLG p;

	memset(&p, 0, sizeof p);
	p.lStructSize = sizeof p;
	p.hwndOwner = _handle;
	p.Flags = PD_RETURNDC|PD_USEDEVMODECOPIES;
	if (PrintDlg(&p) != FALSE)
		return new PrintJob(p.hDC, p.hDevMode);
	else
		return null;
}

Dialog::Dialog() {
	parent = null;
	init();
}

Dialog::Dialog(Window* w) {
	parent = w;
	init();
}

bool Dialog::isDialog() {
	return true;
}

void Dialog::init() {
	HWND	parentH;

	if (parent != null)
		parentH = parent->handle();
	else
		parentH = null;
	_handle = CreateWindowEx(0, "EuropaFrame", "", 
							 WS_DLGFRAME|WS_SYSMENU,
							 0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
							 parentH, null, (HINSTANCE)GetModuleHandle(null), null);
	_windowStyle = WS_DLGFRAME|WS_SYSMENU;
	setRootCanvas(_handle, this);
	RECT r;
	GetClientRect(_handle, &r);
	bounds.topLeft.x = coord(r.left);
	bounds.topLeft.y = coord(r.top);
	bounds.opposite.x = coord(r.right);
	bounds.opposite.y = coord(r.bottom);
	close.addHandler(this, &Dialog::onClose);
	setBackground(&buttonFaceBackground);
}

void Dialog::set_title(const string& t) {
	SetWindowText(_handle, t.c_str());
}

Choice* Dialog::choice(const string &tag) {
	Choice* c = new Choice(tag);
	_items.push_back(c);
	return c;
}

Choice* Dialog::choiceCancel(const string &tag) {
	Choice* c = choice(tag);
	c->click.addHandler(this, &Dialog::launchCancel);
	return c;
}

Choice* Dialog::choiceOk(const string &tag) {
	Choice* c = choice(tag);
	c->click.addHandler(this, &Dialog::launchOk);
	return c;
}

Choice* Dialog::choiceApply(const string &tag) {
	Choice* c = choice(tag);
	c->click.addHandler(this, &Dialog::launchApply);
	return c;
}

void Dialog::onClose() {
	hide();
	cancel.fire();
}

void Dialog::launchCancel(display::point anchor, display::Canvas *target) {
	onClose();
}

void Dialog::launchOk(display::point anchor, display::Canvas *target) {
	hide();
	apply.fire();
}

void Dialog::launchApply(display::point anchor, display::Canvas *target) {
	apply.fire();
}

void Dialog::append(Canvas* c) {
	Grid* buttonPanel = new Grid();
		buttonPanel->cell(new Spacer(0), true);
		for (int i = 0; i < _items.size(); i++) {
			Choice* ch = _items[i];
			Bevel* b = new Bevel(2, new Spacer(3, 2, 3, 2, new Label(ch->tag)));
			buttonPanel->cell(new Spacer(2, b));
			ButtonHandler* bh = new ButtonHandler(b, 0);
			bh->click.addHandler(ch, &Choice::fireButtonClick);
		}
	buttonPanel->complete();
	Grid* g = new Grid();
		g->row(true);
		g->cell(c);
		g->row();
		g->cell(new Bevel(2));
		g->row();
		g->cell(buttonPanel);
	g->complete();
	super::append(g);
	dimension d = child->bounds.size();
	if (d.width != 0 || d.height != 0)
		setSize(d);
	else
		setSize(child->preferredSize());
	child->at(bounds.topLeft);
	child->resize(bounds.size());
}

void Dialog::show(display::ShowWindowStyle shParm) {
	if (parent != null)
		parent->setModalChild(this);
	int swc;
	switch (shParm){
	case	SWS_HIDE:
		swc = SW_HIDE;
		break;

	case	SWS_NORMAL:
		swc = SW_SHOWNORMAL;
		break;

	case	SWS_MAXIMIZED:
		swc = SW_MAXIMIZE;
		break;

	case	SWS_MINIMIZED:
		swc = SW_MINIMIZE;
		break;

	default:
		return;
	}
	super::show();
	if (ShowWindow(_handle, swc) != FALSE)
		BringWindowToTop(_handle);
}

void Dialog::show() {
	show(SWS_NORMAL);
}

void Dialog::hide() {
	super::hide();
	if (parent != null)
		parent->setModalChild(null);
	ShowWindow(_handle, SW_HIDE);
}

RootCanvas::RootCanvas() {
	init();
}

RootCanvas::~RootCanvas() {
	while (_fields) {
		Field* f = _fields;
		_fields = _fields->next;
		delete f;
	}
	if (_handle) {
		DestroyWindow(_handle);
		setRootCanvas(_handle, null);
	}
	if (_hoverTimer != null)
		_hoverTimer->kill(null);
	_fontBindings.deleteAll();
	delete _paintDevice;
	delete _device;
	while (_globalKeys) {
		GlobalFunctionKey* g = _globalKeys;
		_globalKeys = g->next;
		delete g;
	}
}

void RootCanvas::scheduleDelete() {
	PostMessage(_handle, WM_DELETE_ROOT_CANVAS, 0, 0);
}

bool RootCanvas::isDialog() {
	return false;
}

void RootCanvas::purgeCaches(Canvas* pruning) {
	if (_hoverTarget &&
		_hoverTarget->under(pruning))
		_hoverTarget = null;
	if (_dragOwner &&
		_dragOwner->under(pruning))
		_dragOwner = null;
	if (_keyFocus &&
		_keyFocus->under(pruning))
		_keyFocus = null;
}

void RootCanvas::init() {
	_handle = null;
	_curMouseCursor = null;
	blinkInterval = 300;
	mouseCursor = standardCursor(ARROW);
	_globalKeys = null;
	_keyFocus = null;
	_dragOwner = null;
	_isModal = false;
	_modalChild = null;
	_hasKeyFocus = false;
	_paintDevice = null;
	_device = null;
	_fields = null;
	_lButtonState = MBS_UP;
	_mButtonState = MBS_UP;
	_rButtonState = MBS_UP;
	undo.addHandler(this, &RootCanvas::undoCommand);
	redo.addHandler(this, &RootCanvas::redoCommand);
	_hoverTarget = null;
	_hoverTimer = null;
	_visibleCaption = null;
}

dimension RootCanvas::measure() {
	if (child)
		return child->preferredSize();
	else
		return dimension(0, 0);
}
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
void RootCanvas::mouseEvent(unsigned uMsg, MouseKeys fwKeys, short zDelta, point p) {
	if (!isActive)
		return;
	_mouseLocation = p;
	dismissVisibleCaption();
	if (_modalChild != null){
		_modalChild->mouseEvent(uMsg, fwKeys, zDelta, p);
		return;
	}
	arrange();
	beforeInputEvent.fire();
	Canvas* sc = this;
	Canvas* target = hitTest(p);
	if (target == null && _isModal)
		target = this;
	_hoverTarget = null;
	if (_hoverTimer != null) {
		_hoverTimer->kill(null);
		_hoverTimer = null;
	}
	MouseKeys k = fwKeys & (MK_SHIFT|MK_CONTROL);
	switch (uMsg){
	case	WM_LBUTTONDOWN:
		_downPoint = p;
		_lButtonState = MBS_DOWN;
		if (target != null)
			target->doButtonDown(MK_LBUTTON|k, p);
		_dragOwner = target;
		_shortDrag = true;
		break;

	case	WM_MBUTTONDOWN:
		_downPoint = p;
		_mButtonState = MBS_DOWN;
		if (target != null)
			target->doButtonDown(MK_MBUTTON|k, p);
		_dragOwner = target;
		_shortDrag = true;
		break;

	case	WM_RBUTTONDOWN:
		_downPoint = p;
		_rButtonState = MBS_DOWN;
		break;

	case	WM_LBUTTONUP:
		if (_lButtonState == MBS_DOWN){
			if (target != null)
				target->doClick(MK_LBUTTON|k, p);
		} else if (_lButtonState == MBS_DRAGGING && _dragOwner != null)
			_dragOwner->doDrop(MK_LBUTTON|k, p, target);
		_lButtonState = MBS_UP;
		break;

	case	WM_LBUTTONDBLCLK:
		_downPoint = p;
		if (target != null)
			_lButtonState = target->doDoubleClick(MK_LBUTTON|k, p);
		else
			_lButtonState = MBS_DOWN;
		break;

	case	WM_MBUTTONUP:
		if (_mButtonState == MBS_DOWN){
			if (target != null)
				target->doClick(MK_MBUTTON|k, p);
		} else if (_mButtonState == MBS_DRAGGING && _dragOwner != null) {
			if (_shortDrag &&
				abs(p.x - _downPoint.x) < SHORT_DRAG &&
				abs(p.y - _downPoint.y) < SHORT_DRAG)
				target->doClick(MK_MBUTTON|k, p);
			else
				_dragOwner->doDrop(MK_MBUTTON|k, p, target);
		}
		_mButtonState = MBS_UP;
		break;

	case	WM_MBUTTONDBLCLK:
		_downPoint = p;
		if (target != null)
			_mButtonState = target->doDoubleClick(MK_MBUTTON|k, p);
		else
			_mButtonState = MBS_DOWN_DOUBLE;
		break;

	case	WM_RBUTTONUP:
		if (_rButtonState == MBS_DOWN && target != null)
			target->doOpenContextMenu(p);
		_rButtonState = MBS_UP;
		break;

	case	WM_MOUSEMOVE:
		if (_lButtonState == MBS_DOWN ||
			_lButtonState == MBS_DOWN_DOUBLE){
			if (!_shortDrag ||
				abs(p.x - _downPoint.x) >= SHORT_DRAG ||
				abs(p.y - _downPoint.y) >= SHORT_DRAG) {
				_lButtonState = MBS_DRAGGING;
				if (_dragOwner != null)
					_dragOwner->doStartDrag(MK_LBUTTON|k, _downPoint);
			}
		} else if (_mButtonState == MBS_DOWN ||
				   _mButtonState == MBS_DOWN_DOUBLE){
			if (!_shortDrag ||
				abs(p.x - _downPoint.x) >= SHORT_DRAG ||
				abs(p.y - _downPoint.y) >= SHORT_DRAG) {
				_mButtonState = MBS_DRAGGING;
				if (_dragOwner != null)
					_dragOwner->doStartDrag(MK_MBUTTON|k, _downPoint);
			}
		}
		if (_lButtonState == MBS_DRAGGING){
			if (_dragOwner != null)
				_dragOwner->doDrag(MK_LBUTTON|k, p, target);
		} else if (_mButtonState == MBS_DRAGGING){
			if (_dragOwner != null)
				_dragOwner->doDrag(MK_MBUTTON|k, p, target);
		} else if (isActive){
			if (target != null) {
				target->doMouseMove(p);
				_hoverPoint = p;
				_hoverTarget = target;
				_hoverTimer = startTimer(HOVER_PAUSE);
				_hoverTimer->tick.addHandler(this, &RootCanvas::onHoverTick);
			}
		}
		break;

	case	WM_MOUSEWHEEL:
		if (target != null)
			target->doMouseWheel(fwKeys, zDelta, p);
		break;

	case	WM_MOUSEHOVER:
		if (target != null)
			target->doMouseHover(p);
		break;
	}

		// target will be used to locate the mouse cursor shape

	if ((_mButtonState == MBS_DRAGGING || _lButtonState == MBS_DRAGGING) && _dragOwner != null)
		target = _dragOwner;
	if (target != null){
		_curMouseCursor = target->activeMouseCursor();
		SetCursor((HCURSOR)_curMouseCursor);
	}
	afterInputEvent.fire();
	_undoStack.rememberCurrentUndo();
}

void RootCanvas::onHoverTick() {
	if (_hoverTarget) {
		if (_hoverTimer != null) {
			_hoverTimer->kill(null);
			_hoverTimer = null;
		}
		_hoverTarget->doMouseHover(_hoverPoint);
	}
}

void RootCanvas::enqueue(Handler* h) {
	PostMessage(_handle, WM_ROOT_CANVAS_RUN, 0, (LPARAM)h);
}

void RootCanvas::execute(void* handler) {
	Handler* h = (Handler*)handler;
	h->run();
	delete h;
}

void RootCanvas::keyDownEvent(int virtualKey, unsigned keyData) {
	unsigned char keyState[256];
	wchar_t wcharBuffer[5];

	if (_keyFocus == null)
		return;
	if (GetKeyboardState(keyState) == FALSE)
		return;
	dismissVisibleCaption();

		// These keys would be translated by ToUnicode, so
		// skip the translation of them,  We want them to be
		// processed as function keys always.  

	if (FunctionKey(virtualKey) != FK_TAB &&
		FunctionKey(virtualKey) != FK_BACK &&
		FunctionKey(virtualKey) != FK_RETURN &&
		FunctionKey(virtualKey) != FK_ESCAPE){
		int result = ToUnicode(virtualKey, (keyData >> 16) & 0xff, keyState,
							   wcharBuffer, dimOf(wcharBuffer), 0);
		if (result > 0){
			if ((keyState[FK_CONTROL] & 0x80) == 0){
				beforeInputEvent.fire();
				for (int i = 0; i < result; i++)
					_keyFocus->character.fire(char(wcharBuffer[i]));
				afterInputEvent.fire();
				_undoStack.rememberCurrentUndo();
				return;
			}
		}
	}
	if (FunctionKey(virtualKey) == FK_MENU ||
		FunctionKey(virtualKey) == FK_CONTROL ||
		FunctionKey(virtualKey) == FK_SHIFT)
		return;

	ShiftState ss = SS_NONE;

	if ((keyState[FK_MENU] & 0x80) != 0)
		ss |= SS_ALT;
	if ((keyState[FK_CONTROL] & 0x80) != 0)
		ss |= SS_CONTROL;
	if ((keyState[FK_SHIFT] & 0x80) != 0)
		ss |= SS_SHIFT;

		// Global keys take precedence and will not be
		// passed on to the keyboard focus Canvas.

	for (GlobalFunctionKey* gk = _globalKeys; gk != null; gk = gk->next)
		if (gk->key == FunctionKey(virtualKey) &&
			gk->shiftState == ss){
			beforeInputEvent.fire();
			gk->action->fire(gk->key, gk->shiftState);
			afterInputEvent.fire();
			_undoStack.rememberCurrentUndo();
			return;
		}
	beforeInputEvent.fire();
	_keyFocus->functionKey.fire(FunctionKey(virtualKey), ss);
	afterInputEvent.fire();
	_undoStack.rememberCurrentUndo();
}

void RootCanvas::keyUpEvent(int virtualKey, unsigned keyData) {
}

void RootCanvas::setFocusEvent(HWND loseHandle) {
	_hasKeyFocus = true;
	if (_keyFocus != null)
		_keyFocus->getKeyboardFocus();
}

void RootCanvas::killFocusEvent(HWND getHandle) {
	_hasKeyFocus = false;
	if (_keyFocus != null)
		_keyFocus->loseKeyboardFocus();
}

void RootCanvas::paintEvent(HDC hDC, const display::rectangle& paintRect) {
	if (_paintDevice == null)
		_paintDevice = new Device(this);
	_paintDevice->reset(hDC, paintRect);
	paintTree(this, _paintDevice);
}

void RootCanvas::activateWindow(short fActive, bool fMinimized) {
	dismissVisibleCaption();
	if (fActive == WA_INACTIVE)
		isActive = false;
	else if (_modalChild != null)
		SetActiveWindow(_modalChild->_handle);
	else {
		SetCursor((HCURSOR)_curMouseCursor);
		isActive = true;
	}
}

void RootCanvas::invalidate(const rectangle& r) {
	RECT rect;

	rect.top = r.topLeft.y;
	rect.left = r.topLeft.x;
	rect.right = r.opposite.x;
	rect.bottom = r.opposite.y;
	InvalidateRect(_handle, &rect, FALSE);
}

Timer* RootCanvas::startTimer(unsigned elapsed) {
	Timer* t = new Timer(this);
	if (t->start(elapsed))
		return t;
	else {
		delete t;
		return null;
	}
}

void RootCanvas::timerEvent(unsigned id) {
	Timer* t = (Timer*)id;
	t->fire();
}

void RootCanvas::undoCommand(display::FunctionKey key, display::ShiftState ss) {
	_undoStack.undo();
}

void RootCanvas::redoCommand(display::FunctionKey key, display::ShiftState ss) {
	_undoStack.redo();
}

point RootCanvas::toScreenCoords(point p) {
	WINDOWINFO w;

	w.cbSize = sizeof(w);
	GetWindowInfo(_handle, &w);

	p.x += w.rcClient.left;
	p.y += w.rcClient.top;
	return p;
}

point RootCanvas::toLocalCoords(point p) {
	WINDOWINFO w;

	w.cbSize = sizeof(w);
	GetWindowInfo(_handle, &w);

	p.x -= w.rcClient.left;
	p.y -= w.rcClient.top;
	return p;
}

rectangle RootCanvas::screenBounds() {
	WINDOWINFO w;

	w.cbSize = sizeof(w);
	GetWindowInfo(_handle, &w);

	rectangle r = bounds;
	r.topLeft.x += w.rcClient.left;
	r.topLeft.y += w.rcClient.top;
	r.opposite.x += w.rcClient.left;
	r.opposite.y += w.rcClient.top;
	return r;
}

BoundFont* RootCanvas::bind(const Font* f) {
	BoundFont** bf = _fontBindings.get(f);
	if (*bf)
		return *bf;
	BoundFont* n = new BoundFont(f, this);
	_fontBindings.insert(f, n);
	return n;
}

void RootCanvas::setSize(display::dimension d) {
	RECT r;
	SetWindowPos(_handle, null, 0, 0, d.width, d.height, SWP_NOMOVE);
	GetClientRect(_handle, &r);
	// Sometimes we don't get the proper sizing on the first try, so we have to adjust it.
	// This seems to depend on the window style.
	if (r.right != d.width || r.bottom != d.height)
		SetWindowPos(_handle, null, 0, 0, d.width + d.width - r.right, d.height + d.height - r.bottom, SWP_NOMOVE);
}

void RootCanvas::setKeyboardFocus(Canvas* c) {
	if (c == _keyFocus)
		return;
	if (_keyFocus != null && _hasKeyFocus) {
		Canvas* kf = _keyFocus;
		// This prevents any accidental recursive death in loseKeyboardFocus()
		_keyFocus = null;
		kf->loseKeyboardFocus();
	}
	_keyFocus = c;
	if (_keyFocus != null && _hasKeyFocus)
		_keyFocus->getKeyboardFocus();
}

void RootCanvas::onFunctionKey(FunctionKey key, ShiftState shiftState, 
							   Event2<FunctionKey, ShiftState>* action) {
	GlobalFunctionKey* prevGk = null;
	for (GlobalFunctionKey* gk = _globalKeys; gk != null; prevGk = gk, gk = gk->next){
		if (gk->key == key &&
			gk->shiftState == shiftState){
			if (action == null){
				if (prevGk != null)
					prevGk->next = gk->next;
				else
					_globalKeys = gk->next;
				delete gk;
			} else
				gk->action = action;
			return;
		}
	}
	if (action != null){
		GlobalFunctionKey* gk = new GlobalFunctionKey;
		gk->next = _globalKeys;
		_globalKeys = gk;
		gk->key = key;
		gk->shiftState = shiftState;
		gk->action = action;
	}
}

Field* RootCanvas::field(Label *lbl) {
	Field* f = new Field(this, lbl);
	if (_fields == null)
		_fields = f;
	else {
		Field* pf;
		for (pf = _fields; pf->next != null; pf = pf->next)
			;
		pf->next = f;
	}
	return f;
}

Field* RootCanvas::fieldAfter(Label *lbl, Label* prior) {
	Field* f = new Field(this, lbl);
	if (_fields == null)
		_fields = f;
	else {
		Field* pf;
		for (pf = _fields; pf->next != null; pf = pf->next)
			if (pf->label() == prior) {
				f->next = pf->next;
				break;
			}
		pf->next = f;
	}
	return f;
}

bool RootCanvas::removeFields(Canvas* c) {
	Field* pf = _fields;
	_fields = null;
	Field* previous = null;
	bool killFocus = false;
	bool deletedSome = false;
	while (pf != null) {
		Field* pfn = pf->next;
		if (pf->label()->under(c)) {
			if (_keyFocus == pf->label())
				killFocus = true;
			pf->removed();
			delete pf;
			deletedSome = true;
		} else {
			if (previous != null)
				previous->next = pf;
			else
				_fields = pf;
			previous = pf;
		}
		pf = pfn;
	}
	if (previous != null)
		previous->next = null;
	else
		_fields = null;
	if (killFocus)
		setKeyboardFocus(null);
	return deletedSome;
}

Field* RootCanvas::nthUnder(int index, Canvas* c) {
	for (Field* pf = _fields; pf != null; pf = pf->next)
		if (pf->label()->under(c)) {
			if (index == 0)
				return pf;
			index--;
		}
	return null;
}

int RootCanvas::indexUnder(Label* label, Canvas* c) {
	int index = 0;
	for (Field* pf = _fields; pf != null; pf = pf->next)
		if (pf->label()->under(c)) {
			if (pf->label() == label)
				return index;
			index++;
		}
	return -1;
}

HoverCaption* RootCanvas::hoverCaption() {
	return new HoverCaption(this);
}

void RootCanvas::setVisibleCaption(HoverCaption* caption) {
	dismissVisibleCaption();
	_visibleCaption = caption;
}

void RootCanvas::dismissVisibleCaption() {
	if (_visibleCaption != null) {
		_visibleCaption->dismiss();
		_visibleCaption = null;
	}
}

void RootCanvas::addUndo(Undo* u) {
	_undoStack.addUndo(u);
}

Undo* RootCanvas::nextUndo() {
	return _undoStack.nextUndo();
}

void RootCanvas::show() {
	beforeShowTree(this);
}

void RootCanvas::hide() {
	beforeHideTree(this);
}

bool RootCanvas::isBindable() {
	return _handle != null;
}

Device* RootCanvas::device() {
	if (_handle == null)
		return null;
	if (_device == null) {
		_device = new Device(this);
		_device->reset(GetDC(_handle), bounds);
	}
	return _device;
}

RootCanvas* RootCanvas::rootCanvas() {
	return this;
}

void RootCanvas::setModalChild(RootCanvas* window) {
	_modalChild = window;
	if (window != null)
		window->_isModal = true;
}

void beforeShowTree(Canvas* c) {
	c->beforeShow.fire();
	for (Canvas* sc = c->child; sc != null; sc = sc->sibling)
		beforeShowTree(sc);
}

void beforeHideTree(Canvas* c) {
	c->beforeHide.fire();
	for (Canvas* sc = c->child; sc != null; sc = sc->sibling)
		beforeHideTree(sc);
}

static LRESULT CALLBACK frameWindowProc(  HWND hwnd,      // handle to window
  UINT uMsg,      // message identifier
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
  )
{
	PAINTSTRUCT ps;

	switch (uMsg){
	case	WM_CLOSE:
		closeWindow(hwnd);
		return 0;

	case	WM_DELETE_ROOT_CANVAS:
		deleteRootCanvas(hwnd);
		return 0;

	case	WM_ROOT_CANVAS_RUN:
		runInRootCanvas(hwnd, lParam);
		return 0;

	case	WM_PAINT:
		BeginPaint(hwnd, &ps);
		{
			rectangle r;

			r.topLeft.x = (short)ps.rcPaint.left;
			r.topLeft.y = (short)ps.rcPaint.top;
			r.opposite.x = (short)ps.rcPaint.right;
			r.opposite.y = (short)ps.rcPaint.bottom;
			RootCanvas* rc = getRootCanvas(hwnd);
			rc->paintEvent(ps.hdc, r);
			SelectClipRgn(ps.hdc, null);
		}
		EndPaint(hwnd, &ps);
		return 0;

	case	WM_SIZE:
		if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED){
			dimension d;

			d.width = LOWORD(lParam);
			d.height = HIWORD(lParam);
			doSize(hwnd, d);
		}
		return 0;

	case	WM_ERASEBKGND:
		return 1;

	case	WM_LBUTTONDOWN:
	case	WM_MBUTTONDOWN:
	case	WM_RBUTTONDOWN:
		SetCapture(hwnd);
		{
			point p;
			short fwKeys, zDelta;

			fwKeys = LOWORD(wParam);
			zDelta = HIWORD(wParam);		// only good for mouse wheel events
			p.x = short(LOWORD(lParam));
			p.y = short(HIWORD(lParam));
			mouseEvent(hwnd, uMsg, fwKeys, zDelta, p);
		}
		return 0;

	case	WM_LBUTTONUP:
	case	WM_MBUTTONUP:
	case	WM_RBUTTONUP:
		if ((LOWORD(wParam) & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)) == 0)
			ReleaseCapture();

	case	WM_MOUSEHOVER:
	case	WM_MOUSEMOVE:
	case	WM_LBUTTONDBLCLK:
	case	WM_MBUTTONDBLCLK:
//	case	WM_RBUTTONDBLCLK:	 - ignored
		{
			point p;
			short fwKeys;

			fwKeys = LOWORD(wParam);
			p.x = short(LOWORD(lParam));
			p.y = short(HIWORD(lParam));
			mouseEvent(hwnd, uMsg, fwKeys, 0, p);
		}
		return 0;

	case	WM_MOUSEWHEEL:
		{
			point p;
			short fwKeys, zDelta;
			POINT pnt;

			fwKeys = LOWORD(wParam);
			zDelta = HIWORD(wParam);		// only good for mouse wheel events
			pnt.x = LOWORD(lParam);
			pnt.y = HIWORD(lParam);
			ScreenToClient(hwnd, &pnt);
			p.x = short(pnt.x);
			p.y = short(pnt.y);
			mouseEvent(hwnd, uMsg, fwKeys, zDelta, p);
		}
		return 0;

	case	WM_SYSKEYDOWN:
	case	WM_KEYDOWN:
		keyDownEvent(hwnd, (int)wParam, lParam);
		return 0;

	case	WM_SYSKEYUP:
	case	WM_KEYUP:
		keyUpEvent(hwnd, (int)wParam, lParam);
		return 0;

	case	WM_CHAR:
		return 0;

	case	WM_SETFOCUS:
		setFocusEvent(hwnd, (HWND)wParam);
		return 0;

	case	WM_KILLFOCUS:
		killFocusEvent(hwnd, (HWND)wParam);
		return 0;

	case	WM_ACTIVATE:
		{
			activateWindow(hwnd, LOWORD(wParam), HIWORD(wParam) != 0);
		}
		return 0;

	case	WM_MOUSEACTIVATE:
//		if ((int)LOWORD(lParam) == HTCLIENT)
//			return MA_ACTIVATEANDEAT;
//		else
			return MA_ACTIVATE;

	case	WM_SETTINGCHANGE:
		settingChangeTree(getRootCanvas(hwnd));
		return 0;

	case	WM_TIMER:
		timerEvent(hwnd, wParam);
		return 0;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void initialize(const string& binaryPath) {
	WNDCLASSEX	windowClassDef;

	windowClassDef.cbSize = sizeof (WNDCLASSEX);
	windowClassDef.style = CS_DBLCLKS | CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	windowClassDef.lpfnWndProc = frameWindowProc;
	windowClassDef.cbClsExtra = 0;
	windowClassDef.cbWndExtra = 0;
	windowClassDef.hInstance = (HINSTANCE)GetModuleHandle(null);
	windowClassDef.hIcon = NULL;
	windowClassDef.hCursor = NULL;//LoadCursor(0, IDC_ARROW);
	windowClassDef.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
	windowClassDef.lpszMenuName = NULL;
	windowClassDef.lpszClassName = "EuropaFrame";
	windowClassDef.hIconSm = NULL;
	frameWindowClass = RegisterClassEx(&windowClassDef);
	binFolder = fileSystem::absolutePath(fileSystem::directory(binaryPath));
}

int loop() {
	MSG msg;
	BOOL bRet;

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
	FreeConsole();
	for (;;) {
		if (PeekMessage(&msg, null, 0, 0, PM_NOREMOVE) == FALSE)
			process::currentThread()->idle.fire();
		bRet = GetMessage(&msg, null, 0, 0 );
		if (bRet == 0)
			break;
        if (bRet == -1) {
            // handle the error and possibly exit
        } else {
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        }
    } 
 
    // Return the exit code to the system. 
 
    return msg.wParam; 
}

bool allowPaintMessage() {
	MSG msg;

	if (PeekMessage(&msg, null, 0, 0, PM_REMOVE | PM_QS_PAINT) == FALSE)
		return false;
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	return true;
}

bool messagesWaiting() {
	MSG msg;

	return PeekMessage(&msg, null, 0, 0, PM_NOREMOVE) != FALSE;
}

int messageBox(RootCanvas* w, const string& message, const string& title, unsigned uType) {
	HWND hand;

	if (w)
		hand = w->handle();
	else
		hand = 0;
	return MessageBox(hand, message.c_str(), title.c_str(), uType);
}

Clipboard::Clipboard(RootCanvas *rootCanvas) {
	_window = rootCanvas->handle();
}

Clipboard::~Clipboard() {
}

bool Clipboard::read(string* text) {
	text->clear();
	if (!IsClipboardFormatAvailable(CF_TEXT))
		return false;
	if (!OpenClipboard(_window))
		return false;
	HANDLE hData = GetClipboardData(CF_TEXT);
	if (hData == NULL) {
		CloseClipboard();
		return false;
	}
	char* data = (char*)GlobalLock(hData);
	removeCharInstances(data, '\r', text);
	GlobalUnlock(hData);
	CloseClipboard();
	return true;
}

bool Clipboard::write(const string& text) {
	string expanded;
	expandNewlines(text, &expanded);
	if (!OpenClipboard(_window))
		return false;
	EmptyClipboard();
	HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, expanded.size() + 1);
	if (hData == null) {
		CloseClipboard();
		return false;
	}
	char* data = (char*)GlobalLock(hData);
	memcpy(data, expanded.c_str(), expanded.size());
	data[expanded.size()] = 0;
	GlobalUnlock(hData);
	SetClipboardData(CF_TEXT, hData);
	CloseClipboard();
	return true;
}

static void removeCharInstances(const char* data, char filter, string* output) {
	output->clear();
	for (int i = 0; data[i]; i++)
		if (data[i] != '\r')
			output->push_back(data[i]);
}

static void expandNewlines(const string& text, string* output) {
	output->clear();
	for (int i = 0; i < text.size(); i++) {
		if (text[i] == '\n')
			output->push_back('\r');
		output->push_back(text[i]);
	}
}

Bitmap* loadBitmap(Device* d, const string& filename) {
	string s = fileSystem::pathRelativeTo(filename, binFolder + "/../images/HERE");
	return new Bitmap(d, s);
}

static void paintTree(Canvas* c, Device* device) {
	c->arrange();
	bool beforeBuffer = device->buffer();
	bool buffering = c->bufferedDisplay(device);
	if (beforeBuffer)
		buffering = false;
	if (buffering)
		device->set_buffer(true);
	if (c->background())
		c->background()->paint(c, device);
	c->paint(device);
	rectangle saveC = device->clip();
	for (Canvas* sc = c->child; sc != null; sc = sc->sibling){
		device->set_clip(sc->bounds);
		paintTree(sc, device);
	}
	device->set_clip(saveC);
	if (buffering) {
		device->commit();
		device->set_buffer(false);
	}
}

static void settingChangeTree(Canvas* c) {
	for (Canvas* sc = c->child; sc != null; sc = sc->sibling)
		settingChangeTree(sc);
	c->settingChange.fire();
}

}  // namespace display
