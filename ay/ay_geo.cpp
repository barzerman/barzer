#include "ay_geo.h"

namespace ay
{
namespace geo
{
Unit unitFromString(std::string str)
{
	for (size_t i = 0; i < str.size(); ++i)
		str[i] = tolower(str[i]);
	
	if (str == "km" || str == "kilometre")
		return Unit::Kilometre;
	else if (str == "m" || str == "metre")
		return Unit::Metre;
	else if (str == "mile")
		return Unit::Mile;
	else if (str == "deg" || str == "dg" || str == "degree")
		return Unit::Degree;
	else
		return Unit::Invalid;
}
}
}
