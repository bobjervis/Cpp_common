#pragma once
#include <time.h>
#include "dictionary.h"
#include "file_system.h"
#include "process.h"
#include "string.h"
#include "vector.h"

namespace derivative {

class TimeStamp;
template<class T>
class Object;
class ObjectBase;
class ValueBase;
/*
 *	Web
 *
 *	Note on 'generation's:
 *
 *		Change is registered through the marking of generations.
 *		Rather than rely on external time signatures, the objects
 *		are each assigned a generation.  The web itself maintains
 *		three generation values:
 *
 *			current generation		initially 0
 *			built generation		initially 0
 *			building generation		initially 0
 *
 *
 *		The web is considered 'current' when it's built generation
 *		is the same as current generation.
 *
 *		As each new object is added to the web, it is assigned a
 *		generation of zero.  Additionally, the 'touch' method is
 *		called.
 *
 *	Note on thread model
 *
 *		A web of objects consists of a set of Object<T>'s and a
 *		dependency relationship D.  Every pair <d1, d2> in D means
 *		d1 depends on d2.  The build of d1 cannot start until the
 *		build of d2 is complete.
 *
 *		If objects also define an 'age', then the build of an object
 *		in the web can be 'short-cut' to save time.
 *
 *		The build method of each object must be thread-safe with respect
 *		to the build of any other object in the web.
 *
 *		The web creates a thread pool and threads take an available
 *		object at random among the set of object for which no dependencies
 *		are unresolved.
 *
 *		A build method clears the dependency when it finishes.  What
 *		happens to the downstream objects depends on the selected failure
 *		behavior.  If the objects have unreliable build methods, it may be
 *		desirable to pause and allow for corrective action.  Normally, though,
 *		the behavior is to abort the build. 
 */
class Web {
	friend ObjectBase;
public:
	Web();

	~Web();

	void add(ObjectBase* object);

	template<class D, class S>
	D* manageFile(const string& filename, S* source, dictionary<D*>* catalog) {
		process::MutexLock m(&_lock);
		D** entry = catalog->get(filename);
		if (*entry != null)
			return *entry;
		D* file = new D(source);
		file->dependsOn(source);
		catalog->insert(filename, file);
		return file;
	}

	void addToBuild(ObjectBase* object);

	void makeCurrent();

	bool waitFor(ObjectBase* source);
	/*
	 *	wait
	 *
	 *	Waits for all tasks in the build to complete.
	 */
	void wait(unsigned millisecondsWait = INFINITE);
	/*
	 *	touch
	 *
	 *	If the web is current, set the current generation
	 *	to the built generation + 1.
	 *
	 *	Otherwise, if the web is building, cancel the
	 *	build and increment the current generation.
	 */
	void touch();

	bool has(ObjectBase* object);
	/*
	 *	objectBuiltFrom
	 *
	 *	Returns the ith object built from this dependency, or
	 *	null if there is no ith object.  The objects are numbered
	 *	from 0, so an object that builds 3 others will have 
	 *	non-null values for i = 0, 1 or 2.
	 */
	ObjectBase* objectBuiltFrom(ObjectBase* dependency, int i);

	void forget(ObjectBase* object);

	const ValueBase* pin(const ObjectBase* object);

	void setDebuggingMode(bool clearFailedDependencies) {
		_clearFailedDependencies = clearFailedDependencies;
	}

	int generation() const { return _buildingGeneration; }

	bool building() const { return _buildingGeneration > _builtGeneration; }

	bool current() const { return _builtGeneration >= _currentGeneration; }

	bool cancelled() const { return _cancelled; }

	bool clearFailedDependencies() const { return _clearFailedDependencies; }

	bool anyActionFailed() const { return _anyActionFailed; }

	void debugDump();

private:
	class Waiters {
	public:
		process::SignalingEvent		waitEvent;
		Waiters*					next;
	};

	void buildFinished(ObjectBase* object, bool success);

	void checkDependency(ObjectBase* derivative, ObjectBase* source);

	void clearDependency(ObjectBase* object, bool success);

	void onWorkersIdle();

	bool							_clearFailedDependencies;

	process::Mutex					_lock;
	vector<ObjectBase*>				_objects;				// guarded by _lock
	process::ThreadPool*			_workers;				// guarded by _lock
	int								_currentGeneration;		// guarded by _lock
	int								_builtGeneration;		// guarded by _lock
	int								_buildingGeneration;	// guarded by _lock
	bool							_anyActionFailed;		// guarded by _lock
	bool							_cancelled;				// guarded by _lock
	Waiters*						_waiters;				// guarded by _lock
};

class ValueBase {
public:
	ValueBase() {
		_references = 0;
	}

	void addRef() const {
		_references++;
	}

	void release() const {
		_references--;
		if (_references == 0)
			delete this;
	}

protected:
	virtual ~ValueBase() {}

private:

	mutable int		_references;
};

template<class T>
class Value : public ValueBase {
	friend Object<T>;
public:
	Value<T>() {
	}

	static const Value<T>* factory(T** mutator = null) {
		Value<T>* v = new Value<T>();
		if (mutator != null)
			*mutator = &v->_instance;
		return v;
	}

	T* operator -> () {
		return &_instance;
	}

	const T* instance() const { 
		return &_instance; 
	}

	const T* operator -> () const {
		return &_instance;
	}

private:
	T				_instance;
};

class ObjectBase {
	friend Web;
public:
	ObjectBase(Web* web);

	virtual ~ObjectBase();

	virtual bool build() = 0;

	virtual fileSystem::TimeStamp age();

	virtual string toString() = 0;

	virtual const ValueBase* value() const = 0;

	void makeCurrent();

	void dependsOn(ObjectBase* source);

	void removeDependency(ObjectBase* source);

	void touch();
	/*
	 *	current
	 *
	 *	Returns true if the object is 'current' with respect to the
	 *	Web in which it is located, false otherwise.
	 */
	bool current();
	/*
	 *	unfinished
	 *
	 *	Returns the count of unfinished dependencies
	 *	that this object is still waiting on in the
	 *	given generation.
	 */
	int unfinished(int generation) {
		if (generation <= _generation)
			return _dependencies.size() - _finished;
		else
			return _dependencies.size();
	}

	const ValueBase* pin() const;
	
	Web* web() const { return _web; }

	int dependencies_size() const { return _dependencies.size(); }

private:
	Web*							_web;
	int								_generation;
	int								_finished;
	vector<ObjectBase*>				_dependencies;
	vector<ObjectBase*>				_derivatives;
	bool							_busy;
	int								_builtGeneration;
	process::SignalingEvent			_ready;
};

template<class T>
class Object : public ObjectBase {
public:
	Object<T>(Web* web) : ObjectBase(web) { _value = null; }

	~Object<T>() {
		if (_value != null)
			_value->release();
	}

	const Value<T>* pin() const {
		return (const Value<T>*)ObjectBase::pin();
	}

	bool hasValue() const { return _value != null; }

	virtual const ValueBase* value() const {
		return _value;
	}

	Event changed;

protected:
	void set(const Value<T>* newValue) {
		if (newValue != null)
			newValue->addRef();
		if (_value != null)
			_value->release();
		_value = newValue; 
		changed.fire();
	}

private:
	const Value<T>*					_value;
};

}  // namespace derivative
