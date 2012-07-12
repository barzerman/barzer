add_library(autotester
	autotester.cpp
	comparators.cpp
	)

add_executable(runtest cmpmain.cpp)

target_link_libraries(runtest autotester barzer_static)