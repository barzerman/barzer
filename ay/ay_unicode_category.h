
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///
#include "ay_utf8.h"
namespace ay {

struct UnicodeClassifier {
	static bool isPunct(uint32_t);
	static bool isSpace(uint32_t);
	static bool isNumber(uint32_t);
};

}
