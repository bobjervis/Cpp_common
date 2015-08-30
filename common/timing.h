#pragma once

#include "vector.h"

namespace process {

class Mutex;

}  // namespace process;

namespace timing {

typedef __int64 tick;

class Interval {
public:
	int				parentIndex;		// index in interval vector of parent interval
	const char*		tag;
	tick			entry;
	tick			start;
	tick			end;
	tick			follow;
};

class Timer {
public:
	Timer(const char* tag);
	
	~Timer();

private:
	int _index;
};

void enableProfiling();

void disableProfiling();

void defineSnapshot(vector<Interval>* output);

void print(vector<Interval>& snapshot);

__int64 frequency();

}  // namespace timing
