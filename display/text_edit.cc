#include "../common/platform.h"
#include "text_edit.h"

#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include "../common/file_system.h"
#include "../common/machine.h"
#include "../common/process.h"
#include "../display/background.h"
#include "device.h"
#include "hover_caption.h"
#include "label.h"
#include "scrollbar.h"
#include "window.h"

namespace display {

static Color selectionBackgroundColor(0x0);
static SolidBackground selectionBackground(&selectionBackgroundColor);
static Pen* infoPen = createPen(PS_SOLID, 1, 0x0000ff);
static Pen* warningPen = createPen(PS_SOLID, 1, 0xe0e000);
static Pen* errorPen = createPen(PS_SOLID, 1, 0xff0000);

TextEditWindow::TextEditWindow() {
	_viewport = new TextCanvas();
	_editor = new TextEditor(_viewport);
	append(_editor->canvas());
	setKeyboardFocus(_viewport);
}

void TextEditWindow::hScroll(int x) {
	_canvas->hScroll(x);
}

void TextEditWindow::vScroll(int y) {
	_canvas->vScroll(y);
}

void TextEditWindow::set_buffer(TextBuffer *b) {
	_viewport->set_buffer(b);
	_canvas->jumbleContents();
}

void TextEditWindow::set_cursor(script::fileOffset_t location) {
	_viewport->putCursor(location);
}

class DeleteCharsCommand : public display::Undo {
public:
	DeleteCharsCommand(TextCanvas* canvas, int at, int count) {
		canvas->buffer()->read(at, count, &_content);
		_canvas = canvas;
		_at = at;
	}

	virtual void apply() {
		_canvas->buffer()->deleteChars(_at, _content.size());
		_canvas->putCursor(_at);
	}

	virtual void revert() {
		_canvas->buffer()->insertChars(_at, _content.c_str(),  _content.size());
		_canvas->putCursor(_at + _content.size());
	}

	virtual void discard() {
	}

private:
	TextCanvas* _canvas;
	int _at;
	string _content;
};

class InsertCharsCommand : public display::Undo {
public:
	InsertCharsCommand(TextCanvas* canvas, int at, const char* s, int count) {
		_canvas = canvas;
		_at = at;
		_content = string(s, count);
	}

	virtual void apply() {
		_canvas->buffer()->insertChars(_at, _content.c_str(), _content.size());
		_canvas->putCursor(_at + _content.size());
	}

	virtual void revert() {
		_canvas->buffer()->deleteChars(_at, _content.size());
		_canvas->putCursor(_at);
	}

	virtual void discard() {
	}

private:
	TextCanvas* _canvas;
	int _at;
	string _content;
};

TextEditor::TextEditor(TextCanvas* viewport) {
	_scroller = new ScrollableCanvas();
	_viewport = viewport;
	_scroller->append(_viewport);
	_sHandler = new ScrollableCanvasHandler(_scroller);
	_viewport->mouseHover.addHandler(this, &TextEditor::onHover);
	_viewport->character.addHandler(this, &TextEditor::onCharacter);
	_viewport->functionKey.addHandler(this, &TextEditor::onFunctionKey);
	_viewport->buttonDown.addHandler(this, &TextEditor::onButtonDown);
	_viewport->click.addHandler(this, &TextEditor::onClick);
	_viewport->doubleClick.addHandler(this, &TextEditor::onDoubleClick);
	_viewport->startDrag.addHandler(this, &TextEditor::onStartDrag);
	_viewport->drag.addHandler(this, &TextEditor::onDrag);
	_viewport->drop.addHandler(this, &TextEditor::onDrop);
	_viewport->characterBoxChanged.addHandler(this, &TextEditor::onCharacterBoxChanged);
	_viewport->viewportMoved.addHandler(this, &TextEditor::onViewportMoved);
	_scroller->vertical()->changed.addHandler(_viewport, &TextCanvas::vScrollNotice);
	_scroller->horizontal()->changed.addHandler(_viewport, &TextCanvas::hScrollNotice);
	_popInPanel = new Spacer(0);
	_navigationBar = new NavigationBar(_scroller, _viewport);
	_navigationBar->setBackground(&buttonFaceBackground);
	_navigationBar->click.addHandler(this, &TextEditor::onNavBarClick);
	_navigationBar->disableDoubleClick = true;
	Grid* g = new Grid();
		g->row(true);
		g->cell(_scroller, true);
		g->cell(_navigationBar);
		g->row();
		g->cell(display::dimension(2, 1), _popInPanel);
	g->complete();

	_scroller->setBackground(&whiteBackground);
	_canvas = g;
	_searchPanel = null;
	_searchReplacePanel = null;
	_popInLostFocus = true;
}

void TextEditor::setBuffer(TextBuffer* buffer) {
	_viewport->set_buffer(buffer);
	_navigationBar->set_buffer(buffer);
	_canvas->jumbleContents();
	if (buffer) {
		_scroller->vertical()->set_value(buffer->topRow);
		_scroller->horizontal()->set_value(buffer->leftColumn);
	}
}

void TextEditor::onHover(point p, Canvas* target) {
	Anchor* a = _viewport->hoverAt(p);
	if (a) {
		const string& caption = a->caption();
		if (caption.size()) {
			display::HoverCaption* hc = _viewport->rootCanvas()->hoverCaption();
			hc->append(new display::Label(caption));
			hc->show(p);
		}
	}
}

void TextEditor::onViewportMoved(int newX, int newY) {
	_sHandler->hScroll(newX);
	_sHandler->vScroll(newY);
}

void TextEditor::onCharacter(char c) {
	_popInLostFocus = true;
	int len, loc;
	TextBuffer* buffer = _viewport->buffer();
	if (buffer == null)
		return;
	if (buffer->hasSelection())
		deleteSelection();
	apply(new InsertCharsCommand(_viewport, buffer->cursor.location(), &c, 1));
}

void TextEditor::onButtonDown(MouseKeys mKeys, point p, Canvas* target) {
	_popInLostFocus = true;
	if (mKeys & MK_LBUTTON){
		TextBuffer* buffer = _viewport->buffer();
		clearSelection();
		buffer->cursor.show = false;
		target->rootCanvas()->setKeyboardFocus(target);
	}
}

void TextEditor::onClick(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		TextBuffer* buffer = _viewport->buffer();
		int desiredPosition = _viewport->nearestPosition(p);
		_viewport->putCursor(desiredPosition);
	}
}

void TextEditor::onDoubleClick(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		TextBuffer* buffer = _viewport->buffer();
		int startOfWord = buffer->cursor.startOfWord();
		int endOfWord = buffer->cursor.endOfWord();
		// BUGBUG: issuing this warningMessage cascades into a funky state where the
		// TextCanvas is left in drag mode.  It shouldn't do that.
		//warningMessage("Double");
		buffer->startSelection(startOfWord);
		_viewport->putCursor(endOfWord);
		_viewport->invalidate();
	}
}

void TextEditor::onStartDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		TextBuffer* buffer = _viewport->buffer();
		int desiredPosition = _viewport->nearestPosition(p);
		_viewport->putCursor(desiredPosition);
		buffer->startSelection(desiredPosition);
	}
}

void TextEditor::onDrag(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		TextBuffer* buffer = _viewport->buffer();
		int desiredPosition = _viewport->nearestPosition(p);
		extendSelection();
		_viewport->putCursor(desiredPosition);
	}
}

void TextEditor::onDrop(MouseKeys mKeys, point p, Canvas* target) {
	onDrag(mKeys, p, target);
}

void TextEditor::onCharacterBoxChanged(dimension characterBox) {
	_sHandler->hSmallIncrement = characterBox.width;
	_sHandler->vSmallIncrement = characterBox.height;
}

void TextEditor::onFunctionKey(FunctionKey fk, ShiftState ss) {
	_popInLostFocus = true;
	int len, loc;
	TextBuffer* buffer = _viewport->buffer();
	if (buffer == null)
		return;
	TextLine* line = buffer->cursor.line();
	switch (fk){
	case	FK_END:
		if (ss & SS_SHIFT)
			extendSelection();
		else
			clearSelection();
		if (ss & SS_CONTROL)
			_viewport->putCursor(buffer->size());
		else
			_viewport->putCursor(line->location() + line->length());
		break;

	case	FK_HOME:
		if (ss & SS_SHIFT)
			extendSelection();
		else
			clearSelection();
		if (ss & SS_CONTROL)
			_viewport->putCursor(0);
		else
			_viewport->putCursor(line->location());
		break;

	case	FK_LEFT:
		if (ss & SS_SHIFT)
			extendSelection();
		else
			clearSelection();
		if (ss & SS_CONTROL)
			_viewport->putCursor(buffer->cursor.previousWord());
		else
			_viewport->putCursor(buffer->cursor.location() - 1);
		break;

	case	FK_RIGHT:
		if (ss & SS_SHIFT)
			extendSelection();
		else
			clearSelection();
		if (ss & SS_CONTROL)
			_viewport->putCursor(buffer->cursor.nextWord());
		else
			_viewport->putCursor(buffer->cursor.location() + 1);
		break;

	case	FK_UP:
		if (ss & SS_CONTROL) {
			if (ss & SS_SHIFT)
				break;
			_sHandler->vSmallUpMove();
		} else {
			if (ss & SS_SHIFT)
				extendSelection();
			else
				clearSelection();
			cursorMoveLines(-1);
		}
		break;

	case	FK_DOWN:
		if (ss & SS_CONTROL) {
			if (ss & SS_SHIFT)
				break;
			_sHandler->vSmallDownMove();
		} else {
			if (ss & SS_SHIFT)
				extendSelection();
			else
				clearSelection();
			cursorMoveLines(1);
		}
		break;

	case	FK_PRIOR:
		if (ss & SS_SHIFT)
			extendSelection();
		else
			clearSelection();
		if (ss & SS_CONTROL)
			cursorMoveLines(_viewport->topFullLine() - buffer->cursor.lineno());
		else
			cursorMoveLines(-_viewport->height() / _viewport->lineHeight());
		break;

	case	FK_NEXT:
		if (ss & SS_SHIFT)
			extendSelection();
		else
			clearSelection();
		if (ss & SS_CONTROL)
			cursorMoveLines(_viewport->bottomFullLine() - buffer->cursor.lineno());
		else
			cursorMoveLines(_viewport->height() / _viewport->lineHeight());
		break;

	case	FK_BACK:
		if (buffer->hasSelection()) {
			deleteSelection();
			break;
		}
		// Backspace at start of buffer is a no-op.
		if (buffer->cursor.atStartOfBuffer())
			break;
		_viewport->putCursor(buffer->cursor.location() - 1);
		apply(new DeleteCharsCommand(_viewport, buffer->cursor.location(), 1));
		break;

	case	FK_DELETE:
		if (buffer->hasSelection()) {
			deleteSelection();
			break;
		}
		apply(new DeleteCharsCommand(_viewport, buffer->cursor.location(), 1));
		break;

	case	FK_A:
		if (ss == SS_CONTROL)
			selectAll();
		break;

	case	FK_C:
		if (ss == SS_CONTROL)
			copySelection();
		break;

	case	FK_F:
		if (ss == SS_CONTROL)
			openSearchPanel();
		break;

	case	FK_H:
		if (ss == SS_CONTROL)
			openSearchReplacePanel();
		break;

	case	FK_S:
		if (ss == SS_CONTROL)
			buffer->save();
		break;

	case	FK_V:
		if (ss == SS_CONTROL)
			pasteCutBuffer();
		break;

	case	FK_X:
		if (ss == SS_CONTROL)
			cutSelection();
		break;

	case	FK_Y:
		if (ss == SS_CONTROL) {
			clearSelection();
			buffer->history.redo();
		}
		break;

	case	FK_Z:
		if (ss == SS_CONTROL) {
			clearSelection();
			buffer->history.undo();
		}
		break;

	case	FK_TAB:
		onCharacter('\t');
		break;

	case	FK_RETURN:
		onCharacter('\n');
		break;

	case	FK_F3:
		if (ss == 0) {
			startSearch();
			nextMatch();
		}
		else if (ss == SS_SHIFT)
			previousMatch();
	}
}

void TextEditor::onNavBarClick(MouseKeys mKeys, point p, Canvas* target) {
	if (mKeys & MK_LBUTTON){
		int lineno = _navigationBar->effectiveLineno(p);
		TextBuffer* buffer = _viewport->buffer();
		int desiredPosition = buffer->line(lineno)->location();
		_viewport->putCursor(desiredPosition);
	}
}

void TextEditor::selectAll() {
	TextBuffer* buffer = _viewport->buffer();
	buffer->startSelection(0);
	_viewport->putCursor(buffer->size());
	_viewport->invalidate();
}

void TextEditor::clearSelection() {
	TextBuffer* buffer = _viewport->buffer();
	if (buffer->hasSelection()) {
		buffer->clearSelection();
		_viewport->invalidate();
	}
}

void TextEditor::extendSelection() {
	TextBuffer* buffer = _viewport->buffer();
	buffer->extendSelection();
	_viewport->invalidate();
}

void TextEditor::deleteSelection() {
	TextBuffer* buffer = _viewport->buffer();
	int loc1 = buffer->cursor.location();
	int loc2 = buffer->selection.location();
	if (loc1 > loc2) {
		int x = loc2;
		loc2 = loc1;
		loc1 = x;
	}
	if (loc2 != loc1)
		apply(new DeleteCharsCommand(_viewport, loc1, loc2 - loc1));
	clearSelection();
}

void TextEditor::cutSelection() {
	copySelection();
	deleteSelection();
}

void TextEditor::copySelection() {
	TextBuffer* buffer = _viewport->buffer();
	int loc1 = buffer->cursor.location();
	int loc2 = buffer->selection.location();
	if (loc1 == loc2)
		return;
	if (loc1 > loc2) {
		int x = loc2;
		loc2 = loc1;
		loc1 = x;
	}
	string text;
	buffer->read(loc1, loc2 - loc1, &text);
	Clipboard c(_canvas->rootCanvas());
	c.write(text);
}

void TextEditor::pasteCutBuffer() {
	Clipboard c(_canvas->rootCanvas());
	string text;

	if (c.read(&text)) {
		TextBuffer* buffer = _viewport->buffer();
		if (buffer->hasSelection())
			deleteSelection();
		apply(new InsertCharsCommand(_viewport, buffer->cursor.location(), text.c_str(), text.size()));
	}
}

void TextEditor::openSearchPanel() {
	closeSearchReplacePanel(null);
	if (_searchPanel == null) {
		_searchLabel = new Label(30);
		_searchLabel->set_leftMargin(3);
		_searchLabel->setBackground(&editableBackground);
		if (_searchReplacePanel)
			_searchLabel->set_value(_srSearchLabel->value());
		Bevel* go = new Bevel(2, new Label("Go"));
		Bevel* close = new Bevel(2, new Label("Close"));
		Grid* g = new Grid();
			g->cell(new Label("Search:"));
			g->cell(new Bevel(2, true, _searchLabel), true);
			g->cell(go);
			g->cell(close);
		g->complete();
		g->setBackground(&buttonFaceBackground);
		_searchPanel = new Bevel(2, g);
		ButtonHandler* gbh = new ButtonHandler(go, 0);
		gbh->click.addHandler(this, &TextEditor::goSearch);
		ButtonHandler* bh = new ButtonHandler(close, 0);
		bh->click.addHandler(this, &TextEditor::closeSearchPanel);
		_searchField = _canvas->rootCanvas()->field(_searchLabel);
		_searchField->carriageReturn.addHandler(this, &TextEditor::nextMatch);
	}

		// If we are already showing the search panel, do nothing.

	if (_searchPanel->parent != _popInPanel) {
		if (_popInPanel->child)
			_popInPanel->child->prune();
		_popInPanel->append(_searchPanel);
	}
	startSearch();
	_popInPanel->rootCanvas()->setKeyboardFocus(_searchLabel);
	_popInLostFocus = false;
}

void TextEditor::goSearch(Bevel* target) {
	clearSelection();
	nextMatch();
}

void TextEditor::closeSearchPanel(Bevel* target) {
	_searchPanel->prune();
	_viewport->rootCanvas()->setKeyboardFocus(_viewport);
}

void TextEditor::openSearchReplacePanel() {
	if (_searchReplacePanel == null) {
		_srSearchLabel = new Label(30);
		_srSearchLabel->set_leftMargin(3);
		_srSearchLabel->setBackground(&editableBackground);
		_replaceLabel = new Label(30);
		_replaceLabel->set_leftMargin(3);
		_replaceLabel->setBackground(&editableBackground);
		_nextButtonFace = new Label("Next");
		Bevel* next = new Bevel(2, _nextButtonFace);
		_replaceButtonFace = new Label("Replace");
		Bevel* replace = new Bevel(2, _replaceButtonFace);
		_replaceAllButtonFace = new Label("Replace All");
		Bevel* replaceAll = new Bevel(2, _replaceAllButtonFace);
		Bevel* close = new Bevel(2, new Label("Close"));
		Grid* g = new Grid();
			g->cell(new Label("Search:"));
			g->cell(new Bevel(2, true, _srSearchLabel), true);
			g->row();
			g->cell(new Label("Replace:"));
			g->cell(new Bevel(2, true, _replaceLabel), true);
			g->cell(next);
			g->cell(replace);
			g->cell(replaceAll);
			g->cell(close);
		g->complete();
		g->setBackground(&buttonFaceBackground);
		_searchReplacePanel = new Bevel(2, g);
		ButtonHandler* gbh = new ButtonHandler(next, 0);
		gbh->click.addHandler(this, &TextEditor::nextSearchReplace);
		ButtonHandler* rbh = new ButtonHandler(replace, 0);
		rbh->click.addHandler(this, &TextEditor::doReplace);
		ButtonHandler* rabh = new ButtonHandler(replaceAll, 0);
		rabh->click.addHandler(this, &TextEditor::doReplaceAll);
		ButtonHandler* bh = new ButtonHandler(close, 0);
		bh->click.addHandler(this, &TextEditor::closeSearchReplacePanel);
		_srSearchField = _canvas->rootCanvas()->field(_srSearchLabel);
		_replaceField = _canvas->rootCanvas()->field(_replaceLabel);
		_replaceField->carriageReturn.addHandler(this, &TextEditor::srReturn);
		_srSearchLabel->valueChanged.addHandler(this, &TextEditor::srKeyChanged);
	}
	if (_searchReplacePanel->parent != _popInPanel) {
		// Sync up the search fields
		if (_searchPanel)
			_srSearchLabel->set_value(_searchLabel->value());
		if (_popInPanel->child)
			_popInPanel->child->prune();
		_popInPanel->append(_searchReplacePanel);
	}
	srTakeFocus();
}

void TextEditor::nextSearchReplace(Bevel* target) {
	if (_popInLostFocus)
		srTakeFocus();
	if (!advanceToNext())
		warningMessage("Not found");
}

void TextEditor::doReplace(Bevel* target) {
	if (_popInLostFocus)
		srTakeFocus();
	if (!_replacePossible)
		return;
	replaceInstance();
	nextSearchReplace(target);
}

void TextEditor::doReplaceAll(Bevel* target) {
	if (_popInLostFocus)
		srTakeFocus();
	if (!_replacePossible)
		return;
	do
		replaceInstance();
	while (advanceToNext());
}

bool TextEditor::advanceToNext() {
	TextBuffer* buffer = _viewport->buffer();
	const string& key = _srSearchLabel->value();
	if (key.size() == 0)
		return true;
	int foundLocation = buffer->search(key, &buffer->cursor, &_searchStart);
	if (foundLocation == script::FILE_OFFSET_UNDEFINED)
		return false;
	else {
		_viewport->putCursor(foundLocation);
		srKeyChanged();
		return true;
	}
}

void TextEditor::replaceInstance() {
	TextBuffer* buffer = _viewport->buffer();
	if (buffer->hasSelection())
		deleteSelection();
	string text = _replaceLabel->value();
	apply(new InsertCharsCommand(_viewport, buffer->cursor.location(), text.c_str(), text.size()));
}

void TextEditor::srTakeFocus() {
	startSearch();
	_popInPanel->rootCanvas()->setKeyboardFocus(_srSearchLabel);
	srKeyChanged();
	_popInLostFocus = false;
}

void TextEditor::srReturn() {
	if (!_replacePossible)
		return;
}

void TextEditor::srKeyChanged() {
	const string& key = _srSearchLabel->value();
	if (key.size() == 0) {
		_nextButtonFace->set_textColor(&display::disabledButtonText);
		_replaceButtonFace->set_textColor(&display::disabledButtonText);
		_replaceAllButtonFace->set_textColor(&display::disabledButtonText);
		_replacePossible = false;
		return;
	}
	TextBuffer* buffer = _viewport->buffer();
	int loc = buffer->cursor.locationInLine();
	int result = buffer->cursor.line()->search(key, loc, &buffer->cursor);
	if (result == buffer->cursor.location()) {
		_replaceButtonFace->set_textColor(&display::black);
		_replaceAllButtonFace->set_textColor(&display::black);
		_replacePossible = true;
		clearSelection();
		buffer->startSelection(result + key.size());
		_viewport->putCursor(result);
	} else {
		_replaceButtonFace->set_textColor(&display::disabledButtonText);
		_replaceAllButtonFace->set_textColor(&display::disabledButtonText);
		_replacePossible = false;
	}
	_nextButtonFace->set_textColor(&display::black);
}

void TextEditor::closeSearchReplacePanel(Bevel* target) {
	if (_searchReplacePanel) {
		if (_searchPanel)
			_searchLabel->set_value(_srSearchLabel->value());
		_searchReplacePanel->prune();
		_viewport->rootCanvas()->setKeyboardFocus(_viewport);
	}
}

void TextEditor::startSearch() {
	TextBuffer* buffer = _viewport->buffer();
	buffer->yank(&_searchStart);
	_searchStart.put(&buffer->cursor);
}

void TextEditor::nextMatch() {
	TextBuffer* buffer = _viewport->buffer();
	int foundLocation;
	if (_searchPanel) {
		const string& key = _searchLabel->value();
		foundLocation = buffer->search(key, &buffer->cursor, &_searchStart);
	} else if (_searchReplacePanel) {
		const string& key = _srSearchLabel->value();
		foundLocation = buffer->search(key, &buffer->cursor, &_searchStart);
	} else
		foundLocation = script::FILE_OFFSET_UNDEFINED;
	if (foundLocation == script::FILE_OFFSET_UNDEFINED)
		warningMessage("Not found");
	else {
		_viewport->putCursor(foundLocation);
		_viewport->rootCanvas()->setKeyboardFocus(_viewport);
	}
}

void TextEditor::previousMatch() {
}

void TextEditor::apply(Undo *u) {
	_viewport->buffer()->history.addUndo(u);
}

void TextEditor::cursorMoveLines(int amount) {
	if (amount == 0)
		return;
	TextBuffer* buffer = _viewport->buffer();
	int lineno = buffer->cursor.lineno();
	if (amount < 0) {
		// If we are already at the top of the file, do
		// no more.
		if (lineno == 0)
			return;
	} else {
		// If we are already at the bottom of the file,
		// do no more
		if (lineno == buffer->lineCount() - 1)
			return;
	}
	lineno += amount;
	if (lineno < 0)
		lineno = 0;
	else if (lineno >= buffer->lineCount())
		lineno = buffer->lineCount() - 1;
	int d = buffer->desiredColumn;
	TextLine* line = buffer->line(lineno);
	int loc = _viewport->nearestPosition(line, d);
	_viewport->putCursor(loc);
	buffer->desiredColumn = d;
}

TextCanvas::TextCanvas() {
	_buffer = null;
	tabSize = 4;
	_changedHandler = null;
	_deletedHandler = null;
	_insertedHandler = null;
	_blink = null;
	_font = null;
}

TextCanvas::~TextCanvas() {
	if (_blink != null)
		_blink->kill(this);
}

void TextCanvas::set_buffer(TextBuffer* b) {
	if (_buffer) {
		_buffer->viewChanged.removeHandler(_changedHandler);
		_buffer->deleted.removeHandler(_deletedHandler);
		_buffer->inserted.removeHandler(_insertedHandler);
		_buffer->loaded.removeHandler(_loadedHandler);
		rootCanvas()->afterInputEvent.removeHandler(_undoHandler);
		_changedHandler = null;
		_deletedHandler = null;
		_insertedHandler = null;
		_loadedHandler = null;
		_undoHandler = null;
	}
	_buffer = b;
	if (_buffer) {
		_changedHandler = _buffer->viewChanged.addHandler(this, &TextCanvas::onChanged);
		_deletedHandler = _buffer->deleted.addHandler(this, &TextCanvas::onDeleted);
		_insertedHandler = _buffer->inserted.addHandler(this, &TextCanvas::onInserted);
		_loadedHandler = _buffer->loaded.addHandler(this, &TextCanvas::onLoaded);
		RootCanvas* rc = rootCanvas();
		if (rc)
			_undoHandler = rc->afterInputEvent.addHandler(&_buffer->history, &UndoStack::rememberCurrentUndo);
	}
	jumbleContents();
}

int TextCanvas::hScroll(int x) {
	if (_buffer == null)
		return 0;
	int w = _buffer->pixelWidth();
	if (x >= w)
		x = w - bounds.width();
	if (x < 0)
		x = 0;
	if (_buffer->leftColumn != x) {
		_buffer->leftColumn = x;
		invalidate();
		return 1;
	}
	return 0;
}

int TextCanvas::vScroll(int y) {
	if (_buffer == null)
		return 0;
	if (y >= _buffer->lineCount() * _emSize.height)
		y = _buffer->lineCount() * _emSize.height - bounds.height();
	if (y < 0)
		y = 0;
	if (_buffer->topRow != y) {
		_buffer->topRow = y;
		invalidate();
		return 1;
	}
	return 0;
}

Anchor* TextCanvas::hoverAt(point p) {
	int hitLine = (_buffer->topRow + p.y - bounds.topLeft.y) / _emSize.height;
	TextLine* line = _buffer->line(hitLine);
	if (line == null)
		return null;
	DrawContext ctx;
	ctx.canvas = this;
	ctx.device = device();
	ctx.device->set_font(bindFont(monoFont()));
	ctx.lineHeight = _emSize.height;
	ctx.tabStop = _emSize.width * tabSize;
	ctx.topLeft.x = bounds.topLeft.x + -_buffer->leftColumn;
	ctx.topLeft.y = hitLine * _emSize.height - _buffer->topRow;
	return line->hoverAt(ctx, _buffer->leftColumn + p.x - bounds.topLeft.x);
}

void TextCanvas::putCursor(script::fileOffset_t location) {
	repaintCursor();
	_buffer->cursor.show = true;
	_buffer->cursor.moveTo(location);
	_buffer->cacheSelection();
	TextLine* ln = _buffer->cursor.line();
	DrawContext ctx;
	ctx.canvas = this;
	ctx.device = device();
	ctx.tabStop = _emSize.width * tabSize;
	ctx.lineHeight = _emSize.height;
	ctx.device->set_font(bindFont(monoFont()));
	int x = _buffer->cursor.pixelOffset(ctx);
	_buffer->desiredColumn = x;
	int y = _buffer->cursor.lineno() * _emSize.height;
	dimension sz = bounds.size();
	int changes = 0;
	if (x < _buffer->leftColumn)
		changes += hScroll(x);
	else if (x + _emSize.width > _buffer->leftColumn + sz.width)
		changes += hScroll(x + _emSize.width - sz.width);
	if (y < _buffer->topRow)
		changes += vScroll(y);
	if (y >= _buffer->topRow + bounds.height())
		changes += vScroll(y + _emSize.height - sz.height);
	if (changes)
		viewportMoved.fire(_buffer->leftColumn, _buffer->topRow);
	repaintCursor();
}

script::fileOffset_t TextCanvas::nearestPosition(point p) {
	int hitLine = (_buffer->topRow + p.y - bounds.topLeft.y) / _emSize.height;
	if (hitLine < 0)
		return 0;
	if (hitLine >= _buffer->lineCount())
		return _buffer->size();
	TextLine* line = _buffer->line(hitLine);
	DrawContext ctx;
	ctx.canvas = this;
	ctx.device = device();
	ctx.tabStop = _emSize.width * tabSize;
	ctx.lineHeight = _emSize.height;
	ctx.device->set_font(bindFont(monoFont()));
	return line->nearestPosition(ctx, p.x + _buffer->leftColumn - bounds.topLeft.x);
}

script::fileOffset_t TextCanvas::nearestPosition(TextLine* line, int x) {
	DrawContext ctx;
	ctx.canvas = this;
	ctx.device = device();
	ctx.tabStop = _emSize.width * tabSize;
	ctx.lineHeight = _emSize.height;
	ctx.device->set_font(bindFont(monoFont()));
	return line->nearestPosition(ctx, x);
}

void TextCanvas::hScrollNotice(int x) {
	hScroll(x);
}

void TextCanvas::vScrollNotice(int y) {
	vScroll(y);
}

int TextCanvas::pixelHeight() {
	if (_buffer)
		return _buffer->lineCount() * _emSize.height;
	else
		return 0;
}

int TextCanvas::topFullLine() {
	int fraction = _buffer->topRow % _emSize.height;
	int lineno = _buffer->topRow / _emSize.height;
	// If the top row is in mid-line in any way, treat the next line as full
	if (fraction)
		lineno++;
	return lineno;
}

int TextCanvas::bottomFullLine() {
	return (_buffer->topRow + bounds.height()) / _emSize.height - 1;
}

void TextCanvas::getKeyboardFocus() {
	RootCanvas* rc = rootCanvas();
	if (rc != null){
		_blink = startTimer(rc->blinkInterval);
		_blink->tick.addHandler(this, &TextCanvas::onTick);
		repaintCursor();
	}
	if (_buffer)
		_buffer->cursor.blinkOn = true;
}

void TextCanvas::loseKeyboardFocus() {
	_blink->kill(null);
	_blink = null;
	if (_buffer) {
		_buffer->cursor.blinkOn = false;
		repaintCursor();
	}
}

void TextCanvas::onTick() {
	if (_buffer == null)
		return;
	_buffer->cursor.blinkOn = !_buffer->cursor.blinkOn;
	repaintCursor();
}

void TextCanvas::repaintCursor() {
	if (_buffer == null || !_buffer->ready())
		return;
	DrawContext ctx;
	ctx.canvas = this;
	ctx.device = device();
	if (ctx.device == null)
		return;
	ctx.device->set_font(bindFont(monoFont()));
	ctx.tabStop = _emSize.width * tabSize;
	ctx.lineHeight = _emSize.height;
	int x = _buffer->cursor.pixelOffset(ctx) - _buffer->leftColumn;
	int y = _buffer->cursor.lineno() * _emSize.height - _buffer->topRow;
	if (x < 0 || x >= bounds.width())
		return;
	if (y + _emSize.height <= 0 || y >= bounds.height())
		return;
	x += bounds.topLeft.x;
	y += bounds.topLeft.y;
	rectangle r;
	r.topLeft.x = x;
	r.topLeft.y = y;
	r.opposite.x = x + 1;
	r.opposite.y = y + _emSize.height;
	invalidate(r);
}

dimension TextCanvas::measure() {
	if (_emSize.height == 0) {
		Device* dev = device();
		dev->set_font(bindFont(monoFont()));
		_emSize = dev->textExtent("M", 1);
		_emSize.height++;
		characterBoxChanged.fire(_emSize);
	}
	if (_buffer && _buffer->ready()) {
		computeWidestLine();
		return dimension(_buffer->pixelWidth(), pixelHeight());
	} else
		return dimension(0, pixelHeight());
}

void TextCanvas::paint(Device* b) {
	if (_buffer == null || !_buffer->ready())
		return;
	if (_undoHandler == null)
		_undoHandler = rootCanvas()->afterInputEvent.addHandler(&_buffer->history, &UndoStack::rememberCurrentUndo);
	preferredSize();
	b->set_font(bindFont(monoFont()));
	int m = b->backMode(TRANSPARENT);
	b->setTextColor(0);
	display::rectangle clip = b->clip();
	int topRow = _buffer->topRow + clip.top() - bounds.topLeft.y;
	int nextRow = topRow + clip.height();
	int topLine = topRow / _emSize.height;
	int nextLine = (nextRow + _emSize.height - 1) / _emSize.height;
	if (nextLine > _buffer->lineCount())
		nextLine = _buffer->lineCount();
	DrawContext ctx;
	ctx.canvas = this;
	ctx.device = b;
	ctx.lineHeight = _emSize.height;
	ctx.tabStop = _emSize.width * tabSize;
	ctx.topLeft.x = bounds.topLeft.x + -_buffer->leftColumn;
	ctx.topLeft.y = bounds.topLeft.y + topLine * _emSize.height - _buffer->topRow;
	ctx.normalTextBackground = background();
	ctx.normalTextColor = 0;
	ctx.selectionTextBackground = &selectionBackground;
	ctx.selectionTextColor = 0xffffff;
	for (int i = topLine; i < nextLine; i++) {
		_buffer->markSelectionState(ctx, i);
		_buffer->line(i)->paint(ctx);
		ctx.topLeft.y += _emSize.height;
	}
	b->backMode(m);
}

void TextCanvas::computeWidestLine() {
	DrawContext ctx;
	ctx.canvas = this;
	ctx.device = device();
	ctx.tabStop = _emSize.width * tabSize;
	ctx.device->set_font(bindFont(monoFont()));
	if (_buffer && _buffer->ready())
		_buffer->computeWidestLine(ctx);
}

void TextCanvas::onChanged(TextLine* line) {
	if (_emSize.height == 0)
		return;
	int topRow = _buffer->topRow;
	int nextRow = topRow + bounds.height();
	int topLine = topRow / _emSize.height;
	int nextLine = (nextRow + _emSize.height - 1) / _emSize.height;
	if (nextLine > _buffer->lineCount())
		nextLine = _buffer->lineCount();
	for (int i = topLine; i < nextLine; i++) {
		if (_buffer->line(i) == line) {
			int lineRow = i * _emSize.height - topRow;
			int nextRow = lineRow + _emSize.height;
			int h = bounds.height();
			if (lineRow < h &&
				nextRow > 0) {
				rectangle r = bounds;
				if (lineRow < 0)
					lineRow = 0;
				if (nextRow > h)
					nextRow = h;
				r.topLeft.y += lineRow;
				r.opposite.y = r.topLeft.y + nextRow;
				invalidate(r);
			}
			return;
		}
	}
}

void TextCanvas::onDeleted(int line, int count) {
	if (_emSize.height == 0)
		return;
	int topRow = _buffer->topRow;
	int nextRow = topRow + bounds.height();
	int topLine = topRow / _emSize.height;
	int nextLine = (nextRow + _emSize.height - 1) / _emSize.height;
	if (nextLine > _buffer->lineCount())
		nextLine = _buffer->lineCount();
	if (line < nextLine) {
		if (topLine < line)
			topLine = line;
		int lineRow = topLine * _emSize.height - topRow;
		rectangle r = bounds;
		if (lineRow < 0)
			lineRow = 0;
		r.topLeft.y += lineRow;
		invalidate(r);
	}
}

void TextCanvas::onInserted(int line, int count) {
	onDeleted(line, count);
}

void TextCanvas::onLoaded() {
	display::RootCanvas* root = rootCanvas();
	if (root == null)
		return;
	root->run((display::Canvas*)this, &display::Canvas::invalidate);
}

BoundFont* TextCanvas::bindFont(Font* font) {
	if (_font == null)
		_font = font->currentFont(rootCanvas());
	return _font;
}

NavigationBar::NavigationBar(ScrollableCanvas* scroller, TextCanvas* canvas) {
	_scroller = scroller;
	_canvas = canvas;
	_buffer = null;
	_changeHandler = null;
}

dimension NavigationBar::measure() {
	return dimension(10, 0);
}

void NavigationBar::paint(display::Device* device) {
	if (_buffer && _buffer->ready()) {
		int height = bounds.height();
		int yBase = bounds.topLeft.y;
		if (_scroller->usesVerticalBar()) {
			yBase += SCROLLBAR_WIDTH;
			height -= 2 * SCROLLBAR_WIDTH;
		}
		if (_scroller->usesHorizontalBar())
			height -= SCROLLBAR_WIDTH;
		AnchorPriority lastAp = AP_NONE;
		int lastApY = -10;
		for (int i = 0; i < _buffer->lineCount(); i++) {
			TextLine* line = _buffer->line(i);
			AnchorPriority ap = line->maxPriority();
			if (ap == AP_NONE)
				continue;
			int yIntercept = i * height / _buffer->lineCount();
			switch (ap) {
			case AP_INFORMATIVE:
				device->set_pen(infoPen);
				break;

			case AP_WARNING:
				device->set_pen(warningPen);
				break;

			case AP_ERROR:
				device->set_pen(errorPen);
				break;

			}
			device->line(bounds.topLeft.x, yBase + yIntercept, bounds.opposite.x, yBase + yIntercept);
		}
	}
}

int NavigationBar::effectiveLineno(point p) {
		int height = bounds.height();
		int yBase = bounds.topLeft.y;
		if (_scroller->usesVerticalBar()) {
			yBase += SCROLLBAR_WIDTH;
			height -= 2 * SCROLLBAR_WIDTH;
		}
		if (_scroller->usesHorizontalBar())
			height -= SCROLLBAR_WIDTH;
		int y = p.y - yBase;
		if (y < 0)
			return 0;
		if (y >= height)
			return _buffer->lineCount() - 1;
		return y *  _buffer->lineCount() / height;
}

void NavigationBar::set_buffer(TextBuffer* buffer) {
	bool changed = false;
	if (_buffer) {
		_buffer->anchorChanged.removeHandler(_changeHandler);
		_changeHandler = null;
		_buffer = null;
		changed = true;
	}
	if (buffer) {
		_buffer = buffer;
		_changeHandler = _buffer->anchorChanged.addHandler(this, &NavigationBar::onAnchorChanged);
		changed = true;
	}
	if (changed)
		invalidate();
}

void NavigationBar::onAnchorChanged(Anchor* anchor) {
	invalidate();
}

TextBuffer::TextBuffer(const string &filename) {
	_filename = filename;
	_lines = null;
	_lineCount = 0;
	_firstBadLineNumber = 0;
	_firstBadLocation = 0;
	_anchors = null;
	_ready = false;
	topRow = 0;
	leftColumn = 0;
	desiredColumn = 0;
}

TextBuffer::~TextBuffer() {
	for (int i = 0; i < _lineCount; i++)
		delete _lines[i];
	delete [] _lines;
	if (_anchors)
		_anchors->release();
}

void TextBuffer::unload() {
	_ready = false;
	cursor.pop();
	clearSelection();
	if (_lineCount) {
		for (int i = 0; i < _lineCount; i++)
			delete _lines[i];
		delete [] _lines;
		_lines = null;
		_lineCount = 0;
	}
}

bool TextBuffer::load() {
	process::MutexLock m(&_lock);

	// If we've already loaded something, you can't load it again.
	if (_lineCount)
		return false;
	FILE* fp = fileSystem::openTextFile(_filename);
	if (fp == null)
		return false;
	bool result = fileSystem::readAll(fp, &_original);
	_age = fileSystem::lastModified(fp);
	fclose(fp);
	if (!result) {
		_original.clear();
		return false;
	} else {
		finishLoad();
		return true;
	}
}

void TextBuffer::loadFromMemory(const char* s, int length) {
	process::MutexLock m(&_lock);

	// If we've already loaded something, you can't load it again.
	if (_lineCount)
		return;
	_original = string(s, length);
	_age = time(null);
	finishLoad();
}

void TextBuffer::finishLoad() {
	_lineCount = 1;
	for (int i = 0; i < _original.size(); i++)
		if (_original[i] == '\n')
			_lineCount++;
	_lines = new TextLine*[_lineCount];
	int position = 0;
	for (int i = 0; i < _lineCount; i++) {
		const char* start = _original.c_str() + position;
		int offset = position;
		while (position < _original.size() && _original[position] != '\n')
			position++;
		// Never include the newline itself
		_lines[i] = new TextLine(this, start, position - offset);
		// But skip it to get to the next line.
		if (position < _original.size())
			position++;
	}
	put(&cursor, 0);
	_ready = true;
	loaded.fire();
}

void TextBuffer::assignToBuffer(Anchor* anchor) {
	if (anchor->_line)
		return;
	if (_anchors)
		_anchors->_prevBuffer = anchor;
	anchor->_nextBuffer = _anchors;
	_anchors = anchor;
}

void TextBuffer::renumber(const TextLine* line) {
	int startAt = _firstBadLineNumber;
	if (_firstBadLocation < startAt)
		startAt = _firstBadLocation;
	int location = 0;
	if (startAt > 0) {
		TextLine* t = _lines[startAt - 1];
		location = t->_location + t->_length + 1;
	}
	for (int i = startAt; i < _lineCount; i++) {
		TextLine* t = _lines[i];
		t->_lineNumber = i;
		t->_location = location;
		if (t == line)
			break;
		location += t->_length + 1;
	}
	_firstBadLineNumber = 
		_firstBadLocation = line->_lineNumber + 1;
}

TextLine* TextBuffer::lineAtLocation(int location) const {
	if (location < 0)
		return null;
	int low = 0;
	int high = _firstBadLocation - 1;
	while (low <= high) {
		int mid = (high + low) / 2;
		if (location < _lines[mid]->_location)
			high = mid - 1;
		else if (location > _lines[mid]->_location + _lines[mid]->_length)
			low = mid + 1;
		else
			return _lines[mid];
	}
	for (int i = _firstBadLocation; i < _lineCount; i++)
		if (location < _lines[i]->location())
			return _lines[i - 1];

		// If we are still within the span of the last line, plus one character,
		// we consider that still 'within' the text buffer contents.

	TextLine* t = _lines[_lineCount - 1];
	if (location <= t->_location + t->_length)
		return t;
	else
		return null;
}

void TextBuffer::clear() {
	process::MutexLock m(&_lock);

	if (_lineCount) {
		for (int i = 1; i < _lineCount; i++)
			delete _lines[i];
		_lines[0]->reset(null, 0);
	} else {
		_lines = new TextLine*[1];
		_lines[0] = new TextLine(this, null, 0);
	}
	_lineCount = 1;
	moveTo(&cursor, 0);
	clearSelection();
}

bool TextBuffer::hasErrors() {
	for (Anchor* a = _anchors; a != null; a = a->_nextBuffer)
		if (a->priority() == AP_ERROR)
			return true;
	return false;
}

bool TextBuffer::saveAs(const string& filename) {
	if (!fileSystem::createBackupFile(filename))
		return false;
	FILE* fp = fileSystem::createTextFile(filename);
	if (fp == null)
		return false;
	for (int i = 0; i < _lineCount; i++) {
		int result = fwrite(_lines[i]->text(), 1, _lines[i]->length(), fp);
		if (result != _lines[i]->length()) {
			fclose(fp);
			return false;
		}
		// Don't put out a newline after the 'last line'
		if (i < _lineCount - 1) {
			if (fputc('\n', fp) == EOF) {
				fclose(fp);
				return false;
			}
		}
	}
	bool reallySave = (filename == _filename);
	fflush(fp);
	if (reallySave)
		_age = fileSystem::lastModified(fp);
	fclose(fp);
	if (reallySave) {
		history.markSavedState();
		saved.fire();
	}
	return true;
}

void TextBuffer::clearSelection() {
	selection.pop();
}

void TextBuffer::extendSelection() {
	if (selection.line() == null)
		selection.put(&cursor);
}

void TextBuffer::startSelection(int location) {
	selection.pop();
	put(&selection, location);
}

void TextBuffer::cacheSelection() {
	if (selection.line()) {
		if (selection.location() < cursor.location()) {
			_startOfSelection = &selection;
			_endOfSelection = &cursor;
			_startOfSelectionLineno = selection.lineno();
			_endOfSelectionLineno = cursor.lineno();
		} else {
			_startOfSelection = &cursor;
			_endOfSelection = &selection;
			_startOfSelectionLineno = cursor.lineno();
			_endOfSelectionLineno = selection.lineno();
		}
	}
}

void TextBuffer::markSelectionState(DrawContext& ctx, int lineno) {
	if (selection.line()) {
		if (lineno < _startOfSelectionLineno)
			ctx.startingSelection = script::FILE_OFFSET_MAX;
		else if (lineno == _startOfSelectionLineno)
			ctx.startingSelection = _startOfSelection->locationInLine();
		else // if (lineno > _startOfSelectionLineno)
			ctx.startingSelection = 0;
		if (lineno < _endOfSelectionLineno)
			ctx.endingSelection = script::FILE_OFFSET_MAX;
		else if (lineno == _endOfSelectionLineno)
			ctx.endingSelection = _endOfSelection->locationInLine();
		else // if (lineno > _endOfSelectionLineno)
			ctx.endingSelection = 0;
	} else {
		ctx.startingSelection = 0;
		ctx.endingSelection = 0;
	}
}

void TextBuffer::clearAnnotations() {
	Anchor* aNext;
	for (Anchor* a = _anchors; a != null; a = aNext) {
		aNext = a->_nextBuffer;
		if (!a->persistent())
			delete a;
	}
}

void TextBuffer::put(Anchor *anchor, script::fileOffset_t location) {
	if (anchor->_line)
		return;
	assignToBuffer(anchor);
	moveTo(anchor, location);
}

void TextBuffer::moveTo(Anchor *anchor, script::fileOffset_t location) {
	if (anchor->_line) {
		if (has(anchor))
			anchor->popLine();
		else
			return;
	}
	if (location == script::FILE_OFFSET_UNDEFINED)
		return;
	TextLine* ln = lineAtLocation(location);
	if (ln != null)
		ln->put(anchor, location - ln->location());
	else
		_lines[0]->put(anchor, 0);
}

void TextBuffer::yank(Anchor *anchor) {
	if (has(anchor))
		anchor->pop();
}

bool TextBuffer::has(Anchor* anchor) {
	return anchor->_line && anchor->_line->_buffer == this;
}

void TextBuffer::deleteChars(int at, int count) {
	if (at < 0 || count <= 0)
		return;
	TextLine* ln = lineAtLocation(at);
	if (ln != null) {
		invalidateLocation(ln->_lineNumber);
		at -= ln->_location;

		int nextPos = at + count;
		int nextLineNumber = ln->_lineNumber + 1;
		int j = nextLineNumber;
		int remainder = nextPos - ln->length();
		if (remainder > 0) {
			// So, we know we will at least splice a line, so discard the implied newline at the end of this line
			count--;
			remainder--;
			while (j < _lineCount) {
				TextLine* nextLine = _lines[j];
				j++;
				if (remainder > nextLine->length()) {
					// delete the whole line
					remainder -= nextLine->length() + 1;
					count -= nextLine->length() + 1;
				} else {
					// ok, splice in this one
					// TODO: copy anchors
					ln->insertChars(ln->length(), nextLine->text(), nextLine->length());
					break;
				}
			}
			deleteLines(nextLineNumber, j - nextLineNumber);
		}
		ln->deleteChars(at, count);
	}
}

void TextBuffer::insertChars(int at, const char *text, int count) {
	if (at < 0 || count <= 0)
		return;
	TextLine* ln = lineAtLocation(at);
	if (ln != null) {
		invalidateLocation(ln->_lineNumber);
		at -= ln->_location;

		int newlineCount = 0;
		int firstNewline = count;
		for (int j = 0; j < count; j++)
			if (text[j] == '\n') {
				if (firstNewline > j)
					firstNewline = j;
				newlineCount++;
			}
		ln->insertChars(at, text, firstNewline);
		if (newlineCount) {
			text += firstNewline;
			count -= firstNewline;
			at += firstNewline;
			// skip that first newline.
			text++;
			count--;
			int nextLineNumber = ln->_lineNumber + 1;
			insertLines(nextLineNumber, newlineCount);
			// If the current line has more stuff after the insertion point, split that out.
			if (at < ln->length()) {
				TextLine* lastLine = _lines[ln->_lineNumber + newlineCount];
				// TODO: copy anchors
				lastLine->insertChars(0, ln->text() + at, ln->length() - at);
				ln->deleteChars(at, ln->length() - at);
			}
			while (count > 0) {
				ln = _lines[nextLineNumber];
				int nl;
				for (nl = 0; nl < count; nl++)
					if (text[nl] == '\n')
						break;
				ln->insertChars(0, text, nl);
				count -= nl + 1;
				text += nl + 1;
				nextLineNumber++;
			}
		}
	}
}

// Can be called on any thread
void TextBuffer::read(int at, int count, string* output) const {
	process::MutexLock m(&_lock);

	// No undo means no changes, the original text is usable as is.
	if (history.atSavedState()) {
		*output = _original.substr(at, count);
		return;
	}
	output->clear();
	if (at < 0 || count <= 0)
		return;
	TextLine* ln = lineAtLocation(at);
	if (ln != null) {
		at -= ln->_location;
		int i = ln->_lineNumber;
		while (count > 0) {
			int chunk = ln->length() - at;
			if (chunk > count)
				chunk = count;
			if (chunk) {
				output->append(ln->text() + at, chunk);
				count -= chunk;
			}
			if (count) {
				output->push_back('\n');
				count--;
			}
			if (count == 0)
				return;
			i++;
			ln = _lines[i];
			at = 0;
		}
	}
}

void TextBuffer::deleteLines(int at, int count) {
	if (at >= _lineCount || count <= 0)
		return;
	int nextLine = at + count;
	if (nextLine < 0)
		return;
	if (at < 0)
		at = 0;
	if (nextLine >= _lineCount)
		nextLine = _lineCount;
	// Check for overflow
	if (nextLine < at)
		return;
	int remainder = _lineCount - nextLine;

	{
		process::MutexLock m(&_lock);

		invalidateLineNumber(at);
		if (remainder)
			memmove(&_lines[at], &_lines[nextLine], remainder * sizeof (TextLine*));
		_lineCount -= nextLine - at;
	}
	deleted.fire(at, count);
	modified.fire(this);
}

void TextBuffer::insertLines(int at, int count) {
	if (at < 0 || at > _lineCount || count <= 0)
		return;
	TextLine** newLines = new TextLine*[_lineCount + count];
	if (at)
		memcpy(newLines, _lines, at * sizeof (TextLine*));
	for (int i = 0; i < count; i++)
		newLines[at + i] = new TextLine(this, 0, 0);
	int after = _lineCount - at;
	if (after)
		memcpy(newLines + at + count, _lines + at, after * sizeof (TextLine*));

	{
		invalidateLineNumber(at);
		process::MutexLock m(&_lock);

		delete [] _lines;
		_lines = newLines;
		_lineCount += count;
	}
	inserted.fire(at, count);
	modified.fire(this);
}

int TextBuffer::size() const {
	if (_lineCount == 0)
		return 0;
	return _lines[_lineCount - 1]->location() + _lines[_lineCount - 1]->length();
}

TextLine* TextBuffer::line(int i) {
	if (i < 0 || i >= _lineCount)
		return null;
	else
		return _lines[i];
}

void TextBuffer::computeWidestLine(const DrawContext& ctx) {
	if (_lines == null) {
		_widestWidth = 0;
		_widestLine = null;
		return;
	}
	_widestLine = _lines[0];
	_widestWidth = _widestLine->width(ctx);
	for (int i = 1; i < _lineCount; i++) {
		int width = _lines[i]->width(ctx);
		if (width > _widestWidth) {
			_widestLine = _lines[i];
			_widestWidth = width;
		}
	}
}

bool TextBuffer::needsSave() {
	return !history.atSavedState();
}

bool TextBuffer::fileExists() {
	return fileSystem::exists(_filename);
}

int TextBuffer::search(const string& key, Anchor* start, Anchor* end) {
	if (start->line() == null)
		return script::FILE_OFFSET_UNDEFINED;
	if (end == null || !has(end))
		end = start;
	int result = start->line()->search(key, start->locationInLine() + 1, end);
	if (result != script::FILE_OFFSET_UNDEFINED)
		return result;
	if (start->line() == end->line() && start->locationInLine() < end->locationInLine())
		return script::FILE_OFFSET_UNDEFINED;
	int lineno = start->lineno();
	for (;;) {
		lineno++;
		if (lineno >= this->_lineCount)
			lineno = 0;
		result = _lines[lineno]->search(key, 0, end);
		if (result != script::FILE_OFFSET_UNDEFINED)
			return result;
		if (_lines[lineno] == end->line())
			return script::FILE_OFFSET_UNDEFINED;
	}
}

void TextBuffer::invalidateLineNumber(int line) {
	if (line < _firstBadLineNumber) {
		_firstBadLineNumber = line;
		invalidateLocation(line);
	}
}

void TextBuffer::invalidateLocation(int line) {
	if (line < _firstBadLocation)
		_firstBadLocation = line;
}

TextLine::TextLine(TextBuffer* buffer, const char *text, int length) {
	_buffer = buffer;
	_anchors = null;
	reset(text, length);
}

TextLine::~TextLine() {
	if (_text != _original)
		delete [] _text;
	while (_anchors) {
		Anchor* a;
		a = _anchors;
		a->pop();
		a->release();
	}
}

void TextLine::reset(const char* text, int length) {
	_original = text;
	_originalLength = length;
	_length = length;
	_text = const_cast<char*>(text);
	_changed = true;
	_lineNumber = 0;
	_location = 0;
}

void TextLine::put(Anchor *anchor, script::fileOffset_t location) {
	_anchors = anchor->insert(_anchors, this, location);
	_buffer->viewChanged.fire(this);
}

void TextLine::yank(Anchor *anchor) {
	if (anchor->_line == this) {
		anchor->popLine();
		_buffer->viewChanged.fire(this);
	}
}

void TextLine::deleteChars(int at, int count) {
	if (at >= _length || count <= 0)
		return;
	int nextChar = at + count;
	if (nextChar < 0)
		return;
	if (at < 0)
		at = 0;
	if (nextChar >= _length)
		nextChar = _length;
	// Check for overflow
	if (nextChar < at)
		return;
	if (_anchors)
		_anchors->deleteChars(at, nextChar - at);
	int remainder = _length - nextChar;
	{
		process::MutexLock m(&_buffer->_lock);

		if (remainder)
			memmove(&_text[at], &_text[nextChar], remainder);
		_length -= nextChar - at;
	}
	touch();
	_buffer->viewChanged.fire(this);
}

void TextLine::insertChars(int at, const char* text, int count) {
	if (at < 0 || at > _length || count <= 0)
		return;
	if (_anchors)
		_anchors->insertChars(at, count);
	int newLength = _length + count;
	int after = _length - at;
	{
		process::MutexLock m(&_buffer->_lock);

		if (_text != _original &&
			((newLength + (1 << BIT_INCREMENT) - 1) >> BIT_INCREMENT) ==
				((_length + (1 << BIT_INCREMENT) - 1) >> BIT_INCREMENT)) {

			if (after)
				memmove(_text + at + count, _text + at, after);
			memcpy(_text + at, text, count);
		} else {
			int allocLength = (newLength + (1 << BIT_INCREMENT) - 1) & ~((1 << BIT_INCREMENT) - 1);
			char* newText = new char[allocLength];
			if (at)
				memcpy(newText, _text, at);
			memcpy(newText + at, text, count);
			if (after)
				memcpy(newText + at + count, _text + at, after);
			if (_text != _original)
				delete [] _text;
			_text = newText;
		}
		_length += count;
	}
	touch();
	_buffer->viewChanged.fire(this);
}

Anchor* TextLine::hoverAt(const DrawContext& ctx, int x) {
	for (Anchor* a = _anchors; a != null; a = a->_nextLine) {
		int aX = pixelOffset(ctx, a->_location);
		if (abs(aX - x) < 3 && a->caption().size()) {
			return a;
		}
	}
	return null;
}

int TextLine::width(const DrawContext& ctx) {
	if (_changed) {
		_width = pixelOffset(ctx, _length);
		_changed = false;
	}
	return _width;
}

int TextLine::pixelOffset(const DrawContext& ctx, int characterPosition) {
	if (characterPosition > _length)
		characterPosition = _length;
	int offset = 0;
	int firstNonTab = 0;
	for (int i = 0; i < characterPosition; i++) {
		if (_text[i] == '\t') {
			if (firstNonTab < i) {
				dimension d = ctx.device->textExtent(_text + firstNonTab, i - firstNonTab);
				offset += d.width;
			}
			if (ctx.tabStop) {
				int partialTab = offset % ctx.tabStop;
				offset += ctx.tabStop - partialTab;
			}
			firstNonTab = i + 1;
		}
	}
	if (firstNonTab < characterPosition) {
		dimension d = ctx.device->textExtent(_text + firstNonTab, characterPosition - firstNonTab);
		offset += d.width;
	}
	return offset;
}

int TextLine::nearestPosition(const DrawContext& ctx, int x) {
	int offset = 0;
	int position = location();
	for (int i = 0; x > 0 && i < _length; i++) {
		if (_text[i] == '\t') {
			if (ctx.tabStop) {
				int partialTab = offset % ctx.tabStop;
				offset += ctx.tabStop - partialTab;
				if (x < ctx.tabStop / (ctx.canvas->tabSize * 2))
					return position;
				x -= ctx.tabStop - partialTab;
			}
		} else {
			dimension d = ctx.device->textExtent(_text + i, 1);
			if (x < d.width / 2)
				return position;
			offset += d.width;
			x -= d.width;
		}
		position++;
	}
	return position;
}

int TextLine::search(const string &key, int startOffset, Anchor *end) {
	int endOffset = 1 + _length - key.size();
	if (end->line() == this && end->_location > startOffset && end->_location < endOffset)
		endOffset = end->_location;
	for (int i = startOffset; i < endOffset; i++) {
		if (memcmp(key.c_str(), _text + i, key.size()) == 0)
			return location() + i;
	}
	return script::FILE_OFFSET_UNDEFINED;
}

int TextLine::lineno() const {
	if (_lineNumber >= _buffer->_firstBadLineNumber ||
		_buffer->_lines[_lineNumber] != this)
		_buffer->renumber(this);
	return _lineNumber;
}

int TextLine::location() const {
	if (_lineNumber >= _buffer->_firstBadLocation ||
		_buffer->_lines[_lineNumber] != this)
		_buffer->renumber(this);
	return _location;
}

int TextLine::previousWord(int offset) const {
	if (offset == 0) {
		int lineno = this->lineno();
		if (lineno == 0)
			return 0;
		TextLine* ln = _buffer->_lines[lineno - 1];
		if (ln->length() == 0)
			return ln->location();
		else
			return ln->previousWord(ln->length());
	} else {
		offset--;
		while (offset && isspace(_text[offset]))
			offset--;
		if (offset && isalnum(_text[offset])) {
			while (offset && isalnum(_text[offset - 1]))
				offset--;
		}
		return location() + offset;
	}
}

int TextLine::nextWord(int offset) const {
	if (offset == _length) {
		int lineno = this->lineno();
		if (lineno == _buffer->lineCount() - 1)
			return _buffer->size();
		TextLine* ln = _buffer->_lines[lineno + 1];
		if (ln->length() > 0 &&
			isspace(*ln->text()))
			return ln->nextWord(0);
		else
			return ln->location();
	} else {
		if (isalnum(_text[offset])) {
			do
				offset++;
			while (offset < _length &&
				   isalnum(_text[offset]));
		} else if (!isspace(_text[offset]))
			offset++;
		while (offset < _length && isspace(_text[offset]))
			offset++;
		return location() + offset;
	}
}

int TextLine::startOfWord(int offset) const {
	if (offset >= _length || isspace(_text[offset])) {
		if (isalnum(_text[offset - 1])) {
			while (offset && isalnum(_text[offset - 1]))
				offset--;
		} else if (isspace(_text[offset - 1])) {
			while (offset && isspace(_text[offset - 1]))
				offset--;
		} else
			return offset - 1;
	} else if (isalnum(_text[offset])) {
		while (offset && isalnum(_text[offset - 1]))
			offset--;
	}
	return location() + offset;
}

int TextLine::endOfWord(int offset) const {
	if (offset == _length)
		return location() + _length;
	else {
		if (isalnum(_text[offset])) {
			do
				offset++;
			while (offset < _length &&
				   isalnum(_text[offset]));
		} else if (isspace(_text[offset])) {
			if (offset == 0 || isspace(_text[offset - 1])) {
				do
					offset++;
				while (offset < _length &&
					   isspace(_text[offset]));
			}
		} else
			offset++;
		return location() + offset;
	}
}

AnchorPriority TextLine::maxPriority() const {
	Anchor* a = _anchors;
	AnchorPriority ap = AP_NONE;
	while (a) {
		if (a->priority() == AP_ERROR)
			return AP_ERROR;
		if (a->priority() > ap)
			ap = a->priority();
		a = a->_nextLine;
	}
	return ap;
}

void TextLine::touch() {
	_changed = true;
	if (_buffer)
		_buffer->modified.fire(_buffer);
}

void TextLine::paint(const DrawContext& ctx) {
	int soFar = 0;
	int prefixLength = 0;
	int endInfix = 0;
	int suffixLength = 0;
	if (ctx.startingSelection) {
		prefixLength = ctx.startingSelection;
		if (prefixLength > _length)
			prefixLength = _length + 1;
		if (prefixLength) {
			ctx.device->setTextColor(ctx.normalTextColor);
			paintRun(ctx, 0, prefixLength, &soFar, ctx.normalTextBackground);
		}
	}
	if (ctx.endingSelection) {
		endInfix = ctx.endingSelection;
		if (endInfix > _length)
			endInfix = _length + 1;
		if (endInfix > prefixLength) {
			ctx.device->setTextColor(ctx.selectionTextColor);
			paintRun(ctx, prefixLength, endInfix, &soFar, ctx.selectionTextBackground);
		}
	}
	if (endInfix < _length) {
		ctx.device->setTextColor(ctx.normalTextColor);
		paintRun(ctx, endInfix, _length, &soFar, ctx.normalTextBackground);
	}
	for (Anchor* a = _anchors; a != null; a = a->_nextLine)
		a->draw(ctx);
}

void TextLine::paintRun(const DrawContext& ctx, int startLocation, int endLocation, int* cumulativeColumn, Background* background) {
	int firstNonTab = startLocation;
	int firstColumn = *cumulativeColumn;
	int backgroundFill = 0;
	if (endLocation > _length) {
		backgroundFill = ctx.canvas->characterWidth();
		endLocation = _length;
	}
	for (int i = startLocation; i < endLocation; i++) {
		if (_text[i] == '\t') {
			if (firstNonTab < i) {
				dimension d = ctx.device->textExtent(_text + firstNonTab, i - firstNonTab);
				rectangle r;
				r.topLeft = ctx.topLeft;
				r.opposite.y = r.topLeft.y + ctx.lineHeight;
				r.topLeft.x += firstColumn;
				r.opposite.x = r.topLeft.x + *cumulativeColumn + d.width;
				if (background)
					background->paint(ctx.canvas, r, ctx.device);
				ctx.device->text(ctx.topLeft.x + *cumulativeColumn, ctx.topLeft.y, _text + firstNonTab, i - firstNonTab);
				*cumulativeColumn += d.width;
				firstColumn = *cumulativeColumn;
			}
			if (ctx.tabStop) {
				int partialTab = *cumulativeColumn % ctx.tabStop;
				*cumulativeColumn += ctx.tabStop - partialTab;
			}
			firstNonTab = i + 1;
		}
	}
	if (firstNonTab >= endLocation)
		*cumulativeColumn += backgroundFill;
	if (*cumulativeColumn > firstColumn) {
		rectangle r;
		r.topLeft = ctx.topLeft;
		r.opposite.y = r.topLeft.y + ctx.lineHeight;
		r.opposite.x = r.topLeft.x + *cumulativeColumn;
		r.topLeft.x += firstColumn;
		if (background)
			background->paint(ctx.canvas, r, ctx.device);
	}
	if (firstNonTab < endLocation) {
		dimension d = ctx.device->textExtent(_text + firstNonTab, endLocation - firstNonTab);
		rectangle r;
		r.topLeft = ctx.topLeft;
		r.opposite.y = r.topLeft.y + ctx.lineHeight;
		r.topLeft.x += *cumulativeColumn;
		r.opposite.x = r.topLeft.x + d.width + backgroundFill;
		if (background)
			background->paint(ctx.canvas, r, ctx.device);
		ctx.device->text(ctx.topLeft.x + *cumulativeColumn, ctx.topLeft.y, _text + firstNonTab, endLocation - firstNonTab);
		*cumulativeColumn += d.width;
	}
}

Anchor::Anchor() {
	_location = 0;
	_prevBuffer = null;
	_nextBuffer = null;
	_prevLine = null;
	_nextLine = null;
	_line = null;
}

Anchor::~Anchor() {
	if (_line)
		_line->_buffer->viewChanged.fire(_line);
	pop();
}

void Anchor::moveTo(script::fileOffset_t location) {
	if (_line)
		_line->_buffer->moveTo(this, location);
}

void Anchor::put(Anchor* anchor) {
	if (_line)
		return;
	if (anchor->line()) {
		anchor->line()->buffer()->assignToBuffer(this);
		insertAfter(anchor);
		_line = anchor->line();
		_location = anchor->locationInLine();
	}
}

void Anchor::insertChars(int at, int count) {
	if (_nextLine)
		_nextLine->insertChars(at, count);
	if (_location > at)
		_location += count;
}

Anchor* Anchor::deleteChars(int at, int count) {
	if (_nextLine)
		_nextLine = _nextLine->deleteChars(at, count);
	if (_location < at)
		return this;
	if (_location >= at + count) {
		_location -= count;
		return this;
	} else if (persistent()) {
		_location = at;
		return this;
	} else {
		Anchor* n = _nextLine;
		delete this;
		return n;
	}
}

void Anchor::release() {
	if (_nextBuffer)
		_nextBuffer->release();
	if (persistent()) {
		_nextBuffer = null;
		_prevBuffer = null;
		_nextLine = null;
		_prevLine = null;
		_line = null;
		_location = 0;
	} else
		delete this;
}

Anchor* Anchor::insert(Anchor* list, TextLine* line, script::fileOffset_t location) {
	if (_line)
		return list;
	_location = location;
	_line = line;
	if (list == null)
		list = this;
	else if (location < list->_location) {
		list->_prevLine = this;
		_nextLine = list;
		list = this;
	} else {
		for (Anchor* a = list; ; a = a->_nextLine) {
			if (location >= a->_location) {
				_nextLine = a->_nextLine;
				if (a->_nextLine)
					a->_nextLine->_prevLine = this;
				_prevLine = a;
				a->_nextLine = this;
				break;
			}
			if (a->_nextLine == null) {
				a->_nextLine = this;
				_prevLine = a;
				break;
			}
		}
	}
	moved(_line->buffer());
	return list;
}

void Anchor::insertAfter(Anchor* anchor) {
	_nextLine = anchor->_nextLine;
	anchor->_nextLine = this;
	if (_nextLine)
		_nextLine->_prevLine = this;
	_prevLine = anchor;
}

void Anchor::pop() {
	if (_line) {
		if (_prevBuffer)
			_prevBuffer->_nextBuffer = _nextBuffer;
		else
			_line->_buffer->_anchors = _nextBuffer;
		if (_nextBuffer)
			_nextBuffer->_prevBuffer = _prevBuffer;
	}
	popLine();
	_prevBuffer = null;
	_nextBuffer = null;
}

void Anchor::popLine() {
	if (_line) {
		if (_prevLine)
			_prevLine->_nextLine = _nextLine;
		else
			_line->_anchors = _nextLine;
		if (_nextLine)
			_nextLine->_prevLine = _prevLine;
		_line = null;
	}
	_location = 0;
	_prevLine = null;
	_nextLine = null;
}

void Anchor::moved(TextBuffer* buffer) {
}

int Anchor::pixelOffset(const DrawContext& ctx) const {
	if (_line)
		return _line->pixelOffset(ctx, _location);
	else
		return 0;
}

int Anchor::lineno() const {
	if (_line)
		return _line->lineno();
	return 0;
}

int Anchor::location() const {
	if (_line)
		return _line->location() + _location;
	return 0;
}

int Anchor::previousWord() const {
	if (_line) 
		return _line->previousWord(_location);
	return 0;
}

int Anchor::nextWord() const {
	if (_line) 
		return _line->nextWord(_location);
	return 0;
}

int Anchor::startOfWord() const {
	if (_line) 
		return _line->startOfWord(_location);
	return 0;
}

int Anchor::endOfWord() const {
	if (_line) 
		return _line->endOfWord(_location);
	return 0;
}

bool Anchor::atStartOfBuffer() const {
	if (_location)
		return false;
	if (_line == null)
		return false;
	return _line->_buffer->_lines[9] == _line;
}

Cursor::Cursor() {
	blinkOn = true;
	show = true;
}

void Cursor::reset() {
}

bool Cursor::persistent() {
	return true;
}

Canvas* Cursor::balloon() {
	return null;
}

const string& Cursor::caption() const {
	static string dummy;

	return dummy;
}

void Cursor::draw(const DrawContext& ctx) {
	if (blinkOn && show){
		int x_offset = pixelOffset(ctx) + ctx.topLeft.x;
		rectangle r = ctx.device->clip();

		ctx.device->set_pen(blackPen);
		ctx.device->line(x_offset, ctx.topLeft.y, x_offset, ctx.topLeft.y + ctx.lineHeight);
	}
}

void Cursor::drawGutter(const DrawContext& ctx) {
}

AnchorPriority Cursor::priority() const {
	return AP_NONE;
}

HiddenAnchor::HiddenAnchor() {
}

bool HiddenAnchor::persistent() {
	return true;
}

Canvas* HiddenAnchor::balloon() {
	return null;
}

const string& HiddenAnchor::caption() const {
	static string dummy;

	return dummy;
}

void HiddenAnchor::draw(const DrawContext& ctx) {}

void HiddenAnchor::drawGutter(const DrawContext& ctx) {}

AnchorPriority HiddenAnchor::priority() const {
	return  AP_NONE;
}

Annotation::Annotation(AnchorPriority priority, const string& label) {
	_priority = priority;
	_label = label;
}

bool Annotation::persistent() {
	return false;
}

Canvas* Annotation::balloon() {
	return null;
}

const string& Annotation::caption() const {
	return _label;
}

void Annotation::draw(const DrawContext& ctx) {
	int x_offset = pixelOffset(ctx) + ctx.topLeft.x;
	// -> draw alert
	switch (_priority) {
	case AP_WARNING:
		ctx.device->set_pen(yellowPen);
		break;

	case AP_ERROR:
		ctx.device->set_pen(redPen);
		break;

	default:
		ctx.device->set_pen(bluePen);
	}
	ctx.device->line(x_offset, ctx.topLeft.y + ctx.lineHeight - 3, x_offset - 1, ctx.topLeft.y + ctx.lineHeight - 1);
	ctx.device->line(x_offset - 1, ctx.topLeft.y + ctx.lineHeight - 1, x_offset + 1, ctx.topLeft.y + ctx.lineHeight - 1);
	ctx.device->line(x_offset + 1, ctx.topLeft.y + ctx.lineHeight - 1, x_offset, ctx.topLeft.y + ctx.lineHeight - 3);
}

void Annotation::drawGutter(const DrawContext& ctx) {
}

AnchorPriority Annotation::priority() const {
	return _priority;
}

void Annotation::moved(TextBuffer* buffer) {
	buffer->anchorChanged.fire(this);
}

TextBufferPool::TextBufferPool() {
}

bool TextBufferPool::loaded(const string &filename) {
	TextBufferManager*const* entry = _buffers.get(filename);
	if (*entry) 
		return true;
	else
		return false;
}

TextBufferManager* TextBufferPool::manage(const string &filename) {
	TextBufferManager*const* entry = _buffers.get(filename);
	if (*entry) 
		return *entry;
	else {
		TextBufferManager* tbm = new TextBufferManager(filename);
		_buffers.insert(filename, tbm);
		return tbm;
	}
}

bool TextBufferPool::hasUnsavedEdits() {
	dictionary<TextBufferManager*>::iterator i = _buffers.begin();

	while (i.valid()) {
		if ((*i)->needsSave())
			return true;
		i.next();
	}
	return false;
}

bool TextBufferPool::saveAll() {
	dictionary<TextBufferManager*>::iterator i = _buffers.begin();

	while (i.valid()) {
		if (!(*i)->save())
			return false;
		i.next();
	}
	return true;
}

TextBufferManager::TextBufferManager(const string &filename) : _buffer(filename) {
	selected.addHandler(this, &TextBufferManager::onSelected);
	unselected.addHandler(this, &TextBufferManager::onUnselected);
	_editor = null;
	_buffer.modified.addHandler(this, &TextBufferManager::onModified);
	_buffer.saved.addHandler(this, &TextBufferManager::onBufferSaved);
	_source = null;
	_state = FILE_NOT_LOADED;
}

bool TextBufferManager::load() {
	if (_buffer.load()) {
		if (fileSystem::writable(_buffer.filename()))
			_state = FILE_OK;
		else
			_state = FILE_READONLY;
		tabModified();
		return true;
	} else {
		_buffer.clear();
		if (_buffer.fileExists())
			_state = FILE_COULD_NOT_BE_READ;
		else
			_state = FILE_DOES_NOT_EXIST;
		tabModified();
		return false;
	}
}

const char* TextBufferManager::tabLabel() {
	return _buffer.filename().c_str();
}

Canvas* TextBufferManager::tabBody() {
	if (_editor)
		return _editor->canvas();
	else
		return null;
}

Canvas* TextBufferManager::onHover() {
	switch (_state) {
	case	FILE_NOT_LOADED:
		return new display::Label(_buffer.filename() + " not loaded");

	case	FILE_READONLY:
		return new display::Label(_buffer.filename() + " read-only");

	case	FILE_COULD_NOT_BE_READ:
		return new display::Label(_buffer.filename() + " could not be read");

	case	FILE_DOES_NOT_EXIST:
		return new display::Label(_buffer.filename() + " does not exist");

	case	FILE_OK:
		return new display::Label(_buffer.filename());

	default:
		return new display::Label(string("Unexpected _state: ") + int(_state));
	}
}

void TextBufferManager::onSelected() {
	if (_editor) {
		if (_source) {
			if (!_source->current())
				return;
		}
		_editor->setBuffer(&_buffer);
		if (_editor->viewport()->rootCanvas())
			_editor->viewport()->rootCanvas()->setKeyboardFocus(_editor->viewport());
	}
}

void TextBufferManager::onUnselected() {
	if (_editor)
		_editor->setBuffer(null);
}

void TextBufferManager::bind(display::TextEditor *editor) {
	_editor = editor;
}

bool TextBufferManager::needsSave() {
	return _buffer.needsSave();
}

bool TextBufferManager::save() {
	if (_buffer.needsSave())
		return _buffer.save();
	else
		return true;
}

void TextBufferManager::reload() {
	_buffer.unload();
	_state = FILE_NOT_LOADED;
	load();
}

bool TextBufferManager::hasErrors() {
	if (errorState())
		return true;
	return _buffer.hasErrors();
}

void TextBufferManager::onModified(TextBuffer* buffer) {
	tabModified();
}

void TextBufferManager::onBufferSaved() {
	tabModified();
}

void TextBufferManager::clearAnnotations() {
	_buffer.clearAnnotations();
}

int TextBufferManager::compare(TextBufferManager* manager) const {
	TextBuffer* buffer = manager->buffer();
	return ::compare(_buffer.filename(), buffer->filename().c_str(), buffer->filename().size());
}

TextBufferSource* TextBufferManager::source(derivative::Web* web) {
	if (_source == null)
		_source = new TextBufferSource(web, this);
	return _source;
}

TextBufferSource::TextBufferSource(derivative::Web* web, TextBufferManager* tbm) : derivative::Object<TextSnapshot>(web) {
	_errorLoading = false;
	_messageLog = null;
	_textBufferManager = tbm;
	tbm->buffer()->modified.addHandler(this, &TextBufferSource::onModified);
	tbm->buffer()->saved.addHandler(this, &TextBufferSource::onSaved);
}

bool TextBufferSource::build() {
	if (_errorLoading)
		return false;
	TextSnapshot* mutableV;
	const derivative::Value<TextSnapshot>* v = derivative::Value<TextSnapshot>::factory(&mutableV);
	if (_textBufferManager->buffer()->lineCount() > 0) {
		if (_age == 0)
			_age = _textBufferManager->buffer()->age();
		_textBufferManager->buffer()->read(0, _textBufferManager->buffer()->size(), &mutableV->image);
		set(v);
		return true;
	}
	if (_textBufferManager->load()) {
		_age = _textBufferManager->buffer()->age();
		_textBufferManager->buffer()->read(0, _textBufferManager->buffer()->size(), &mutableV->image);
		set(v);
		return true;
	}
	_errorLoading = true;
	return false;
}

fileSystem::TimeStamp TextBufferSource::age() {
	return _age;
}

script::MessageLog* TextBufferSource::messageLog() {
	if (_messageLog == null)
		_messageLog = new TextMessageLog(this);
	return _messageLog;
}

void TextBufferSource::commitMessages() {
	if (_messageLog != null) {
		_messageLog->commit();
		_messageLog = null;
	}
}

string TextBufferSource::toString() {
	return "TextBufferSource " + _textBufferManager->buffer()->filename();
}

void TextBufferSource::onModified(TextBuffer* buffer) {
	_age.touch();
	touch();
}

void TextBufferSource::onSaved() {
	_age = _textBufferManager->buffer()->age();
	touch();
}

TextMessageLog::TextMessageLog(TextBufferSource *source) {
	_source = source;
}

void TextMessageLog::commit() {
	if (_source->manager()->tabBody() != null) {
		display::RootCanvas* rootCanvas = _source->manager()->tabBody()->rootCanvas();
		if (rootCanvas != null) {
			rootCanvas->run(this, &TextMessageLog::commitImpl);
			return;
		}
	}
	commitImpl();
}

void TextMessageLog::commitImpl() {
	_source->manager()->buffer()->clearAnnotations();
	for (int i = 0; i < _messages.size(); i++)
		_source->manager()->buffer()->put(_messages[i], _locations[i]);
	_messages.clear();
	_locations.clear();
	_source->manager()->tabModified();
	delete this;
}

void TextMessageLog::log(script::fileOffset_t loc, const string& msg) {
	_messages.push_back(new Annotation(AP_INFORMATIVE, msg));
	_locations.push_back(loc);
}

void TextMessageLog::error(script::fileOffset_t loc, const string& msg) {
	_messages.push_back(new Annotation(AP_ERROR, msg));
	_locations.push_back(loc);
}

}  // namespace display
