#include "../common/platform.h"
#include "derivative.h"

#include <typeinfo.h>
#include "machine.h"

namespace derivative {

Web::Web() {
	_workers = new process::ThreadPool(2);
	_workers->idle.addHandler(this, &Web::onWorkersIdle);
	_currentGeneration = 0;
	_builtGeneration = 0;
	_buildingGeneration = 0;
	_anyActionFailed = false;
	_cancelled = false;
	_waiters = null;
	_clearFailedDependencies = true;
}

Web::~Web() {
	delete _workers;
}

// *** Runs on any thread ***
void Web::add(ObjectBase* object) {
	process::MutexLock m(&_lock);

	debugPrint(string("add @") + int(object) + ": " + object->toString() + "\n");
	// If the object is already known, don't add it again.
	for (int i = 0; i < _objects.size(); i++)
		if (_objects[i] == object)
			return;
	_objects.push_back(object);
	if (current())
		_currentGeneration = _builtGeneration + 1;
}
// *** Runs on any thread ***
void Web::touch() {
	process::MutexLock m(&_lock);

	if (current())
		_currentGeneration = _builtGeneration + 1;
	else if (building()) {
		_cancelled = true;
		_currentGeneration++;
	}
}

bool Web::has(ObjectBase* object) {
	process::MutexLock m(&_lock);

	for (int i = 0; i < _objects.size(); i++)
		if (_objects[i] == object)
			return true;
	return false;
}

void Web::forget(ObjectBase* object) {
	process::MutexLock m(&_lock);

	while (object->_busy) {
		m.unlock();
		waitFor(object);
		m.lock();
	}
	int deleteThis = -1;
	for (int i = 0; i < _objects.size(); i++)
		if (_objects[i] == object)
			deleteThis = i;
		else if (object->dependencies_size() > 0)
			object->removeDependency(_objects[i]);
	if (deleteThis >= 0) {
		if (deleteThis != _objects.size() - 1)
			_objects[deleteThis] = _objects[_objects.size() - 1];
		_objects.resize(_objects.size() - 1);
	}
}

const ValueBase* Web::pin(const ObjectBase* object) {
	process::MutexLock m(&_lock);

	const ValueBase* v = object->value();
	if (v != null)
		v->addRef();
	return v;
}

ObjectBase* Web::objectBuiltFrom(ObjectBase* dependency, int i) {
	process::MutexLock m(&_lock);

	if (_objects[i]->_derivatives.size() > i) {
		debugPrint("objectBuiltFrom " + dependency->toString() + " " + i + ": " + _objects[i]->_derivatives[i]->toString() + "\n");
		return _objects[i]->_derivatives[i];
	} else {
		debugPrint("objectBuiltFrom " + dependency->toString() + " " + i + ": null\n");
		return null;
	}
}

void Web::addToBuild(ObjectBase* object) {
	process::MutexLock m(&_lock);
	debugPrint("addToBuild " + object->toString() + "\n");
	if (building() && !object->_busy && object->_builtGeneration < _buildingGeneration) {
		object->_busy = false;
		if (object->unfinished(_buildingGeneration) == 0) {
			object->_busy = true;
			_workers->run(object, &ObjectBase::makeCurrent);
		}
	}
}
// *** Runs on any thread ***
void Web::makeCurrent() {
	process::MutexLock m(&_lock);
	if (current() || building())
		return;

	_anyActionFailed = false;
	_cancelled = false;
	_currentGeneration++;
	_buildingGeneration = _currentGeneration;
	for (int i = 0; i < _objects.size(); i++) {
		_objects[i]->_busy = false;
		if (_objects[i]->unfinished(_buildingGeneration) == 0) {
			_objects[i]->_busy = true;
			_workers->run(_objects[i], &ObjectBase::makeCurrent);
		}
	}
}
// *** Runs on worker thread ***
void Web::buildFinished(ObjectBase* object, bool success) {
	process::MutexLock m(&_lock);
	debugPrint("@" + string(int(object)) + ": " + object->toString() + " buildFinished: " + int(success) + "\n");
	object->_busy = false;
	object->_builtGeneration = _buildingGeneration;
	object->_ready.signal();
	if (success) {
		if (_cancelled)
			return;
	} else {
		_anyActionFailed = true;
		if (!_clearFailedDependencies)
			return;
	}
	// Recheck, because a build might impart new dependencies.
	if (object->_finished == object->_dependencies.size()) {
		for (int i = 0; i < object->_derivatives.size(); i++) {
			ObjectBase* o = object->_derivatives[i];
			clearDependency(o, success);
		}
	}
}
// *** Runs on any thread ***
void Web::checkDependency(ObjectBase* derivative, ObjectBase* source) {
	process::MutexLock m(&_lock);
	if (building() && source->_builtGeneration >= _buildingGeneration)
		clearDependency(derivative, true);
}
// *** Runs on any thread, lock must be held ***
void Web::clearDependency(ObjectBase* object, bool success) {
	if (object->_generation < _buildingGeneration) {
		object->_generation = _buildingGeneration;
		object->_finished = 0;
	}
	object->_finished++;
	if (object->_finished == object->_dependencies.size() && !object->_busy) {
		object->_busy = true;
		_workers->run(object, &ObjectBase::makeCurrent);
	}
}
// *** Runs on worker thread ***
void Web::onWorkersIdle() {
	process::MutexLock m(&_lock);
	if (_cancelled || _anyActionFailed)
		_buildingGeneration = _builtGeneration;
	else
		_builtGeneration = _buildingGeneration;
	for (Waiters* w = _waiters; w != null; ) {
		// Deallocation of w is done by the waiting thread as soon as it
		// wakes up, so we cannot rely on w->next after signal returns.
		Waiters* wNext = w->next;
		w->waitEvent.signal();
		w = wNext;
	}
	_waiters = null;
	if (_cancelled)
		makeCurrent();
}
// *** Runs on non-worker thread ***
void Web::wait(unsigned millisecondsWait) {
	Waiters w;

	process::MutexLock m(&_lock);

	if (building()) {
		w.next = _waiters;
		_waiters = &w;
		m.unlock();
		w.waitEvent.wait(millisecondsWait);
	}
}

void Web::debugDump() {
	debugPrint(string("idle threads ") + _workers->idleThreads() + " building " + _buildingGeneration + " built " + _builtGeneration + "\n");
	for (int i = 0; i < _objects.size(); i++) {
		debugPrint(string("[") + i + "]: " + _objects[i]->toString() + "(gen " + _objects[i]->_generation + " built " + _objects[i]->_builtGeneration + ")");
		if (_objects[i]->_busy)
			debugPrint(" (busy)");
		if (_objects[i]->unfinished(_buildingGeneration))
			debugPrint(" waiting on " + string(_objects[i]->unfinished(_buildingGeneration)));
		debugPrint("\n");
	}
}

bool Web::waitFor(ObjectBase* source) {
	for (;;) {
		{
			process::MutexLock m(&_lock);
			if (_cancelled || source->_builtGeneration >= _buildingGeneration)
				break;
		}
//		debugDump();
		if (!_workers->runOne(&source->_ready))
			return false;
	}
	return true;
}

ObjectBase::ObjectBase(Web* web) {
	_web = web;
	_generation = 0;
	_finished = 0;
	_busy = false;
	_builtGeneration = 0;
}

ObjectBase::~ObjectBase() {
	if (_web)
		_web->forget(this);
}

fileSystem::TimeStamp ObjectBase::age() {
	return fileSystem::TimeStamp::UNDEFINED;
}

// *** Runs on worker thread ***
void ObjectBase::makeCurrent() {
	for (int i = 0; i < _dependencies.size(); i++) {
		if (!_dependencies[i]->current()) {
			_web->buildFinished(this, false);
			return;
		}
	}
	_generation = _web->generation();
	debugPrint("@" + string(int(this)) + ": " + toString() + " makeCurrent\n");
	fileSystem::TimeStamp a = age();
	// By default an object does not short-cut execution based on a
	// clock time.  However, as a service, a set of objects may report
	// a defined 'age' and the object will participate in the short-cutting
	// of a web build.
	//
	// Note: short-cutting will only happen if all objects report a defined
	// age that is less than the downstream object.
	if (a != fileSystem::TimeStamp::UNDEFINED) {
		if (_dependencies.size() > 0) {
			fileSystem::TimeStamp newest = _dependencies[0]->age();
			for (int i = 1; i < _dependencies.size(); i++) {
				fileSystem::TimeStamp depAge = _dependencies[i]->age();
				if (depAge != fileSystem::TimeStamp::UNDEFINED) {
					newest = a;
					break;
				}
				if (newest < depAge)
					newest = depAge;
			}
			if (newest < a) {
				debugPrint("@" + string(int(this)) + ": " + "    already current\n");
				_web->buildFinished(this, true);
				return;
			}
		}
		_web->buildFinished(this, build());
	}
}

void ObjectBase::dependsOn(ObjectBase* source) {
	for (int i = 0; i < source->_derivatives.size(); i++)
		if (source->_derivatives[i] == this)
			return;
	_web->add(source);
	source->_derivatives.push_back(this);
	_dependencies.push_back(source);
	_web->checkDependency(this, source);
}

void ObjectBase::removeDependency(ObjectBase* source) {
	for (int i = 0; i < _dependencies.size(); i++)
		if (_dependencies[i] == source) {
			_dependencies.remove(i);
			break;
		}
	for (int i = 0; i < source->_derivatives.size(); i++)
		if (source->_derivatives[i] == this) {
			source->_derivatives.remove(i);
			return;
		}
}

void ObjectBase::touch() {
	if (_web != null)
		_web->touch();
}

bool ObjectBase::current() {
	if (_web == null)
		return false;
	else
		return _web->generation() <= _builtGeneration;
}

const ValueBase* ObjectBase::pin() const {
	if (_web != null)
		return _web->pin(this);
	else
		return null;
}

}  // namespace derivative