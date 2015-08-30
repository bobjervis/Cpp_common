#include "../common/platform.h"
#include "execute.h"

#include "../display/root_canvas.h"
#include <shellapi.h>

bool launchUrl(display::RootCanvas* root, const string& url) {
	HINSTANCE h;
	HWND hwnd = null;

	if (root)
		hwnd = root->handle();
	string s;
	for (int i = 0; i < url.size(); i++) {
		if (url[i] == '\\')
			s.push_back('/');
		else
			s.push_back(url[i]);
	}

	h = ShellExecute(hwnd, "open", s.c_str(), null, null, SW_SHOWNORMAL);
	return (int)h > 32;
}
