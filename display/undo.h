#pragma once
#define null 0

namespace display {

class GroupUndo;
class Undo;

class UndoStack {
public:
	UndoStack();

	~UndoStack();

	void addUndo(Undo* u);

	void clear();

	Undo* nextUndo();

	Undo* nextRedo();

	void undo();

	void redo();

	void rememberCurrentUndo();

	void markSavedState();

	bool atSavedState() const;

	static void revertReverse(Undo* u);

private:
	void clearRedo();

	void clearUndo();

	Undo*		_savedUndo;
	Undo*		_currentUndo;
	Undo*		_lastUndo;
	Undo*		_undoStack;
	Undo*		_redoStack;
};

class Undo {
	friend UndoStack;
	friend GroupUndo;
public:
	Undo() { _next = null; }

protected:
	virtual ~Undo();

private:

	virtual void apply() = 0;

	virtual void revert() = 0;

	virtual void discard() = 0;

	Undo*		_next;
};

class GroupUndo : public Undo {
	typedef Undo super;
public:
	GroupUndo(Undo* s) {
		subList = s;
	}

	virtual void apply();

	virtual void revert();

	virtual void discard();

private:
	Undo*		subList;
};

} // namespace display