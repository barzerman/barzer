cmake_minimum_required (VERSION 2.6)
set(PKGDATADIR "${INSTALL_DIR}/kytea")
include_directories (blas)
INCLUDE (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
#INCLUDE (${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
#INCLUDE (${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
#INCLUDE (${CMAKE_ROOT}/Modules/CheckCXXSourceCompiles.cmake)
#INCLUDE (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
#INCLUDE (${CMAKE_ROOT}/Modules/CheckSymbolExists.cmake)

CHECK_INCLUDE_FILE("stdint.h" HAVE_STDINT_H)
CHECK_INCLUDE_FILE("dlfcn.h" HAVE_DLFCN_H)
CHECK_INCLUDE_FILE("inttypes.h" HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE("memory.h" HAVE_MEMORY_H)
CHECK_INCLUDE_FILE("stdbool.h" HAVE_STDBOOL_H)
CHECK_INCLUDE_FILE("stdint.h" HAVE_STDINT_H)
CHECK_INCLUDE_FILE("stdlib.h" HAVE_STDLIB_H)
CHECK_INCLUDE_FILE("strings.h" HAVE_STRINGS_H)
CHECK_INCLUDE_FILE("string.h" HAVE_STRING_H)
CHECK_INCLUDE_FILE("sys/stat.h" HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE("sys/types.h>" HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTD_H)


option (ENABLE_QUANTIZE "Build pybarzer" OFF)
if (ENABLE_QUANTIZE)
    set(DISABLE_QUANTIZE "0")
else ()
    set(DISABLE_QUANTIZE "1")
endif (ENABLE_QUANTIZE)

set(DISABLE_QUANTIZE 0)

configure_file ("src/include/kytea/config.h.in" "src/include/kytea/config.h")

include_directories("src/include"
                    "src/lib/liblinear")
add_subdirectory (src/lib/liblinear)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Isrc/include -Isrc/include/kytea -Wno-c++11-narrowing -Wall")
set (SRCS
    src/lib/corpus-io.cpp
    src/lib/corpus-io-eda.cpp
    src/lib/corpus-io-full.cpp
    src/lib/corpus-io-part.cpp
    src/lib/corpus-io-prob.cpp
    src/lib/corpus-io-raw.cpp
    src/lib/corpus-io-tokenized.cpp
    src/lib/dictionary.cpp
    src/lib/feature-io.cpp
    src/lib/feature-lookup.cpp
    src/lib/general-io.cpp
    src/lib/kytea-config.cpp
    src/lib/kytea.cpp
    src/lib/kytea-lm.cpp
    src/lib/kytea-model.cpp
    src/lib/kytea-string.cpp
    src/lib/kytea-struct.cpp
    src/lib/kytea-util.cpp
    src/lib/model-io.cpp
    src/lib/string-util.cpp
)

#SET(CMAKE_EXE_LINKER_FLAGS "-static")
add_library(kytea STATIC ${SRCS})
add_executable(kytea.exe src/api/api-example.cpp)
target_link_libraries (kytea  linear)
target_link_libraries (kytea.exe kytea)
