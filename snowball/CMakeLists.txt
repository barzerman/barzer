cmake_minimum_required (VERSION 2.6)

include_directories (libstemmer_c/include)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

set (SNOWBALL_SRCS
	libstemmer_c/src_c/stem_UTF_8_danish.c
	libstemmer_c/src_c/stem_UTF_8_dutch.c
	libstemmer_c/src_c/stem_UTF_8_english.c
	libstemmer_c/src_c/stem_UTF_8_finnish.c
	libstemmer_c/src_c/stem_UTF_8_french.c
	libstemmer_c/src_c/stem_UTF_8_german.c
	libstemmer_c/src_c/stem_UTF_8_hungarian.c
	libstemmer_c/src_c/stem_UTF_8_italian.c
	libstemmer_c/src_c/stem_UTF_8_norwegian.c
	libstemmer_c/src_c/stem_UTF_8_porter.c
	libstemmer_c/src_c/stem_UTF_8_portuguese.c
	libstemmer_c/src_c/stem_UTF_8_romanian.c
	libstemmer_c/src_c/stem_UTF_8_russian.c
	libstemmer_c/src_c/stem_UTF_8_spanish.c
	libstemmer_c/src_c/stem_UTF_8_swedish.c
	libstemmer_c/src_c/stem_UTF_8_turkish.c
	libstemmer_c/runtime/api.c
	libstemmer_c/runtime/utilities.c
	libstemmer_c/libstemmer/libstemmer_utf8.c
	)

add_library (snowlib STATIC ${SNOWBALL_SRCS})