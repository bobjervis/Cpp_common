#pragma once
#define null 0

template<class I>
class list {
public:
	list() {
		_base = null;
	}

	void pop(I* me) {
		if (me->next() == me) {
			if (me != _base) {
				// Inconsistency: why are you trying to pop an item off a list it isn't part of?
				// TODO: add error handling
			}
			_base = null;
		} else {
			if (me == _base)
				_base = _me->next();
			me->pop();
		}
	}

private:
	I*		_base;
};
