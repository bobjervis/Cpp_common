#include "string.h"
#include <windows.h>

namespace locale {

bool endsWithIgnoreCase(const string& s1, const string& s2);
bool startsWithIgnoreCase(const string& s1, const string& s2);

bool toDate(const string& s, SYSTEMTIME* t);

}  // namespace locale