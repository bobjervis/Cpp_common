#pragma once
#include <typeinfo.h>
#include "dictionary.h"
#include "string.h"
#include "vector.h"

namespace script {

class Parser;

class Atom {
public:
	virtual ~Atom();

	virtual bool validate(Parser* parser);

	virtual bool isRunnable() const;

	virtual bool run();

	virtual Atom* get(const string& name) const;

	template<class T>
	bool containedBy(T** output) {
		Atom* a = this;
		*output = null;
		for (;;) {
			Atom* p = a->get("parent");
			if (p == null)
				return false;
			if (typeid(*p) == typeid(T)) {
				*output = (T*)p;
				return true;
			}
			a = p;
		}
	}

	virtual bool put(const string& name, Atom* value);

	virtual Atom* getIndexed(int i) const;

	virtual int size() const;

	virtual string toSource() = 0;

	virtual string toString() = 0;

	Atom* operator [] (int i) const;
};

class Object : public Atom {
public:
	~Object();

	virtual bool isRunnable() const;

	virtual string toSource();

	virtual string toString();

	virtual Atom* get(const string& name) const;
	/*
	 * put
	 *
	 * Put method defines the name to have the given value.
	 */
	virtual bool put(const string& name, Atom* value);

	bool runAnyContent();

	bool runAllContent();

private:
	dictionary<Atom*>	_properties;
};

class TextRun : public Atom {
public:
	TextRun(const char* text, int length);

	virtual string toSource();

	virtual string toString();

private:
	string				_content;
};

class String : public Atom {
public:
	//String(const char* text, int length);

	String(const string& s);

	virtual string toSource();

	virtual string toString();

private:
	string				_content;
};

class Null : public Atom {
public:
	virtual string toSource();

	virtual string toString();
};

class Vector : public Atom {
public:
	Vector(vector<Atom*>* value);

	~Vector();

	virtual string toSource();

	virtual string toString();

	virtual Atom* getIndexed(int i) const;

	virtual int size() const;

	const vector<Atom*>& value() const { return _value; }

private:
	vector<Atom*>		_value;
};

}  // namespace script
