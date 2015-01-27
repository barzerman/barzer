/*
 * ay_unicode_category.cpp
 *
 *  Created on: Jan 26, 2015
 *      Author: polter
 */



#include "ay_unicode_category.h"

namespace {
#include "tables/ucat.cpp"
}

namespace ay {
bool UnicodeClassifier::isPunct(uint32_t c) {
	uint32_t *end = table_unicode_punct + sizeof(table_unicode_punct) / sizeof(table_unicode_punct[0]);
	uint32_t *pair = std::lower_bound(table_unicode_punct, end, c);
	return pair != end && pair[0] == c;
}

bool UnicodeClassifier::isSpace(uint32_t c) {
	uint32_t *end = table_unicode_space + sizeof(table_unicode_space) / sizeof(table_unicode_space[0]);
	uint32_t *pair = std::lower_bound(table_unicode_space, end, c);
	return pair != end && pair[0] == c;
}

bool UnicodeClassifier::isNumber(uint32_t c) {
	uint32_t *end = table_unicode_numbers_dec + sizeof(table_unicode_numbers_dec) / sizeof(table_unicode_numbers_dec[0]);
	uint32_t *pair = std::lower_bound(table_unicode_numbers_dec, end, c);
	return pair != end && pair[0] == c;
}

}
