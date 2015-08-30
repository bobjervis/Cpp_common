#include "../common/platform.h"
#include <ctype.h>

#include <time.h>
#include "locale.h"

namespace locale {

bool endsWithIgnoreCase(const string& s1, const string& s2) {
	if (s2.size() > s1.size())
		return false;
	for (int i = s1.size() - s2.size(), j = 0; j < s2.size(); j++, i++)
		if (tolower(s1[i]) != tolower(s2[j]))
			return false;
	return true;
}

bool startsWithIgnoreCase(const string& s1, const string& s2) {
	if (s2.size() > s1.size())
		return false;
	for (int i = 0; i < s2.size(); i++)
		if (tolower(s1[i]) != tolower(s2[i]))
			return false;
	return true;
}

bool toDate(const string& s, SYSTEMTIME* d) {
	memset(d, 0, sizeof *d);

		// First get the month

	int i = s.find('/');
	if (i == string::npos)
		return false;
	d->wMonth = atoi(s.substr(0, i).c_str());
	i++;
	int j = s.find('/', i);
	if (j == string::npos)
		return false;
	d->wDay = atoi(s.substr(i, j - i).c_str());
	j++;
	i = s.find(' ', j);
	if (i == string::npos)
		d->wYear = atoi(s.substr(j).c_str());
	else {
		d->wYear = atoi(s.substr(j, i - j).c_str());
		i++;
		j = s.find(':', i);
		if (j == string::npos)
			return false;
		d->wHour = atoi(s.substr(i, j - i).c_str());
		j++;
		d->wMinute = atoi(s.substr(j).c_str());
	}
	return true;
}

}  // namespace locale