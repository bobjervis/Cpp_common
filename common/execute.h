#pragma once

#include "string.h"

namespace display {

class RootCanvas;

}  // namespace display

bool launchUrl(display::RootCanvas* root, const string& url);
