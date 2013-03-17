cmake_minimum_required (VERSION 2.6)

# WINDOWS USING:
# cmake -G "MinGW Makefiles" -DBOOST_ROOT=<boost> -DEXPAT_INCLUDE_DIR=<expat_include> -DEXPAT_LIBRARY=<expat_lib>

SET(CC $ENV{CC})
IF(CC STREQUAL "clang" AND NOT DEFINED ENV{CXX} AND NOT DEFINED CMAKE_CXX_COMPILER)
    MESSAGE("Setting c++ compiler to clang++; set CXX/CMAKE_CXX_COMPILER to override")
    set(CMAKE_CXX_COMPILER "clang++")
ENDIF()


project (barzer)
set (barzer_VERSION_MAJOR 0)
set (barzer_VERSION_MINOR 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wnon-virtual-dtor -Wno-unused-variable -Wno-unused-function")

set(EN_DIR "lg_en")
set(RU_DIR "lg_ru")
if (NOT WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{OPT} -fPIC")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

IF (NOT DEFINED BARZER_VERSION)
	FIND_PROGRAM (GIT_CMD git)
	EXECUTE_PROCESS (COMMAND "${GIT_CMD}" "--git-dir=${CMAKE_CURRENT_SOURCE_DIR}/.git" "describe" "--tags"
		OUTPUT_VARIABLE BARZER_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	IF (NOT DEFINED BARZER_VERSION)
		IF (WIN32)
			EXECUTE_PROCESS(COMMAND "date" "/T" OUTPUT_VARIABLE
				${BARZER_VERSION})
			STRING(REGEX REPLACE "(..)/(..)/..(..).*" "\\3\\2\\1"
				${BARZER_VERSION} ${${BARZER_VERSION}})
		ELSEIF(UNIX)
			EXECUTE_PROCESS(COMMAND "date" "+%d/%m/%Y" OUTPUT_VARIABLE
				${BARZER_VERSION})
			STRING(REGEX REPLACE "(..)/(..)/..(..).*" "\\3\\2\\1"
				${BARZER_VERSION} ${${BARZER_VERSION}})
		ELSE (WIN32)
			MESSAGE(SEND_ERROR "date not implemented")
			SET(${RESULT} 000000)
		ENDIF (WIN32)
	ENDIF (NOT DEFINED BARZER_VERSION)
	MESSAGE (STATUS "Detected Barzer version: ${BARZER_VERSION}")
ENDIF (NOT DEFINED BARZER_VERSION)

ADD_DEFINITIONS (-DBARZER_VERSION_STR="${BARZER_VERSION}")

IF(WIN32)
	SET(Boost_USE_STATIC_LIBS   ON)
	add_definitions(-DBOOST_THREAD_USE_LIB -DBOOST_PYTHON_STATIC_LIB)
	MESSAGE("-- Linking boost thread and python libs statically")
ENDIF()

find_package(Boost COMPONENTS system python thread regex filesystem REQUIRED)
find_package(EXPAT REQUIRED)
find_package(PythonLibs REQUIRED)



link_directories (  ${BOOST_LIBRARYDIR}
		    ${EXPAT_LIBRARIES} )


include_directories("${PROJECT_BINARY_DIR}"
                    "${PROJECT_SOURCE_DIR}"
                    "${PROJECT_SOURCE_DIR}/ay"
                    "${PROJECT_SOURCE_DIR}/util"
                    "${PROJECT_SOURCE_DIR}/${EN_DIR}"
                    "${PROJECT_SOURCE_DIR}/${RU_DIR}"
                     ${Boost_INCLUDE_DIRS}
                     ${PYTHON_INCLUDE_DIRS}
		     ${EXPAT_INCLUDE_DIRS})
add_subdirectory (ay)
add_subdirectory (util)
add_subdirectory (snowball)

set(EXTRA_LIBS ${EXTRA_LIBS}
               ${Boost_SYSTEM_LIBRARY}
               ${Boost_FILESYSTEM_LIBRARY}
               ${Boost_THREAD_LIBRARY}
               ${Boost_REGEX_LIBRARY}
               ay
               snowlib)

if(WIN32)
	set(EXTRA_LIBS ${EXTRA_LIBS}
			libwsock32.a
			libws2_32.a
	        	${EXPAT_LIBRARIES}/libexpat.lib)
else()
	set(EXTRA_LIBS ${EXTRA_LIBS} ${EXPAT_LIBRARIES} dl)
endif()

set(SOURCES 
            barzer_entity_name_index.cpp
            barzer_basic_types_range.cpp
            barzer_el_pattern_range.cpp
            barzer_el_rewrite_types.cpp
            barzer_el_pattern.cpp
            barzer_el_pattern_token.cpp
            barzer_el_pattern_datetime.cpp
            barzer_el_pattern_entity.cpp 
            barzer_el_pattern_number.cpp 
            barzer_el_pattern_entity.cpp 
            mongoose/mongoose.c barzer_universe.cpp  barzer_http.cpp
            barzer_global_pools.cpp 
            barzer_json_output.cpp 
            barzer_el_cast.cpp
            barzer_meaning.cpp 
            barzer_tokenizer.cpp 
            barzer_barz.cpp
            barzer_entity.cpp
            barzer_basic_types.cpp
            barzer_datelib.cpp
            barzer_date_util.cpp
            barzer_dtaindex.cpp
            barzer_el_analysis.cpp
            barzer_el_btnd.cpp
            barzer_el_chain.cpp
            barzer_el_function.cpp
            barzer_el_function_util.cpp
            barzer_el_matcher.cpp
            barzer_el_parser.cpp
            barzer_el_rewriter.cpp
            barzer_el_trie.cpp
            barzer_el_trie_processor.cpp
            barzer_el_trie_shell.cpp
            barzer_el_trie_walker.cpp
            barzer_el_wildcard.cpp
            barzer_el_xml.cpp
            barzer_el_proc.cpp
            barzer_language.cpp
            barzer_lexer.cpp
            barzer_loader_xml.cpp
            barzer_parse.cpp
            barzer_parse_types.cpp
            barzer_server.cpp
            barzer_server_request.cpp
            barzer_server_response.cpp
            barzer_settings.cpp
            barzer_shell.cpp
            barzer_storage_types.cpp
            barzer_token.cpp
            barzer_bzspell.cpp
            barzer_autocomplete.cpp
            barzer_emitter.cpp
            barzer_locale.cpp
			barzer_number.cpp
			barzer_barzxml.cpp
			barzer_el_rewrite_control.cpp
			barzer_relbits.cpp
			barzer_spellheuristics.cpp
			barzer_geoindex.cpp
			barzer_spell_features.cpp
            zurch/zurch_loader_longxml.cpp
            zurch_barzer.cpp
            zurch_server.cpp
            zurch_settings.cpp
            zurch_classifier.cpp
            zurch_docidx.cpp
            zurch_phrasebreaker.cpp
            zurch_tokenizer.cpp
			autotester/barzer_at_autotester.cpp
			autotester/barzer_at_comparators.cpp
            "${EN_DIR}/barzer_en_date_util.cpp"
            "${EN_DIR}/barzer_en_lex.cpp"
            "${RU_DIR}/barzer_ru_date_util.cpp"
            "${RU_DIR}/barzer_ru_lex.cpp"
            "${RU_DIR}/barzer_ru_stemmer.cpp"
            )

IF(NOT DEFINED INSTALL_DIR)
    set(INSTALL_DIR "/usr/share/barzer")
ENDIF(NOT DEFINED INSTALL_DIR)
set(INSTALL_DATA_DIR "${INSTALL_DIR}/data")
#set(INSTALL_UTIL_DIR "${INSTALL_DIR}/util")
set(INSTALL_UTIL_DIR "${INSTALL_DIR}")


SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "${INSTALL_DIR}")
SET(CMAKE_INSTALL_NAME_DIR "${INSTALL_DIR}")


IF(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
ENDIF()

MESSAGE("build type: ${CMAKE_BUILD_TYPE}")

SET(CMAKE_CXX_FLAGS_DEBUG "-g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_definitions(-DBARZER_HOME=${INSTALL_DIR})
add_definitions("\$(OPT)")

IF(DEFINED STD)
    add_definitions("-std=${STD}")
ENDIF()

set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

IF( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" )
    add_definitions("-Wno-parentheses")
ENDIF( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" )

IF( ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" )
    #list(APPEND CMAKE_CXX_FLAGS "")
ENDIF( ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" )

### libbarzer static
add_library(barzer_static ${SOURCES})
target_link_libraries (barzer_static ${EXTRA_LIBS})
set_target_properties(barzer_static PROPERTIES OUTPUT_NAME barzer_static )

option (ENABLE_BARZ_SHARED "Build shared version of barzer libraries" OFF)
option (ENABLE_PYBARZER "Build pybarzer" ON)

### libbarzer shared
if (ENABLE_BARZ_SHARED)
	add_library(barzer SHARED ${SOURCES})
	target_link_libraries (barzer barzer_static)
	set_target_properties(barzer PROPERTIES OUTPUT_NAME barzer )
endif (ENABLE_BARZ_SHARED)

### pybarzer
if (ENABLE_PYBARZER)
#	if (NOT ENABLE_BARZ_SHARED)
#		MESSAGE (ERROR "Pybarzer requires ENABLE_BARZ_SHARED set to true")
#	endif (NOT ENABLE_BARZ_SHARED)

	add_library(pybarzer SHARED barzer_python.cpp zurch_python.cpp)
	target_link_libraries (pybarzer barzer_static butil ${EXTRA_LIBS})
	SET_TARGET_PROPERTIES(pybarzer PROPERTIES PREFIX "")
	IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		SET_TARGET_PROPERTIES(pybarzer PROPERTIES SUFFIX ".so")
	ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif (ENABLE_PYBARZER)

### binary
add_executable(barzer.exe barzer.cpp)
target_link_libraries (barzer.exe  barzer_static)
IF (WIN32)
    SET_TARGET_PROPERTIES(barzer.exe PROPERTIES SUFFIX "")
ENDIF()

### test
add_executable(testme EXCLUDE_FROM_ALL testme.cpp ${SOURCES})
target_link_libraries (testme  ${EXTRA_LIBS})


GET_TARGET_PROPERTY(PYBARZER_LIBNAME pybarzer LOCATION)
GET_TARGET_PROPERTY(BARZER_LIBNAME barzer LOCATION)

#SET(CMAKE_INSTALL_SO_NO_EXE "0")

SET(CMAKE_INSTALL_RPATH "${INSTALL_DIR}")
#SET(CMAKE_INSTALL_NAME_DIR "${INSTALL_DIR}")

install(TARGETS barzer.exe
        DESTINATION "${INSTALL_DIR}")
if (ENABLE_BARZ_SHARED)
	install(TARGETS barzer
			DESTINATION ${INSTALL_DIR})
endif (ENABLE_BARZ_SHARED)
if (ENABLE_PYBARZER)
	install(TARGETS pybarzer
		   DESTINATION ${INSTALL_UTIL_DIR})
endif (ENABLE_PYBARZER)
install(DIRECTORY "data/configs" "data/entities" "data/rules" "data/spell"
        DESTINATION ${INSTALL_DATA_DIR})
install(DIRECTORY "proxy"
        DESTINATION ${INSTALL_DIR})
