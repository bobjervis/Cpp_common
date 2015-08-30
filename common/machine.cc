#include "../common/platform.h"
#include <stdlib.h>
#include <windows.h>

#include "../display/window.h"
#include "machine.h"

static bool messagesEnabled = true;

void disableMessages() {
	messagesEnabled = false;
}

void fatalMessage(const string& s) {
	if (messagesEnabled)
		display::messageBox(null, s, "Fatal", 0);
	exit(1);
}

void warningMessage(const string& s) {
	if (messagesEnabled)
		display::messageBox(null, s, "Warning", 0);
}

char string::dummy;

const int OUTPUT_BLOCK = 32;

void debugPrint(const string& s) {
	char chars[OUTPUT_BLOCK + 1];

	for (int i = 0; i < s.size(); i += OUTPUT_BLOCK){
		int j = OUTPUT_BLOCK;
		if (i + j > s.size())
			j = s.size() - i;
		memcpy(chars, &s.c_str()[i], j);
		chars[j] = 0;

		OutputDebugString(chars);
	}
}

Milliseconds millisecondMark() {
	return GetTickCount();
}
