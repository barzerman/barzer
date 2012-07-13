#include "ay_keymaps.h"
#include "ay_utf8.h"

namespace ay
{
namespace km
{
	static const char qwerty[] = "qwertyuiop[]asdfghjkl;'zxcvbnm,.QWERTYUIOP{}ASDFGHJKL:\"ZXCVBNM<>";
	static const StrUTF8 ycuken ("йцукенгшщзхъфывапролджэячсмитьбюЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮ");

	bool engToRus(const char *eng, size_t size, std::string& rus)
	{
		bool converted = false;
		rus.reserve(size);
		for (size_t i = 0; i < size; ++i)
		{
			const char ch = eng[i];
			const char *pos = strchr(qwerty, ch);
			if (!pos)
				continue;

			rus.append(ycuken[pos - qwerty], 2);

			if (!converted && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')))
				converted = true;
		}

		return converted;
	}
}
}
