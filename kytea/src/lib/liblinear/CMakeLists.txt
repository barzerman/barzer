cmake_minimum_required (VERSION 2.6)

include_directories (blas)
add_subdirectory (blas)
set (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fPIC")

set (SRCS
    tron.cpp
    linear.cpp
	)

add_library (linear STATIC ${SRCS})
target_link_libraries (linear blas)

