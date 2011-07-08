cmake_minimum_required (VERSION 2.6)
project (barzer)
set (barzer_VERSION_MAJOR 0)
set (barzer_VERSION_MINOR 1)

set(EN_DIR "lg_en")
set(RU_DIR "lg_ru")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Boost COMPONENTS system python filesystem REQUIRED)
link_directories ( ${boost_LIBRARY_DIRS} )

find_package(Hunspell REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(EXPAT REQUIRED)

include_directories("${PROJECT_BINARY_DIR}"
                    "${PROJECT_SOURCE_DIR}"
                    "${PROJECT_SOURCE_DIR}/ay"
                    "${PROJECT_SOURCE_DIR}/${EN_DIR}"
                    "${PROJECT_SOURCE_DIR}/${RU_DIR}"
                    ${boost_INCLUDE_DIRS})
add_subdirectory (ay)
add_subdirectory (util)

set(EXTRA_LIBS ${EXTRA_LIBS}
               ${Boost_SYSTEM_LIBRARY}
               ${Boost_FILESYSTEM_LIBRARY}
               ${EXPAT_LIBRARIES}
               ${HUNSPELL_LIBRARIES}
               ay)

set(SOURCES barzer_barz.cpp
            barzer_basic_types.cpp
            barzer_datelib.cpp
            barzer_date_util.cpp
            barzer_dtaindex.cpp
            barzer_el_analysis.cpp
            barzer_el_btnd.cpp
            barzer_el_chain.cpp
            barzer_el_function.cpp
            barzer_el_matcher.cpp
            barzer_el_parser.cpp
            barzer_el_rewriter.cpp
            barzer_el_trie.cpp
            barzer_el_trie_processor.cpp
            barzer_el_trie_shell.cpp
            barzer_el_trie_walker.cpp
            barzer_el_wildcard.cpp
            barzer_el_xml.cpp
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
            barzer_spell.cpp
            barzer_storage_types.cpp
            barzer_token.cpp
            barzer_universe.cpp
            "${EN_DIR}/barzer_en_date_util.cpp"
            "${EN_DIR}/barzer_en_lex.cpp"
            "${RU_DIR}/barzer_ru_date_util.cpp"
            "${RU_DIR}/barzer_ru_lex.cpp"
            )


set(PYHTON_UTIL_LIBNAME "python_util.so")

set(INSTALL_DIR "/usr/share/barzer")
set(INSTALL_DATA_DIR "${INSTALL_DIR}/data")
set(INSTALL_UTIL_DIR "${INSTALL_DIR}/util")

list(APPEND CMAKE_CXX_FLAGS "-DBARZER_HOME=${INSTALL_DIR} -Wnon-virtual-dtor -fpic -Wall -g")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

IF( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" )
    #list(APPEND CMAKE_CXX_FLAGS "")
ENDIF( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" )

IF( ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" )
    #list(APPEND CMAKE_CXX_FLAGS "")
ENDIF( ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" )

add_executable(barzer.exe barzer.cpp ${SOURCES})
target_link_libraries (barzer.exe  ${EXTRA_LIBS})

install(FILES barzer.exe
        DESTINATION ${INSTALL_DIR})
install(FILES "util/${PYHTON_UTIL_LIBNAME}"
        DESTINATION ${INSTALL_UTIL_DIR})
install(DIRECTORY "data/configs" "data/entities" "data/rules"
        DESTINATION ${INSTALL_DATA_DIR})
