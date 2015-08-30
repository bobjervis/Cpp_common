#pragma once
#include "../common/derivative.h"
#include "../common/dictionary.h"
#include "../common/event.h"
#include "../common/file_system.h"
#include "../common/process.h"
#include "../common/script.h"
#include "../common/string.h"
#include "grid.h"
#include "tabbed_group.h"
#include "window.h"

namespace process {

class ThreadPool;

}  // namespace process

namespace display {

class Anchor;
class Background;
class Cursor;
class DrawContext;
class NavigationBar;
class ScrollableCanvas;
class ScrollableCanvasHandler;
class HiddenAnchor;
class TextBuffer;
class TextBufferDerivative;
class TextBufferManager;
class TextBufferSource;
class TextCanvas;
class TextEditor;
class TextLine;
class TextMessageLog;

class TextEditWindow : public Window {
	typedef Window super;
public:
	TextEditWindow();

	void hScroll(int x);

	void vScroll(int y);

	void set_buffer(TextBuffer* buffer);

	void set_cursor(script::fileOffset_t location);

private:
	ScrollableCanvas*		_canvas;
	TextCanvas*				_viewport;
	TextEditor*				_editor;
};

enum AnchorPriority {
	AP_NONE,
	AP_INFORMATIVE,
	AP_WARNING,
	AP_ERROR
};

class Anchor {
	friend TextBuffer;
	friend TextLine;
	friend TextCanvas;

public:
	Anchor();

	virtual ~Anchor();

	void insertChars(int at, int count);

	Anchor* deleteChars(int at, int count);

	void release();

	virtual bool persistent() = 0;

	virtual Canvas* balloon() = 0;

	virtual const string& caption() const = 0;
	/*
	 *	draw
	 *
	 *	This method is called after the text of the line has been
	 *	painted.  The topLeft point locates the upper left corner of the
	 *	line box containing the line.  The tabStop is helpful in
	 *	calculating the horizontal position of the annotation glyph.
	 */
	virtual void draw(const DrawContext& ctx) = 0;
	/*
	 *	draw
	 *
	 *	This method is called after the background of the gutter has
	 *	been painted.  The topLeft point locates the upper left corner
	 *	of the gutter area for the line.
	 */
	virtual void drawGutter(const DrawContext& ctx) = 0;

	virtual AnchorPriority priority() const = 0;

	virtual void moved(TextBuffer* buffer);

	TextLine* line() const { return _line; }

	int pixelOffset(const DrawContext& ctx) const;

	int lineno() const;

	int location() const;

	int locationInLine() const { return _location; }

	int previousWord() const;

	int nextWord() const;

	int startOfWord() const;

	int endOfWord() const;

	bool atStartOfBuffer() const;

	void put(Anchor* anchor);

	const Anchor* next() const { return _nextLine; }
protected:
	void moveTo(script::fileOffset_t location);

	Anchor* insert(Anchor* list, TextLine* line, script::fileOffset_t location);

	void insertAfter(Anchor* prev);

	void pop();

	void popLine();

	int						_location;
	Anchor*					_prevBuffer;
	Anchor*					_nextBuffer;
	Anchor*					_prevLine;
	Anchor*					_nextLine;
	TextLine*				_line;
};

class Cursor : public Anchor {
public:
	Cursor();

	void reset();

	virtual bool persistent();

	virtual Canvas* balloon();

	virtual const string& caption() const;

	virtual void draw(const DrawContext& ctx);

	virtual void drawGutter(const DrawContext& ctx);

	virtual AnchorPriority priority() const;

	bool	blinkOn;
	bool	show;

private:
};

class HiddenAnchor : public Anchor {
public:
	HiddenAnchor();

	virtual bool persistent();

	virtual Canvas* balloon();

	virtual const string& caption() const;

	virtual void draw(const DrawContext& ctx);

	virtual void drawGutter(const DrawContext& ctx);

	virtual AnchorPriority priority() const;
};

class Annotation : public Anchor {
public:
	Annotation(AnchorPriority priority, const string& label);

	virtual bool persistent();

	virtual Canvas* balloon();

	virtual const string& caption() const;

	virtual void draw(const DrawContext& ctx);

	virtual void drawGutter(const DrawContext& ctx);

	virtual AnchorPriority priority() const;

	virtual void moved(TextBuffer* buffer);

private:
	AnchorPriority		_priority;
	string				_label;
};

class TextEditor {
public:
	TextEditor(TextCanvas* viewport);

	void setBuffer(TextBuffer* buffer);

	Canvas* canvas() const { return (Canvas*)_canvas; }

	TextCanvas* viewport() const { return _viewport; }

private:
	void onHover(point p, Canvas* target);

	void onButtonDown(MouseKeys mKeys, point p, Canvas* target);

	void onClick(MouseKeys mKeys, point p, Canvas* target);

	void onDoubleClick(MouseKeys mKeys, point p, Canvas* target);

	void onStartDrag(MouseKeys mKeys, point p, Canvas* target);

	void onDrag(MouseKeys mKeys, point p, Canvas* target);

	void onDrop(MouseKeys mKeys, point p, Canvas* target);

	void onCharacterBoxChanged(dimension characterBox);

	void onViewportMoved(int newX, int newY);

	void onCharacter(char c);

	void onFunctionKey(FunctionKey fk, ShiftState ss);

	void onNavBarClick(MouseKeys mKeys, point p, Canvas* target);

	void selectAll();

	void clearSelection();

	void extendSelection();

	void deleteSelection();

	void cutSelection();

	void copySelection();

	void pasteCutBuffer();

	void openSearchPanel();

	void closeSearchPanel(Bevel* target);

	void goSearch(Bevel* target);

	void openSearchReplacePanel();

	void closeSearchReplacePanel(Bevel* target);

	void nextSearchReplace(Bevel* target);

	void doReplace(Bevel* target);

	void doReplaceAll(Bevel* target);

	bool advanceToNext();

	void replaceInstance();

	void srTakeFocus();

	void srReturn();

	void srKeyChanged();

	void srGetKeyFoucs();

	void srLoseKeyFocus();

	void startSearch();

	void nextMatch();

	void previousMatch();

	void apply(Undo* u);

	void cursorMoveLines(int amount);

	ScrollableCanvas*			_scroller;
	TextCanvas*					_viewport;
	NavigationBar*				_navigationBar;
	Canvas*						_canvas;
	ScrollableCanvasHandler*	_sHandler;
	Spacer*						_popInPanel;
	Canvas*						_searchPanel;
	Label*						_searchLabel;
	Field*						_searchField;
	Canvas*						_searchReplacePanel;
	Label*						_srSearchLabel;
	Label*						_replaceLabel;
	Field*						_srSearchField;
	Field*						_replaceField;
	Label*						_nextButtonFace;
	Label*						_replaceButtonFace;
	Label*						_replaceAllButtonFace;
	HiddenAnchor				_searchStart;
	bool						_replacePossible;
	bool						_popInLostFocus;
};
/*
 *	TextCanvas
 *
 *	This canvas does the basic drawing for a text buffer.
 */
class TextCanvas : public Canvas {
public:
	TextCanvas();

	~TextCanvas();

	virtual void getKeyboardFocus();

	virtual void loseKeyboardFocus();

	virtual dimension measure();

	virtual void paint(display::Device* b);

	void touch(TextLine* line);

	void putCursor(script::fileOffset_t location);

	Anchor* hoverAt(point p);

	script::fileOffset_t nearestPosition(point p);

	script::fileOffset_t nearestPosition(TextLine* line, int x);

	void hScrollNotice(int x);

	void vScrollNotice(int y);

	int pixelHeight();

	int topFullLine();

	int bottomFullLine();

	int lineHeight() { return _emSize.height; }

	int characterWidth() { return _emSize.width; }

	TextBuffer* buffer() const { return _buffer; }

	void set_buffer(TextBuffer* b);

	int					tabSize;

	Event1<dimension>	characterBoxChanged;
	Event2<int, int>	viewportMoved;

private:
	void computeWidestLine();

	int hScroll(int x);

	int vScroll(int y);

	void onChanged(TextLine* line);

	void onDeleted(int line, int count);

	void onInserted(int line, int count);

	void onLoaded();

	void onTick();

	void repaintCursor();

	BoundFont* bindFont(Font* font);

		// File context

	TextBuffer*		_buffer;

		// Event handlers

	void*			_changedHandler;
	void*			_deletedHandler;
	void*			_insertedHandler;
	void*			_loadedHandler;
	void*			_undoHandler;

		// Reference data and display status

	dimension		_emSize;
	Timer*			_blink;		// null = no focus, non-null = focus
	BoundFont*		_font;
};

class NavigationBar : public Canvas {
public:
	NavigationBar(ScrollableCanvas* scroller, TextCanvas* canvas);

	virtual dimension measure();

	virtual void paint(display::Device* device);

	int effectiveLineno(point p);

	void set_buffer(TextBuffer* buffer);

private:
	void onAnchorChanged(Anchor* anchor);

	ScrollableCanvas*	_scroller;
	TextCanvas*			_canvas;
	TextBuffer*			_buffer;
	void*				_changeHandler;
};

/*
 *	TextBuffer
 *
 *	This is the backing store for the text editor canvas.  It
 *	can be loaded once and must be deleted if you wish to load
 *	a different file.
 */
class TextBuffer {
	friend Anchor;
	friend HiddenAnchor;
	friend TextEditor;
	friend TextLine;
public:
	TextBuffer(const string& filename);

	~TextBuffer();

	void unload();

	bool load();

	void loadFromMemory(const string& s) {
		loadFromMemory(s.c_str(), s.size());
	}

	void loadFromMemory(const string& s, int offset, int length) {
		loadFromMemory(s.c_str() + offset, length);
	}

	void loadFromMemory(const char* s, int length);
	/*
	 *	clear
	 *
	 *	This method is called to set the text buffer up to
	 *	edit a new 'empty' file.  If the load method has
	 *	failed, but you wish to edit the file anyway, you
	 *	must call this method to prep the buffer.
	 */
	void clear();

	bool save() { return saveAs(_filename); }

	bool hasErrors();
	/*
	 *	saveAs
	 *
	 *	This method writes the contents of the buffer to the
	 *	named file.  The filename of the buffer is not
	 *	modified by this method.
	 */
	bool saveAs(const string& filename);

	void clearAnnotations();

	void put(Anchor* anchor, script::fileOffset_t location);

	/*
	 *	moveTo
	 *
	 *	This moves an anchor that is already in this buffer
	 *	to a new location.  If the anchor is not in any buffer,
	 *	this will do the same thing as a put operation (above).
	 *	It is the equivalent of:
	 *
	 *		if (this->has(anchor))
	 *			this->yank(anchor);
	 *		this->put(anchor, location);
	 */
	void moveTo(Anchor* anchor, script::fileOffset_t location);

	void yank(Anchor* anchor);

	bool has(Anchor* anchor);

	void deleteLines(int at, int count);

	void insertLines(int at, int count);

	void deleteChars(int at, int count);

	void insertChar(int at, char c) { insertChars(at, &c, 1); }

	void insertChars(int at, const char* text, int count);

	void read(int at, int count, string* output) const;

	bool hasSelection() const { return selection.line() != null; }

	int selectionStart();

	int selectionEnd();

	void clearSelection();

	void startSelection(int location);

	void extendSelection();

	void cacheSelection();

	void markSelectionState(DrawContext& ctx, int lineno);

	TextLine* line(int i);

	void computeWidestLine(const DrawContext& ctx);

	bool needsSave();

	bool fileExists();

	int search(const string& key, Anchor* start, Anchor* end);

	void invalidateLineNumber(int i);

	void invalidateLocation(int i);

	bool ready() const { return _ready; }

	int pixelWidth() { return _widestWidth; }

	int lineCount() const { return _lineCount; }

	int size() const;

	const string& filename() const { return _filename; }

	void set_filename(const string& newFilename) { _filename = newFilename; }

	fileSystem::TimeStamp age() const { return _age; }

	Event				saved;
	/*
	 *	modified
	 *
	 *	This event is fired when the contents of the buffer have been modified.
	 *	This happens when text is changed, inserted or deleted.
	 */
	Event1<TextBuffer*> modified;
	Event1<TextLine*>	viewChanged;
	Event2<int, int>	deleted;
	Event2<int, int>	inserted;
	Event1<Anchor*>		anchorChanged;
	Event				loaded;

	UndoStack			history;
	Cursor				cursor;
	HiddenAnchor		selection;
	int					topRow;
	int					leftColumn;

	// The desiredColumn is set on a mouse click event or a
	// lateral motion key (e.g. left or right arrow, backspace, a normal
	// keystroke) to the pixelOffset of the cursor location.  On
	// an up or down arrow key, the desiredColumn is left unchanged
	// as the actual cursor location is moved to 'best-fit' the
	// desired column.

	int			desiredColumn;

private:
	void finishLoad();

	void assignToBuffer(Anchor* anchor);

	void renumber(const TextLine* line);

	TextLine* lineAtLocation(int location) const;

	mutable process::Mutex		_lock;

	// Updated by cacheSelection
	Anchor*						_startOfSelection;
	Anchor*						_endOfSelection;
	int							_startOfSelectionLineno;
	int							_endOfSelectionLineno;

	string						_filename;
	TextLine**					_lines;
	int							_lineCount;
	int							_firstBadLineNumber;
	int							_firstBadLocation;
	string						_original;			// Original content of the file.
	Anchor*						_anchors;
	TextLine*					_widestLine;
	int							_widestWidth;
	fileSystem::TimeStamp		_age;
	bool						_ready;
};

class TextLine {
	friend TextBuffer;
	friend Anchor;
public:
	TextLine(TextBuffer* buffer, const char* text, int length);

	~TextLine();

	void reset(const char* text, int length);

	void put(Anchor* anchor, script::fileOffset_t location);

	void yank(Anchor* anchor);

	void deleteChars(int at, int count);

	void insertChar(int at, char c) { insertChars(at, &c, 1); }

	void insertChars(int at, const char* text, int count);

	Anchor* hoverAt(const DrawContext& ctx, int x);

	int width(const DrawContext& ctx);

	void paint(const DrawContext& ctx);

	int pixelOffset(const DrawContext& ctx, int characterPosition);

	int nearestPosition(const DrawContext& ctx, int x);

	int search(const string& key, int startOffset, Anchor* end);

	int lineno() const;

	int location() const;

	int previousWord(int offset) const;

	int nextWord(int offset) const;

	int startOfWord(int offset) const;

	int endOfWord(int offset) const;

	AnchorPriority maxPriority() const;
	/*
	 *	touch
	 *
	 *	Called to signal that the contents of the line
	 *	have changed.
	 */
	void touch();

	int length() const { return _length; }
	const char* text() const { return _text; }
	TextBuffer* buffer() const { return _buffer; }

	const Anchor* anchors() const { return _anchors; }

private:
	static const int BIT_INCREMENT = 4;

	void paintRun(const DrawContext& ctx, int startLocation, int endLocation, int* cumulativeColumn, Background* background);

	const char*		_original;
	int				_originalLength;
	char*			_text;
	int				_length;
	int				_width;
	bool			_changed;
	Anchor*			_anchors;
	TextBuffer*		_buffer;
	int				_lineNumber;
	int				_location;
};

class DrawContext {
public:
	TextCanvas*	canvas;
	Device*		device;
	point		topLeft;
	int			tabStop;
	int			lineHeight;
	int			startingSelection;
	int			endingSelection;
	int			normalTextColor;
	int			selectionTextColor;
	Background*	normalTextBackground;
	Background*	selectionTextBackground;
};

class TextBufferPool {
public:
	TextBufferPool();

	bool loaded(const string& filename);

	TextBufferManager* manage(const string& filename);

	bool saveAll();

	bool hasUnsavedEdits();

	class iterator {
		friend TextBufferPool;
	public:
		bool valid() {
			return _iterator.valid();
		}

		void next() {
			_iterator.next();
		}

		TextBufferManager* operator* () {
			return *_iterator;
		}

		const string& key() {
			return _iterator.key();
		}

	private:
		iterator(const TextBufferPool* pool) : _iterator(pool->_buffers.begin()){
		}

		dictionary<TextBufferManager*>::iterator _iterator;
	};

	friend iterator;

	iterator begin() const {
		return iterator(this);
	}

private:
	dictionary<TextBufferManager*> _buffers;
};

class TextBufferManager : public TabManager {
public:
	TextBufferManager(const string& filename);

	bool load();

	virtual const char* tabLabel();

	virtual Canvas* tabBody();

	virtual Canvas* onHover();

	virtual void bind(TextEditor* editor);

	virtual bool needsSave();

	virtual bool save();

	virtual void reload();

	virtual bool hasErrors();

	virtual void clearAnnotations();

	int compare(TextBufferManager* manager) const;

	TextBufferSource* source(derivative::Web* web);

	const string& filename() const { return _buffer.filename(); }

	TextBuffer* buffer() { return &_buffer; }

	TextEditor* editor() const { return _editor; }
private:
	enum FileState {
		FILE_NOT_LOADED,
		FILE_DOES_NOT_EXIST,
		FILE_COULD_NOT_BE_READ,
		FILE_READONLY,
		FILE_OK
	};

	void onSelected();

	void onUnselected();

	void onModified(TextBuffer*);

	void onBufferSaved();

	bool errorState() const { return _state < FILE_READONLY; }

	TextBufferSource*						_source;
	TextEditor*								_editor;
	TextBuffer								_buffer;
	FileState								_state;
};

class TextSnapshot {
public:
	string			image;
};

class TextBufferSource : public derivative::Object<TextSnapshot> {
public:
	typedef derivative::Value<TextSnapshot> Value;

	TextBufferSource(derivative::Web* web, TextBufferManager* tbm);

	virtual bool build();

	virtual fileSystem::TimeStamp age();

	script::MessageLog* messageLog();

	void commitMessages();

	virtual string toString();

	const string& filename() const { return _textBufferManager->buffer()->filename(); }

	TextBufferManager* manager() const { return _textBufferManager; }

private:
	void onModified(TextBuffer* buffer);

	void onSaved();

	fileSystem::TimeStamp		_age;
	bool						_errorLoading;
	TextMessageLog*				_messageLog;
	TextBufferManager*			_textBufferManager;
};

class TextMessageLog : public script::MessageLog {
public:
	TextMessageLog(TextBufferSource* source);

	void commit();

	virtual void log(script::fileOffset_t loc, const string& msg);

	virtual void error(script::fileOffset_t loc, const string& msg);

private:
	void commitImpl();

	TextBufferSource*				_source;
	vector<Anchor*>					_messages;
	vector<script::fileOffset_t>	_locations;
};

}  // namespace display
