find_package(Boost COMPONENTS system REQUIRED)

link_directories ( ${boost_LIBRARY_DIRS} )


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

                   
#SET_TARGET_PROPERTIES(butil PROPERTIES PREFIX "")


#IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#    SET_TARGET_PROPERTIES(pybarzer PROPERTIES SUFFIX ".so")
#ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


