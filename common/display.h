#pragma once
#include <windows.h>
#include "string.h"

namespace display {

class RootCanvas;

class Canvas {
};

class RootCanvas : public Canvas {
	typedef Canvas super;
public:
	HWND	handle;
};

}  // namespace display
