find_package(Boost COMPONENTS system python REQUIRED)
link_directories ( ${boost_LIBRARY_DIRS} )

find_package(PythonLibs REQUIRED)


include_directories("${PROJECT_BINARY_DIR}"
                    "${PROJECT_SOURCE_DIR}"
                    "${PROJECT_SOURCE_DIR}/ay"
                    ${boost_INCLUDE_DIRS}
                    ${PYTHON_INCLUDE_DIRS})
set(EXTRA_LIBS ${EXTRA_LIBS}
               ${Boost_SYSTEM_LIBRARY}
               ${Boost_PYTHON_LIBRARY}
               ${PYTHON_LIBRARIES}
               ay)
                    
add_library(python_util SHARED python_util.cpp umlaut.cpp)
target_link_libraries (python_util ${EXTRA_LIBS})
SET_TARGET_PROPERTIES(python_util PROPERTIES PREFIX "")