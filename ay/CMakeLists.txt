set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{OPT} -Wall -Wnon-virtual-dtor -fPIC")
add_library(ay ay_cmdproc.cpp ay_logger.cpp ay_shell.cpp ay_string_pool.cpp ay_util.cpp ay_util_time.cpp)
