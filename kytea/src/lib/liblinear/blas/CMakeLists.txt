cmake_minimum_required (VERSION 2.6)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

set (SRCS
    daxpy.c 
    ddot.c
    dnrm2.c
    dscal.c
	)

add_library (blas STATIC ${SRCS})
