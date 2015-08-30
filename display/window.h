#pragma once
#include <windows.h>
#include "../common/event.h"
#include "../common/string.h"
#include "../common/vector.h"
#include "canvas.h"
#include "root_canvas.h"

namespace display {

const int HOVER_PAUSE = 750;

class Cell;
class Choice;
class Clipboard;
class Field;
class GlobalFunctionKey;
class Label;
class Undo;

enum ShowWindowStyle {
	SWS_HIDE,
	SWS_NORMAL,
	SWS_MINIMIZED,
	SWS_MAXIMIZED
};

class PrintJob : public RootCanvas {
public:
	PrintJob(HDC hDC, HGLOBAL hDevMode);

	virtual bool isBindable();

	virtual Device* device();

	int startPrinting();

	void paint();

	int startPage();

	int endPage();

	int close();

	string title;
	string output;
	string datatype;
/*

	setSize: (d: dimension)
	{
		windows.SetWindowPos(handle, null, 0, 0, d.width, d.height, windows.SWP_NOMOVE)
	}

	paint: (b: Device)
	{
		child.paint(b)
	}

	resize: (d: dimension)
	{
		bounds.size = d
		measure()
		child.resize(d)
	}
*/
	bool color() const { return _color; }

private:
	Device*		_device;
	HDC			_printDC;
	bool		_color;
};
/*
abortPrint: (hDC: windows.HDC, iError: int) windows.BOOL
{
    msg: instance windows.MSG

    while(windows.PeekMessageW(&msg, null, 0, 0, windows.PM_REMOVE) != windows.FALSE)
    {
        windows.TranslateMessage(&msg)
        windows.DispatchMessageW(&msg)
    }
    return windows.TRUE
}
 */
class Window : public RootCanvas {
	typedef RootCanvas super;
public:
	Window();

	string title();

	void set_title(const string& title);

	void show(ShowWindowStyle shParm);

	virtual void show();

	virtual void hide();

	PrintJob* openPrintDialog();
};

class Dialog : public RootCanvas {
	typedef RootCanvas super;
public:
	Dialog();

	Dialog(Window* w);

	virtual bool isDialog();

	Choice* choice(const string& tag);

	Choice* choiceCancel(const string& tag);

	Choice* choiceOk(const string& tag);

	Choice* choiceApply(const string& tag);

	void onClose();

	void launchCancel(point anchor, Canvas* target);

	void launchOk(point anchor, Canvas* target);

	void launchApply(point anchor, Canvas* target);

	virtual void append(Canvas* c);

	void show(ShowWindowStyle shParm);

	virtual void show();

	virtual void hide();

	Event			cancel;
	Event			apply;

	string title() const;

	void set_title(const string& t);

private:
	void init();

	Window*				parent;
	vector<Choice*>		_items;
};

void beforeShowTree(Canvas* c);

void beforeHideTree(Canvas* c);

/*
createWindow: public (displayFile: string) Window
{
	fp := fileSystem.openTextFile(displayFile)
	if (fp == null)
		return null
	s := fp.readAllText()
	fp.close()
	x := heap new xml.XMLDOMParser()
	d := x.parse(s, false)
	heap.free(x)
	return createWindow(d)
}

createWindow: public (doc: xml.Document) Window
{
	if (doc.root.kind != xml.ELEMENT ||
		doc.root.tag != "Window")
		return null
	layout := doc.root.getChild("Layout")
	if (layout == null)
		return null
	c := layout.child
	if (c == null)
		return null
	class := c.getValue("parasol:class")
	if (class != null){
//		r := script.createInstance(class)
//		canvas := Canvas(r)
	}
	return null
}

out: Stream

dumpFonts: public (w: Window)
{
	hDC: windows.HDC
	lf: instance windows.LOGFONT

	out = process.out//fileSystem.createTextFile("fonts.txt")
	lf.lfCharSet = display::DEFAULT_CHARSET
	lf.lfFaceName[0] = 0
	lf.lfPitchAndFamily = 0
	hDC = windows.GetDC(w.handle)
	windows.EnumFontFamiliesExW(hDC, &lf, windows.FONTENUMPROC(&exproc), 0, 0)
	out.close()
}

exproc: (a: pointer windows.ENUMLOGFONTEX, 
				b: pointer windows.NEWTEXTMETRICEX, fontType: int, 
				appData: windows.LPARAM) int
{
	out.writeLine("Font:" + text.widezToString(a.elfLogFont.lfFaceName))
	return 1
}
 */
void CALLBACK timerCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

class GlobalFunctionKey {
public:
	GlobalFunctionKey*		next;
	FunctionKey				key;
	ShiftState				shiftState;
	Event2<FunctionKey,
		   ShiftState>*		action;
};

class Clipboard {
public:
	Clipboard(RootCanvas* rootCanvas);

	~Clipboard();

	bool read(string* text);

	bool write(const string& text);

private:
	HWND			_window;
};

void initialize(const string& binaryPath);

int loop();

bool allowPaintMessage();

bool messagesWaiting();

int messageBox(RootCanvas* w, const string& message, const string& title, unsigned uType);

}  // namespace display
