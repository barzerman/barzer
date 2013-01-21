#include "ay_keymaps.h"
#include <iostream>
#include "ay_utf8.h"

namespace ay
{
namespace km
{
	namespace
	{
		const char qwerty[] = "qwertyuiop[]asdfghjkl;'zxcvbnm,.QWERTYUIOP{}ASDFGHJKL:\"ZXCVBNM<>";
		const StrUTF8 ycuken ("йцукенгшщзхъфывапролджэячсмитьбюЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮ");

		const struct TableGenerator
		{
			enum { TableSize = 127 };

			typedef char RuLetter_t[2];
			RuLetter_t m_table[TableSize];

			TableGenerator ()
			: m_table{}
			{
				for (unsigned char i = 0; i < TableSize; ++i)
				{
					const char *pos = strchr(qwerty, i);

					if (pos)
						ycuken[pos - qwerty].copyToBufNoNull(m_table[i]);
					else
						m_table[i][0] = m_table[i][1] = 0;
				}
			}
		} TableGen;
	}

	bool engToRus(const char *eng, size_t size, std::string& rus)
	{
		bool converted = false;
		rus.reserve(size * 2);
		for (size_t i = 0; i < size; ++i)
		{
			const char ch = eng[i];
			const auto letter = TableGen.m_table[static_cast<size_t>(ch < 127 && ch > 0 ? ch : 0)];
			if (!*reinterpret_cast<const uint16_t*>(letter))
				continue;

			rus.append(letter, 2);
			converted = true;
		}

		return converted;
	}
}
}
