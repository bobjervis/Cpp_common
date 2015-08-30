#include "../common/platform.h"
#include "undo.h"

namespace display {

UndoStack::UndoStack() {
	_currentUndo = null;
	_lastUndo = null;
	_undoStack = null;
	_redoStack = null;
	_savedUndo = null;
}

UndoStack::~UndoStack() {
	clear();
}

void UndoStack::clear() {
	_currentUndo = null;
	_lastUndo = null;
	clearRedo();
	clearUndo();
	_savedUndo = null;
}

void UndoStack::rememberCurrentUndo() {
	if (_currentUndo == null)
		return;
	if (_currentUndo->_next != null)
		_currentUndo = new GroupUndo(_currentUndo);
	_currentUndo->_next = _undoStack;
	_undoStack = _currentUndo;
	clearRedo();
	_currentUndo = null;
	_lastUndo = null;
}

void UndoStack::undo() {
	rememberCurrentUndo();
	if (_undoStack == null)
		return;
	_undoStack->revert();

		// Pop the front command off the undo stack

	Undo* u = _undoStack;
	_undoStack = u->_next;

		// and push it onto the redo stack

	u->_next = _redoStack;
	_redoStack = u;
}

void UndoStack::redo() {
	rememberCurrentUndo();
	if (_redoStack == null)
		return;
	_redoStack->apply();

		// Pop the front command off the redo stack

	Undo* u = _redoStack;
	_redoStack = u->_next;

		// and push it onto the undo stack

	u->_next = _undoStack;
	_undoStack = u;
}

void UndoStack::addUndo(Undo* u) {
	if (_currentUndo == null)
		_currentUndo = u;
	else
		_lastUndo->_next = u;
	u->_next = null;
	_lastUndo = u;
	u->apply();
}

Undo* UndoStack::nextUndo() {
	return _undoStack;
}

Undo* UndoStack::nextRedo() {
	return _redoStack;
}

void UndoStack::revertReverse(Undo* u) {
	if (u->_next != null)
		revertReverse(u->_next);
	u->revert();
}

void UndoStack::markSavedState() {
	_savedUndo = _undoStack;
}

bool UndoStack::atSavedState() const {
	return _savedUndo == _undoStack && _currentUndo == null;
}

void UndoStack::clearRedo() {
	while (_redoStack) {
		Undo* u = _redoStack;
		_redoStack = u->_next;
		u->discard();
		delete u;
	}
}

void UndoStack::clearUndo() {
	while (_undoStack) {
		Undo* u = _undoStack;
		_undoStack = u->_next;
		delete u;
	}
}

Undo::~Undo() {
}

void Undo::apply() {
}

void Undo::revert() {
}

void GroupUndo::apply() {
	for (Undo* u = subList; u != null; u = u->_next)
		u->apply();
}

void GroupUndo::revert() {
	UndoStack::revertReverse(subList);
}

void GroupUndo::discard() {
	for (Undo* u = subList; u != null; u = u->_next)
		u->discard();
}


} // namespace display
